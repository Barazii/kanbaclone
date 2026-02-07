#!/usr/bin/env bash
set -euo pipefail

# deploy-resources.sh - Deploy everything: CloudFormation stack, backend (ECR), frontend
#
# Usage: DB_PASSWORD=xxx SSH_KEY_PATH=/path/to/key.pem bash cloudformation/scripts/deploy-resources.sh [stack-name]
#
# Requires: aws cli, docker, node/npm, jq, SSH key for EC2 access
#
# Idempotent - checks existing state and skips steps that are already done.
#
# Order of operations:
#   1. Deploy CloudFormation stack (creates ECR repo, VPC, RDS, EC2, S3, CloudFront)
#   2. Build & push backend Docker image to the ECR repo created by the stack
#   3. Deploy backend on EC2 (pull image from ECR)
#   4. Build & deploy frontend to S3/CloudFront

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
CF_DIR="$PROJECT_ROOT/cloudformation"
FRONTEND_DIR="$PROJECT_ROOT/frontend"

STACK_NAME="${1:-kanba-production}"
TEMPLATE_FILE="$CF_DIR/infrastructure.yaml"
PARAMS_FILE="$CF_DIR/parameters/production.json"
REGION="${AWS_DEFAULT_REGION:-us-east-1}"
SSH_KEY="${SSH_KEY_PATH:-}"
SSH_USER="ec2-user"
ACCOUNT_ID=$(aws sts get-caller-identity --query Account --output text)

if [[ -z "${DB_PASSWORD:-}" ]]; then
    echo "ERROR: DB_PASSWORD environment variable is required."
    echo "Usage: DB_PASSWORD=xxx SSH_KEY_PATH=/path/to/key.pem bash $0 [stack-name]"
    exit 1
fi

if [[ -z "$SSH_KEY" ]]; then
    echo "ERROR: SSH_KEY_PATH environment variable is required."
    echo "Usage: DB_PASSWORD=xxx SSH_KEY_PATH=/path/to/key.pem bash $0 [stack-name]"
    exit 1
fi

get_output() {
    aws cloudformation describe-stacks \
        --stack-name "$STACK_NAME" \
        --region "$REGION" \
        --query "Stacks[0].Outputs[?OutputKey=='$1'].OutputValue" \
        --output text 2>/dev/null || echo ""
}

# Build parameters JSON with DB_PASSWORD injected
PARAMS_JSON=$(jq --arg pw "$DB_PASSWORD" \
    'map(if .ParameterKey == "DatabasePassword" then .ParameterValue = $pw else . end)' \
    "$PARAMS_FILE")

echo "============================================"
echo "  Kanba - Full Deployment"
echo "  Stack: $STACK_NAME"
echo "  Region: $REGION"
echo "============================================"
echo ""

# ===========================================================
# Step 1: CloudFormation Stack (creates ECR, VPC, RDS, EC2, S3, CloudFront)
# ===========================================================
echo ">>> Step 1/4: CloudFormation stack"
echo ""

echo "==> Validating template..."
aws cloudformation validate-template \
    --template-body "file://$TEMPLATE_FILE" \
    --region "$REGION" > /dev/null
echo "    Template is valid."

STACK_STATUS=$(aws cloudformation describe-stacks \
    --stack-name "$STACK_NAME" \
    --region "$REGION" \
    --query 'Stacks[0].StackStatus' \
    --output text 2>/dev/null || echo "DOES_NOT_EXIST")

if [[ "$STACK_STATUS" == "DOES_NOT_EXIST" ]]; then
    echo "==> Creating stack '$STACK_NAME'..."
    aws cloudformation create-stack \
        --stack-name "$STACK_NAME" \
        --template-body "file://$TEMPLATE_FILE" \
        --parameters "$PARAMS_JSON" \
        --capabilities CAPABILITY_NAMED_IAM \
        --region "$REGION" \
        --tags Key=Project,Value=kanba Key=Environment,Value=production

    echo "==> Waiting for stack creation (this may take 10-15 minutes)..."
    aws cloudformation wait stack-create-complete \
        --stack-name "$STACK_NAME" \
        --region "$REGION"
    echo "    Stack created."

elif [[ "$STACK_STATUS" == *"COMPLETE"* && "$STACK_STATUS" != *"DELETE"* ]]; then
    echo "    Stack already exists (status: $STACK_STATUS)."
    echo "==> Checking for updates..."
    UPDATE_OUTPUT=$(aws cloudformation update-stack \
        --stack-name "$STACK_NAME" \
        --template-body "file://$TEMPLATE_FILE" \
        --parameters "$PARAMS_JSON" \
        --capabilities CAPABILITY_NAMED_IAM \
        --region "$REGION" 2>&1) || {
        if echo "$UPDATE_OUTPUT" | grep -q "No updates are to be performed"; then
            echo "    No infrastructure changes needed. Skipping."
        else
            echo "    Update failed: $UPDATE_OUTPUT"
            exit 1
        fi
    }

    if echo "$UPDATE_OUTPUT" | grep -q "StackId"; then
        echo "==> Waiting for stack update..."
        aws cloudformation wait stack-update-complete \
            --stack-name "$STACK_NAME" \
            --region "$REGION"
        echo "    Stack updated."
    fi

elif [[ "$STACK_STATUS" == *"IN_PROGRESS"* ]]; then
    echo "    Stack is currently $STACK_STATUS. Waiting for it to finish..."
    if [[ "$STACK_STATUS" == "CREATE_IN_PROGRESS" ]]; then
        aws cloudformation wait stack-create-complete --stack-name "$STACK_NAME" --region "$REGION"
    else
        aws cloudformation wait stack-update-complete --stack-name "$STACK_NAME" --region "$REGION"
    fi
    echo "    Stack ready."

else
    echo "ERROR: Stack is in state '$STACK_STATUS'. Please resolve manually."
    exit 1
fi

echo ""
aws cloudformation describe-stacks \
    --stack-name "$STACK_NAME" \
    --region "$REGION" \
    --query 'Stacks[0].Outputs[*].[OutputKey,OutputValue]' \
    --output table

# Fetch outputs for subsequent steps
EC2_IP=$(get_output "EC2PublicIP")
INSTANCE_ID=$(get_output "EC2InstanceId")
BUCKET_NAME=$(get_output "FrontendBucketName")
DISTRIBUTION_ID=$(get_output "CloudFrontDistributionId")
FRONTEND_URL=$(get_output "FrontendUrl")
ECR_URI=$(get_output "ECRRepositoryUri")

echo ""
echo "    ECR URI: $ECR_URI"

# ===========================================================
# Step 2: Build & push backend Docker image to ECR
# ===========================================================
echo ""
echo ">>> Step 2/4: Build & push backend to ECR"
echo ""

echo "==> Logging into ECR..."
aws ecr get-login-password --region "$REGION" | \
    docker login --username AWS --password-stdin "$ACCOUNT_ID.dkr.ecr.$REGION.amazonaws.com" 2>/dev/null

echo "==> Building backend Docker image locally..."
docker build -t kanba-backend:latest "$PROJECT_ROOT/backend" 2>&1 | tail -5

echo "==> Pushing to ECR ($ECR_URI:latest)..."
docker tag kanba-backend:latest "$ECR_URI:latest"
docker push "$ECR_URI:latest" 2>&1 | tail -3

echo "    Image pushed to ECR."

# ===========================================================
# Step 3: Deploy backend on EC2 (pull from ECR)
# ===========================================================
echo ""
echo ">>> Step 3/4: Backend deployment (ECR pull on EC2)"
echo ""
echo "    EC2 IP: $EC2_IP"
echo "    Instance ID: $INSTANCE_ID"

SSH_OPTS="-o StrictHostKeyChecking=no -o ConnectTimeout=10 -i $SSH_KEY"

# Wait for EC2 to be reachable via SSH
echo "==> Waiting for EC2 SSH to be reachable..."
RETRIES=0
while ! ssh $SSH_OPTS "$SSH_USER@$EC2_IP" "echo ok" &>/dev/null; do
    RETRIES=$((RETRIES + 1))
    if [[ $RETRIES -ge 30 ]]; then
        echo "ERROR: EC2 not reachable via SSH after 5 minutes."
        exit 1
    fi
    echo "    Waiting... ($RETRIES/30)"
    sleep 10
done
echo "    EC2 is reachable."

# Wait for Docker to be ready (UserData may still be running)
echo "==> Waiting for Docker to be ready on EC2..."
while ! ssh $SSH_OPTS "$SSH_USER@$EC2_IP" "command -v docker &>/dev/null && sudo systemctl is-active docker &>/dev/null" 2>/dev/null; do
    sleep 10
    echo "    Still waiting for Docker..."
done
echo "    Docker is ready."

# Update FRONTEND_URL in .env (UserData leaves it blank to avoid circular dependency)
echo "==> Setting FRONTEND_URL in .env..."
ssh $SSH_OPTS "$SSH_USER@$EC2_IP" "sudo sed -i 's|^FRONTEND_URL=.*|FRONTEND_URL=$FRONTEND_URL|' /opt/kanba/.env"

# Pull from ECR and restart the container
echo "==> Pulling latest image from ECR and restarting backend..."
ssh $SSH_OPTS "$SSH_USER@$EC2_IP" bash -s <<REMOTE_SCRIPT
set -euxo pipefail

# Login to ECR
aws ecr get-login-password --region $REGION | sudo docker login --username AWS --password-stdin $ACCOUNT_ID.dkr.ecr.$REGION.amazonaws.com

# Pull latest image
sudo docker pull $ECR_URI:latest

# Stop existing container if running
sudo docker stop kanba-backend 2>/dev/null || true
sudo docker rm kanba-backend 2>/dev/null || true

# Start new container
sudo docker run -d \
    --name kanba-backend \
    --restart unless-stopped \
    --env-file /opt/kanba/.env \
    -p 3001:3001 \
    $ECR_URI:latest

# Wait and verify
echo "Waiting for backend to start..."
sleep 5

if sudo docker ps | grep -q kanba-backend; then
    echo "Backend container is running!"
    sudo docker logs --tail 10 kanba-backend
else
    echo "ERROR: Backend container failed to start."
    sudo docker logs kanba-backend
    exit 1
fi

# Clean up old images
sudo docker image prune -f
REMOTE_SCRIPT

echo "==> Backend deployed: http://$EC2_IP:3001"

# ===========================================================
# Step 4: Frontend (S3 + CloudFront)
# ===========================================================
echo ""
echo ">>> Step 4/4: Frontend deployment"
echo ""
echo "    S3 Bucket: $BUCKET_NAME"
echo "    CloudFront ID: $DISTRIBUTION_ID"

# Build frontend
echo "==> Installing dependencies..."
cd "$FRONTEND_DIR"
npm ci

echo "==> Building frontend (VITE_API_URL=$FRONTEND_URL/api)..."
VITE_API_URL="$FRONTEND_URL/api" npm run build

# Sync to S3 - assets with immutable cache, HTML with no-cache
echo "==> Syncing to S3..."
aws s3 sync dist/ "s3://$BUCKET_NAME/" \
    --region "$REGION" \
    --delete \
    --exclude "index.html" \
    --exclude "*.html" \
    --cache-control "public, max-age=31536000, immutable"

aws s3 sync dist/ "s3://$BUCKET_NAME/" \
    --region "$REGION" \
    --exclude "*" \
    --include "*.html" \
    --cache-control "no-cache, no-store, must-revalidate"

echo "    S3 sync complete."

# Invalidate CloudFront
echo "==> Invalidating CloudFront cache..."
INVALIDATION_ID=$(aws cloudfront create-invalidation \
    --distribution-id "$DISTRIBUTION_ID" \
    --paths "/*" \
    --query 'Invalidation.Id' \
    --output text)

echo "    Invalidation ID: $INVALIDATION_ID"
echo "    Waiting for invalidation..."
aws cloudfront wait invalidation-completed \
    --distribution-id "$DISTRIBUTION_ID" \
    --id "$INVALIDATION_ID"

# ===========================================================
# Done
# ===========================================================
echo ""
echo "============================================"
echo "  Deployment Complete!"
echo "============================================"
echo ""
echo "  Backend:  http://$EC2_IP:3001"
echo "  Frontend: $FRONTEND_URL"
echo ""
echo "  If this is the first deployment, run:"
echo "    SSH_KEY_PATH=$SSH_KEY bash cloudformation/scripts/init-database.sh $STACK_NAME"
