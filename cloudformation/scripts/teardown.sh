#!/usr/bin/env bash
set -euo pipefail

# teardown.sh - Delete all AWS resources
#
# Usage: bash cloudformation/scripts/teardown.sh [stack-name]
#
# This script:
# 1. Empties the S3 frontend bucket (required before stack deletion)
# 2. Deletes the CloudFormation stack (RDS will create a final snapshot)
# 3. Releases all associated resources

STACK_NAME="${1:-kanba-production}"
REGION="${AWS_DEFAULT_REGION:-us-east-1}"

echo "============================================"
echo "  Kanba - Teardown"
echo "  Stack: $STACK_NAME"
echo "  Region: $REGION"
echo "============================================"
echo ""
echo "WARNING: This will delete ALL resources including:"
echo "  - EC2 instance (backend)"
echo "  - RDS database (a final snapshot will be taken)"
echo "  - S3 bucket (frontend files)"
echo "  - CloudFront distribution"
echo "  - VPC and networking"
echo ""
read -p "Are you sure? Type 'yes' to confirm: " CONFIRM

if [[ "$CONFIRM" != "yes" ]]; then
    echo "Aborted."
    exit 0
fi

# Get S3 bucket name before deleting stack
get_output() {
    aws cloudformation describe-stacks \
        --stack-name "$STACK_NAME" \
        --region "$REGION" \
        --query "Stacks[0].Outputs[?OutputKey=='$1'].OutputValue" \
        --output text 2>/dev/null || echo ""
}

BUCKET_NAME=$(get_output "FrontendBucketName")

# Step 1: Empty S3 bucket
if [[ -n "$BUCKET_NAME" && "$BUCKET_NAME" != "None" ]]; then
    echo ""
    echo "==> Emptying S3 bucket: $BUCKET_NAME..."
    aws s3 rm "s3://$BUCKET_NAME" --recursive --region "$REGION"
    echo "    Bucket emptied."
else
    echo "==> No S3 bucket found (may already be deleted)."
fi

# Step 2: Stop the backend container (prevents issues with EIP release)
INSTANCE_ID=$(get_output "EC2InstanceId")
if [[ -n "$INSTANCE_ID" && "$INSTANCE_ID" != "None" ]]; then
    echo ""
    echo "==> Stopping backend container on EC2..."
    aws ssm send-command \
        --instance-ids "$INSTANCE_ID" \
        --document-name "AWS-RunShellScript" \
        --parameters 'commands=["docker stop kanba-backend 2>/dev/null || true"]' \
        --region "$REGION" > /dev/null 2>&1 || true
fi

# Step 3: Delete CloudFormation stack
echo ""
echo "==> Deleting CloudFormation stack '$STACK_NAME'..."
aws cloudformation delete-stack \
    --stack-name "$STACK_NAME" \
    --region "$REGION"

echo "==> Waiting for stack deletion (this may take several minutes)..."
aws cloudformation wait stack-delete-complete \
    --stack-name "$STACK_NAME" \
    --region "$REGION"

echo ""
echo "============================================"
echo "  Teardown Complete!"
echo "============================================"
echo ""
echo "  Stack '$STACK_NAME' has been deleted."
echo "  A final RDS snapshot was created automatically."
echo ""
echo "  To also delete the RDS snapshot, find it with:"
echo "    aws rds describe-db-snapshots --query 'DBSnapshots[?contains(DBSnapshotIdentifier, \`$STACK_NAME\`)]'"
