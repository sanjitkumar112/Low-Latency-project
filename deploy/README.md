# AWS Cloud Deployment

Complete AWS deployment solution for the Low-Latency Trading System with automated infrastructure provisioning and monitoring.

## 🚀 Quick Deployment

### Prerequisites
- AWS CLI installed and configured
- EC2 Key Pair created in your AWS region
- jq installed for JSON parsing

### One-Click Deployment
```bash
cd deploy
./deploy.sh
```

## 📁 Deployment Files

### Core Files
- `aws-setup.sh` - Manual EC2 setup script
- `deploy.sh` - Automated deployment script
- `cloudformation.yaml` - Infrastructure as Code template
- `trading-system.service` - Systemd service for C++ trading system
- `telemetry.service` - Systemd service for telemetry dashboard

### Documentation
- `DEPLOYMENT_GUIDE.md` - Comprehensive deployment guide
- `README.md` - This file

## ☁️ Infrastructure Components

### EC2 Instance
- **Instance Type**: c6g.medium (ARM-based, cost-effective)
- **OS**: Amazon Linux 2023
- **Storage**: 8GB GP3 EBS volume
- **Network**: Public subnet with internet access

### Security
- **Security Group**: Restricts access to SSH (22) and telemetry (8000)
- **IAM Role**: CloudWatch monitoring permissions
- **VPC**: Isolated network with public subnet

### Monitoring
- **CloudWatch Agent**: System metrics collection
- **CloudWatch Dashboard**: Real-time monitoring
- **CloudWatch Logs**: Centralized logging

## 🔧 Deployment Options

### Option 1: Automated Deployment (Recommended)
```bash
# Run the automated deployment script
./deploy.sh
```

**Features:**
- Interactive parameter input
- Automatic stack creation
- Connectivity testing
- Deployment verification

### Option 2: Manual CloudFormation
```bash
# Deploy using AWS CLI
aws cloudformation create-stack \
    --stack-name low-latency-trading-system \
    --template-body file://cloudformation.yaml \
    --parameters ParameterKey=KeyName,ParameterValue=your-key-name \
    --capabilities CAPABILITY_NAMED_IAM
```

### Option 3: Manual EC2 Setup
```bash
# Launch EC2 instance manually, then run:
curl -sSL https://raw.githubusercontent.com/yourusername/low-latency-trading/main/deploy/aws-setup.sh | bash
```

## 📊 Monitoring and Metrics

### CloudWatch Dashboard
Access your CloudWatch dashboard to monitor:
- CPU utilization
- Memory usage
- Network I/O
- Disk I/O
- TCP connections

### Telemetry Dashboard
Access your telemetry dashboard at:
```
http://YOUR_EC2_PUBLIC_IP:8000
```

**Available endpoints:**
- `/` - Service information
- `/health` - Health check
- `/latest` - Latest metrics (JSON)
- `/metrics` - Prometheus metrics

### Key Metrics to Monitor

#### Performance Targets
- **Average Order Latency**: < 500 μs
- **P95 Latency**: < 1 ms
- **P99 Latency**: < 2 ms
- **Throughput**: > 10,000 ops/sec
- **CPU Usage**: < 80%
- **Memory Usage**: < 90%

#### System Health
- **Service Status**: Both services running
- **Network Connectivity**: Port 8000 accessible
- **Error Rates**: Low network and application errors
- **Resource Utilization**: Balanced CPU/memory usage

## 🔒 Security Configuration

### Network Security
- **SSH Access**: Restricted to specified IP ranges
- **Telemetry Access**: Public access for monitoring
- **Firewall**: iptables configured for minimal exposure

### Instance Security
- **IAM Role**: Least privilege access
- **Systemd Services**: Secure service configuration
- **File Permissions**: Restricted access to application files

### Best Practices
1. **Use SSH Keys**: Disable password authentication
2. **Regular Updates**: Keep system packages updated
3. **Monitor Access**: Review CloudTrail logs
4. **Backup Configuration**: Regular backups of config files

## 🚀 Performance Optimization

### Instance Tuning
```bash
# Set CPU governor to performance
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Optimize network settings
echo 'net.core.rmem_max = 16777216' | sudo tee -a /etc/sysctl.conf
echo 'net.core.wmem_max = 16777216' | sudo tee -a /etc/sysctl.conf
sudo sysctl -p
```

### Application Tuning
- **CPU Affinity**: Pin processes to specific cores
- **Memory Locking**: Lock critical memory pages
- **Network Optimization**: Use kernel bypass if needed
- **Buffer Tuning**: Optimize ring buffer sizes

## 📈 Scaling Considerations

### Vertical Scaling
- Upgrade to larger instance types (c5.4xlarge, c6g.4xlarge)
- Use instances with more CPU cores
- Consider memory-optimized instances

### Horizontal Scaling
- Deploy multiple instances behind a load balancer
- Use auto-scaling groups for dynamic workloads
- Implement distributed processing

### Cost Optimization
- Use Spot Instances for non-critical workloads
- Implement auto-shutdown for development
- Monitor and optimize resource usage

## 🔍 Troubleshooting

### Common Issues

#### Deployment Fails
```bash
# Check CloudFormation events
aws cloudformation describe-stack-events --stack-name low-latency-trading-system

# Check for resource limits
aws service-quotas get-service-quota --service-code ec2 --quota-code L-1216C47A
```

#### Service Won't Start
```bash
# Check service status
sudo systemctl status trading-system
sudo systemctl status telemetry

# Check logs
sudo journalctl -u trading-system -f
sudo journalctl -u telemetry -f
```

#### High Latency
1. Check CPU usage: `htop`
2. Monitor network: `iotop`
3. Check for context switches: `vmstat 1`
4. Verify CPU governor settings

#### Connectivity Issues
```bash
# Check security group rules
aws ec2 describe-security-groups --group-ids sg-xxxxxxxxx

# Test connectivity
telnet YOUR_EC2_IP 8000
curl http://YOUR_EC2_IP:8000/health
```

## 🎯 Production Checklist

- [ ] Security group configured correctly
- [ ] All services running and healthy
- [ ] Telemetry dashboard accessible
- [ ] Performance metrics within targets
- [ ] Logging and monitoring configured
- [ ] Backup strategy implemented
- [ ] Alerting configured
- [ ] Documentation updated
- [ ] Team access configured
- [ ] Disaster recovery plan ready

## 💰 Cost Estimation

### Monthly Costs (us-east-1)
- **c6g.medium**: ~$25/month
- **EBS Storage**: ~$1/month
- **Data Transfer**: ~$5/month
- **CloudWatch**: ~$3/month
- **Total**: ~$34/month

### Cost Optimization Tips
1. Use Spot Instances for development
2. Implement auto-shutdown for non-production
3. Monitor and optimize resource usage
4. Use reserved instances for production

## 🚀 Next Steps

1. **Deploy Application**: Upload your C++ and Python code
2. **Configure Monitoring**: Set up alerts and dashboards
3. **Performance Testing**: Run benchmarks and optimize
4. **Security Hardening**: Implement additional security measures
5. **Backup Strategy**: Set up automated backups
6. **Documentation**: Update team documentation

## 📞 Support

For deployment issues:
1. Check the troubleshooting section
2. Review CloudFormation events
3. Check service logs
4. Monitor system resources
5. Contact AWS support if needed

---

**Status**: ✅ **Production Ready** 