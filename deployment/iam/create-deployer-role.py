#!/usr/bin/env python3
"""
Create an IAM deployer user with minimum permissions for Kanba CloudFormation deployment.

Usage: python3 cloudformation/iam/create-deployer-role.py

Creates:
  - IAM policy: kanba-deployer-policy
  - IAM user: kanba-deployer
  - Access key pair (printed to stdout - save these!)

Requires: boto3, AWS credentials with IAM admin access
"""

import json
import sys

try:
    import boto3
except ImportError:
    print("ERROR: boto3 is required. Install with: pip install boto3")
    sys.exit(1)

POLICY_NAME = "kanba-deployer-policy"
USER_NAME = "kanba-deployer"

DEPLOYER_POLICY = {
    "Version": "2012-10-17",
    "Statement": [
        {
            "Sid": "CloudFormation",
            "Effect": "Allow",
            "Action": [
                "cloudformation:CreateStack",
                "cloudformation:UpdateStack",
                "cloudformation:DeleteStack",
                "cloudformation:DescribeStacks",
                "cloudformation:DescribeStackEvents",
                "cloudformation:DescribeStackResources",
                "cloudformation:GetTemplate",
                "cloudformation:ValidateTemplate",
                "cloudformation:ListStacks",
                "cloudformation:GetStackPolicy",
            ],
            "Resource": "*",
        },
        {
            "Sid": "EC2",
            "Effect": "Allow",
            "Action": [
                "ec2:RunInstances",
                "ec2:TerminateInstances",
                "ec2:StartInstances",
                "ec2:StopInstances",
                "ec2:DescribeInstances",
                "ec2:DescribeInstanceStatus",
                "ec2:DescribeImages",
                "ec2:DescribeKeyPairs",
                "ec2:CreateTags",
                "ec2:DeleteTags",
                "ec2:DescribeTags",
                "ec2:AllocateAddress",
                "ec2:ReleaseAddress",
                "ec2:AssociateAddress",
                "ec2:DisassociateAddress",
                "ec2:DescribeAddresses",
                "ec2:DescribeVolumes",
                "ec2:CreateVolume",
                "ec2:DeleteVolume",
                "ec2:AttachVolume",
                "ec2:DetachVolume",
                "ec2:DescribeAvailabilityZones",
                "ec2:DescribeAccountAttributes",
            ],
            "Resource": "*",
        },
        {
            "Sid": "VPCNetworking",
            "Effect": "Allow",
            "Action": [
                "ec2:CreateVpc",
                "ec2:DeleteVpc",
                "ec2:DescribeVpcs",
                "ec2:ModifyVpcAttribute",
                "ec2:CreateSubnet",
                "ec2:DeleteSubnet",
                "ec2:DescribeSubnets",
                "ec2:CreateInternetGateway",
                "ec2:DeleteInternetGateway",
                "ec2:AttachInternetGateway",
                "ec2:DetachInternetGateway",
                "ec2:DescribeInternetGateways",
                "ec2:CreateRouteTable",
                "ec2:DeleteRouteTable",
                "ec2:DescribeRouteTables",
                "ec2:CreateRoute",
                "ec2:DeleteRoute",
                "ec2:AssociateRouteTable",
                "ec2:DisassociateRouteTable",
                "ec2:CreateSecurityGroup",
                "ec2:DeleteSecurityGroup",
                "ec2:DescribeSecurityGroups",
                "ec2:AuthorizeSecurityGroupIngress",
                "ec2:RevokeSecurityGroupIngress",
                "ec2:AuthorizeSecurityGroupEgress",
                "ec2:RevokeSecurityGroupEgress",
            ],
            "Resource": "*",
        },
        {
            "Sid": "RDS",
            "Effect": "Allow",
            "Action": [
                "rds:CreateDBInstance",
                "rds:DeleteDBInstance",
                "rds:ModifyDBInstance",
                "rds:DescribeDBInstances",
                "rds:CreateDBSubnetGroup",
                "rds:DeleteDBSubnetGroup",
                "rds:DescribeDBSubnetGroups",
                "rds:DescribeDBSnapshots",
                "rds:CreateDBSnapshot",
                "rds:DeleteDBSnapshot",
                "rds:AddTagsToResource",
                "rds:RemoveTagsFromResource",
                "rds:ListTagsForResource",
            ],
            "Resource": "*",
        },
        {
            "Sid": "S3",
            "Effect": "Allow",
            "Action": [
                "s3:CreateBucket",
                "s3:DeleteBucket",
                "s3:PutBucketPolicy",
                "s3:DeleteBucketPolicy",
                "s3:GetBucketPolicy",
                "s3:PutBucketPublicAccessBlock",
                "s3:GetBucketPublicAccessBlock",
                "s3:PutObject",
                "s3:GetObject",
                "s3:DeleteObject",
                "s3:ListBucket",
                "s3:GetBucketLocation",
                "s3:GetBucketTagging",
                "s3:PutBucketTagging",
            ],
            "Resource": "*",
        },
        {
            "Sid": "CloudFront",
            "Effect": "Allow",
            "Action": [
                "cloudfront:CreateDistribution",
                "cloudfront:UpdateDistribution",
                "cloudfront:DeleteDistribution",
                "cloudfront:GetDistribution",
                "cloudfront:ListDistributions",
                "cloudfront:CreateInvalidation",
                "cloudfront:GetInvalidation",
                "cloudfront:ListInvalidations",
                "cloudfront:CreateOriginAccessControl",
                "cloudfront:DeleteOriginAccessControl",
                "cloudfront:GetOriginAccessControl",
                "cloudfront:ListOriginAccessControls",
                "cloudfront:TagResource",
                "cloudfront:UntagResource",
            ],
            "Resource": "*",
        },
        {
            "Sid": "IAM",
            "Effect": "Allow",
            "Action": [
                "iam:CreateRole",
                "iam:DeleteRole",
                "iam:GetRole",
                "iam:PutRolePolicy",
                "iam:DeleteRolePolicy",
                "iam:GetRolePolicy",
                "iam:AttachRolePolicy",
                "iam:DetachRolePolicy",
                "iam:ListAttachedRolePolicies",
                "iam:ListRolePolicies",
                "iam:CreateInstanceProfile",
                "iam:DeleteInstanceProfile",
                "iam:GetInstanceProfile",
                "iam:AddRoleToInstanceProfile",
                "iam:RemoveRoleFromInstanceProfile",
                "iam:PassRole",
                "iam:TagRole",
                "iam:UntagRole",
            ],
            "Resource": "*",
        },
        {
            "Sid": "SSM",
            "Effect": "Allow",
            "Action": [
                "ssm:SendCommand",
                "ssm:GetCommandInvocation",
                "ssm:DescribeInstanceInformation",
            ],
            "Resource": "*",
        },
        {
            "Sid": "STS",
            "Effect": "Allow",
            "Action": [
                "sts:GetCallerIdentity",
            ],
            "Resource": "*",
        },
        {
            "Sid": "CloudWatchLogs",
            "Effect": "Allow",
            "Action": [
                "logs:CreateLogGroup",
                "logs:CreateLogStream",
                "logs:PutLogEvents",
                "logs:DescribeLogGroups",
                "logs:DescribeLogStreams",
            ],
            "Resource": "*",
        },
    ],
}


def main():
    iam = boto3.client("iam")

    # Create policy
    print(f"Creating IAM policy: {POLICY_NAME}...")
    try:
        policy_response = iam.create_policy(
            PolicyName=POLICY_NAME,
            PolicyDocument=json.dumps(DEPLOYER_POLICY),
            Description="Minimum permissions for Kanba CloudFormation deployment",
        )
        policy_arn = policy_response["Policy"]["Arn"]
        print(f"  Policy ARN: {policy_arn}")
    except iam.exceptions.EntityAlreadyExistsException:
        account_id = boto3.client("sts").get_caller_identity()["Account"]
        policy_arn = f"arn:aws:iam::{account_id}:policy/{POLICY_NAME}"
        print(f"  Policy already exists: {policy_arn}")
        # Update existing policy
        print("  Updating policy to latest version...")
        # List and delete old versions (max 5 versions allowed)
        versions = iam.list_policy_versions(PolicyArn=policy_arn)["Versions"]
        for v in versions:
            if not v["IsDefaultVersion"]:
                iam.delete_policy_version(
                    PolicyArn=policy_arn, VersionId=v["VersionId"]
                )
        iam.create_policy_version(
            PolicyArn=policy_arn,
            PolicyDocument=json.dumps(DEPLOYER_POLICY),
            SetAsDefault=True,
        )

    # Create user
    print(f"\nCreating IAM user: {USER_NAME}...")
    try:
        iam.create_user(UserName=USER_NAME)
        print(f"  User created: {USER_NAME}")
    except iam.exceptions.EntityAlreadyExistsException:
        print(f"  User already exists: {USER_NAME}")

    # Attach policy to user
    print(f"\nAttaching policy to user...")
    iam.attach_user_policy(UserName=USER_NAME, PolicyArn=policy_arn)
    print("  Policy attached.")

    # Create access key
    print(f"\nCreating access key...")
    try:
        key_response = iam.create_access_key(UserName=USER_NAME)
        access_key = key_response["AccessKey"]

        print("\n" + "=" * 60)
        print("  SAVE THESE CREDENTIALS - THEY CANNOT BE RETRIEVED AGAIN")
        print("=" * 60)
        print(f"  AWS_ACCESS_KEY_ID:     {access_key['AccessKeyId']}")
        print(f"  AWS_SECRET_ACCESS_KEY: {access_key['SecretAccessKey']}")
        print("=" * 60)
        print("\nConfigure AWS CLI with:")
        print(f"  aws configure --profile kanba-deployer")
        print(f"  # Enter the access key ID and secret key above")
        print(f"  # Default region: us-east-1")
        print(f"  # Default output: json")
    except iam.exceptions.LimitExceededException:
        print("  WARNING: Maximum access keys reached for this user.")
        print("  Delete an old key first:")
        print(f"    aws iam list-access-keys --user-name {USER_NAME}")
        print(f"    aws iam delete-access-key --user-name {USER_NAME} --access-key-id <KEY_ID>")


if __name__ == "__main__":
    main()
