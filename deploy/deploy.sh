#!/bin/bash
# AWS Deployment Script for Low-Latency Trading System

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
STACK_NAME="low-latency-trading-system"
REGION="us-east-1"
INSTANCE_TYPE="c6g.medium"

echo -e "${BLUE}🚀 Low-Latency Trading System AWS Deployment${NC}"
echo "================================================"

# Check if AWS CLI is installed
if ! command -v aws &> /dev/null; then
    echo -e "${RED}❌ AWS CLI is not installed. Please install it first.${NC}"
    exit 1
fi

# Check if AWS credentials are configured
if ! aws sts get-caller-identity &> /dev/null; then
    echo -e "${RED}❌ AWS credentials not configured. Please run 'aws configure' first.${NC}"
    exit 1
fi

# Check if CloudFormation template exists
if [ ! -f "cloudformation.yaml" ]; then
    echo -e "${RED}❌ CloudFormation template not found.${NC}"
    exit 1
fi

# Get parameters
echo -e "${YELLOW}📋 Deployment Parameters:${NC}"
echo "Stack Name: $STACK_NAME"
echo "Region: $REGION"
echo "Instance Type: $INSTANCE_TYPE"

# Check for key pair
echo -e "${YELLOW}🔑 Checking for EC2 Key Pair...${NC}"
read -p "Enter your EC2 Key Pair name: " KEY_NAME

if ! aws ec2 describe-key-pairs --key-names "$KEY_NAME" --region "$REGION" &> /dev/null; then
    echo -e "${RED}❌ Key pair '$KEY_NAME' not found in region $REGION${NC}"
    exit 1
fi

# Get SSH location
echo -e "${YELLOW}🌐 SSH Access Configuration:${NC}"
read -p "Enter your IP address for SSH access (or press Enter for 0.0.0.0/0): " SSH_LOCATION
SSH_LOCATION=${SSH_LOCATION:-"0.0.0.0/0"}

# Deploy CloudFormation stack
echo -e "${YELLOW}☁️ Deploying CloudFormation stack...${NC}"
aws cloudformation create-stack \
    --stack-name "$STACK_NAME" \
    --template-body file://cloudformation.yaml \
    --parameters \
        ParameterKey=InstanceType,ParameterValue="$INSTANCE_TYPE" \
        ParameterKey=KeyName,ParameterValue="$KEY_NAME" \
        ParameterKey=SSHLocation,ParameterValue="$SSH_LOCATION" \
    --capabilities CAPABILITY_NAMED_IAM \
    --region "$REGION"

echo -e "${YELLOW}⏳ Waiting for stack creation to complete...${NC}"
aws cloudformation wait stack-create-complete \
    --stack-name "$STACK_NAME" \
    --region "$REGION"

# Get stack outputs
echo -e "${YELLOW}📊 Getting deployment information...${NC}"
OUTPUTS=$(aws cloudformation describe-stacks \
    --stack-name "$STACK_NAME" \
    --region "$REGION" \
    --query 'Stacks[0].Outputs')

# Extract values
INSTANCE_ID=$(echo "$OUTPUTS" | jq -r '.[] | select(.OutputKey=="InstanceId") | .OutputValue')
PUBLIC_IP=$(echo "$OUTPUTS" | jq -r '.[] | select(.OutputKey=="PublicIP") | .OutputValue')
TELEMETRY_URL=$(echo "$OUTPUTS" | jq -r '.[] | select(.OutputKey=="TelemetryURL") | .OutputValue')
SSH_CMD=$(echo "$OUTPUTS" | jq -r '.[] | select(.OutputKey=="SSHCommand") | .OutputValue')

echo -e "${GREEN}✅ Deployment completed successfully!${NC}"
echo ""
echo -e "${BLUE}📋 Deployment Summary:${NC}"
echo "Instance ID: $INSTANCE_ID"
echo "Public IP: $PUBLIC_IP"
echo "Telemetry Dashboard: $TELEMETRY_URL"
echo "SSH Command: $SSH_CMD"
echo ""

# Wait for instance to be ready
echo -e "${YELLOW}⏳ Waiting for instance to be ready...${NC}"
aws ec2 wait instance-running --instance-ids "$INSTANCE_ID" --region "$REGION"

# Wait a bit more for system initialization
echo -e "${YELLOW}⏳ Waiting for system initialization...${NC}"
sleep 60

# Test connectivity
echo -e "${YELLOW}🔍 Testing connectivity...${NC}"
if curl -s --connect-timeout 10 "$TELEMETRY_URL/health" > /dev/null; then
    echo -e "${GREEN}✅ Telemetry service is responding${NC}"
else
    echo -e "${YELLOW}⚠️ Telemetry service not yet ready (this is normal for first deployment)${NC}"
fi

echo ""
echo -e "${GREEN}🎉 Deployment completed!${NC}"
echo ""
echo -e "${BLUE}📋 Next Steps:${NC}"
echo "1. SSH into your instance:"
echo "   $SSH_CMD"
echo ""
echo "2. Deploy your application files:"
echo "   scp -r -i $KEY_NAME.pem ./* ec2-user@$PUBLIC_IP:/opt/trading-system/"
echo ""
echo "3. Build and start the trading system:"
echo "   cd /opt/trading-system"
echo "   make && ./main"
echo ""
echo "4. Start the telemetry service:"
echo "   cd telemetry && uvicorn metrics:app --host 0.0.0.0 --port 8000"
echo ""
echo "5. Access the telemetry dashboard:"
echo "   $TELEMETRY_URL"
echo ""
echo -e "${YELLOW}💡 Useful Commands:${NC}"
echo "Monitor system: aws cloudwatch get-metric-statistics --namespace AWS/EC2 --metric-name CPUUtilization --dimensions Name=InstanceId,Value=$INSTANCE_ID --start-time \$(date -d '1 hour ago' -u +%Y-%m-%dT%H:%M:%S) --end-time \$(date -u +%Y-%m-%dT%H:%M:%S) --period 300 --statistics Average"
echo "View logs: aws logs describe-log-groups --log-group-name-prefix /aws/ec2/$INSTANCE_ID"
echo "Terminate stack: aws cloudformation delete-stack --stack-name $STACK_NAME --region $REGION" 