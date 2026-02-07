#!/usr/bin/env bash
set -euo pipefail

# init-database.sh - Initialize the RDS database schema from within the VPC
#
# Usage: bash cloudformation/scripts/init-database.sh [stack-name]
#
# Since RDS is not publicly accessible, this script SSHes into the EC2 instance
# and runs psql from there to apply schema.sql and functions.sql.
#
# Requires: SSH key (SSH_KEY_PATH env var) or SSM access

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
DB_DIR="$PROJECT_ROOT/database"

STACK_NAME="${1:-kanba-production}"
REGION="${AWS_DEFAULT_REGION:-us-east-1}"
SSH_KEY="${SSH_KEY_PATH:-}"
SSH_USER="ec2-user"

# Get stack outputs
echo "==> Fetching stack outputs..."
get_output() {
    aws cloudformation describe-stacks \
        --stack-name "$STACK_NAME" \
        --region "$REGION" \
        --query "Stacks[0].Outputs[?OutputKey=='$1'].OutputValue" \
        --output text
}

EC2_IP=$(get_output "EC2PublicIP")
DB_ENDPOINT=$(get_output "DatabaseEndpoint")

if [[ -z "$EC2_IP" || "$EC2_IP" == "None" ]]; then
    echo "ERROR: Could not find EC2 IP. Is the stack '$STACK_NAME' deployed?"
    exit 1
fi

echo "    EC2 IP: $EC2_IP"
echo "    DB Endpoint: $DB_ENDPOINT"

if [[ -z "$SSH_KEY" ]]; then
    echo "ERROR: SSH_KEY_PATH is required to copy SQL files to EC2."
    echo "  SSH_KEY_PATH=/path/to/key.pem bash $0"
    exit 1
fi

SSH_OPTS="-o StrictHostKeyChecking=no -i $SSH_KEY"

echo ""
echo "==> Copying SQL files to EC2..."
scp $SSH_OPTS "$DB_DIR/schema.sql" "$SSH_USER@$EC2_IP:/tmp/schema.sql"
scp $SSH_OPTS "$DB_DIR/functions.sql" "$SSH_USER@$EC2_IP:/tmp/functions.sql"

echo ""
echo "==> Running database initialization on EC2 (via psql in Docker)..."
echo "    This connects to RDS from within the VPC."

ssh $SSH_OPTS "$SSH_USER@$EC2_IP" bash -s << 'REMOTE_SCRIPT'
set -euo pipefail

# Read database connection info from the .env file
source /opt/kanba/.env

echo "Connecting to: $DATABASE_HOST:$DATABASE_PORT/$DATABASE_NAME as $DATABASE_USER"

# Run psql via Docker (using the official postgres image for the psql client)
docker run --rm \
    -v /tmp/schema.sql:/tmp/schema.sql:ro \
    -v /tmp/functions.sql:/tmp/functions.sql:ro \
    -e PGPASSWORD="$DATABASE_PASSWORD" \
    postgres:15-alpine \
    bash -c "
        echo '==> Applying schema.sql...'
        psql -h '$DATABASE_HOST' -p '$DATABASE_PORT' -U '$DATABASE_USER' -d '$DATABASE_NAME' -f /tmp/schema.sql
        echo ''
        echo '==> Applying functions.sql...'
        psql -h '$DATABASE_HOST' -p '$DATABASE_PORT' -U '$DATABASE_USER' -d '$DATABASE_NAME' -f /tmp/functions.sql
        echo ''
        echo '==> Verifying tables...'
        psql -h '$DATABASE_HOST' -p '$DATABASE_PORT' -U '$DATABASE_USER' -d '$DATABASE_NAME' -c '\dt'
    "

# Clean up SQL files
rm -f /tmp/schema.sql /tmp/functions.sql
REMOTE_SCRIPT

echo ""
echo "==> Database initialization complete!"
echo "    Tables and functions have been created on RDS."
