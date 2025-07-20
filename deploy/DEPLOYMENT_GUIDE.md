# AWS EC2 Deployment Guide

Complete guide for deploying the Low-Latency Trading System on AWS EC2.

## 🚀 Quick Start

### 1. Launch EC2 Instance

**Recommended Instance Types:**
- **t3.medium** - Good balance of CPU and memory for development/testing
- **c6g.medium** - Compute-optimized for better performance
- **c5.2xlarge** - For production workloads

**AMI:** Amazon Linux 2 (x86_64) or Amazon Linux 2023

### 2. Configure Security Group

Create a security group with the following rules:

| Type | Protocol | Port | Source | Description |
|------|----------|------|--------|-------------|
| SSH | TCP | 22 | Your IP | SSH access |
| Custom TCP | TCP | 8000 | 0.0.0.0/0 | Telemetry dashboard |
| Custom TCP | TCP | 8001 | 0.0.0.0/0 | Optional: Benchmark tool |

### 3. Connect and Deploy

```bash
# SSH into your instance
ssh -i your-key.pem ec2-user@YOUR_EC2_PUBLIC_IP

# Run the setup script
curl -sSL https://raw.githubusercontent.com/yourusername/low-latency-trading/main/deploy/aws-setup.sh | bash
```

## 📋 Detailed Setup Steps

### Step 1: Instance Preparation

```bash
# Update system
sudo yum update -y

# Install dependencies
sudo yum install -y gcc-c++ python3 python3-pip tmux git htop iotop sysstat make cmake3
```

### Step 2: Deploy Application

**Option A: Clone from Git**
```bash
cd /opt
sudo mkdir trading-system
sudo chown ec2-user:ec2-user trading-system
cd trading-system
git clone https://github.com/yourusername/low-latency-trading.git .
```

**Option B: Upload Files**
```bash
# From your local machine
scp -r -i your-key.pem ./* ec2-user@YOUR_EC2_PUBLIC_IP:/opt/trading-system/
```

### Step 3: Build and Install

```bash
cd /opt/trading-system

# Build C++ system
make clean
make -j$(nproc)

# Install Python dependencies
cd telemetry && pip3 install --user -r requirements.txt && cd ..
cd benchmark && pip3 install --user -r requirements.txt && cd ..
```

### Step 4: Configure Services

```bash
# Create systemd services
sudo cp deploy/trading-system.service /etc/systemd/system/
sudo cp deploy/telemetry.service /etc/systemd/system/

# Enable and start services
sudo systemctl daemon-reload
sudo systemctl enable trading-system telemetry
sudo systemctl start trading-system telemetry
```

## 🔧 Service Management

### Start Services
```bash
./start-services.sh
```

### Stop Services
```bash
./stop-services.sh
```

### Monitor System
```bash
./monitor.sh
```

### View Logs
```bash
# Trading system logs
sudo journalctl -u trading-system -f

# Telemetry service logs
sudo journalctl -u telemetry -f

# System logs
sudo tail -f /var/log/messages
```

## 📊 Monitoring and Metrics

### Telemetry Dashboard
Access your telemetry dashboard at:
```
http://YOUR_EC2_PUBLIC_IP:8000
```

**Available Endpoints:**
- `/` - Service information
- `/health` - Health check
- `/latest` - Latest metrics (JSON)
- `/metrics` - Prometheus metrics

### Key Metrics to Monitor

#### Performance Metrics
- **Average Order Latency**: Target < 500 μs
- **P95 Latency**: Target < 1 ms
- **P99 Latency**: Target < 2 ms
- **Throughput**: Orders per second
- **Batch Processing Time**: Time to process batches

#### System Metrics
- **CPU Usage**: Should be < 80%
- **Memory Usage**: Should be < 90%
- **Network I/O**: Monitor for bottlenecks
- **Disk I/O**: Should be minimal for in-memory trading

### Benchmark Analysis
```bash
# Generate performance reports
cd benchmark
python3 plots.py ../telemetry.log --output /tmp/benchmark-report
```

## 🔒 Security Configuration

### Firewall Rules
```bash
# Allow only necessary ports
sudo iptables -A INPUT -p tcp --dport 22 -j ACCEPT    # SSH
sudo iptables -A INPUT -p tcp --dport 8000 -j ACCEPT  # Telemetry
sudo iptables -P INPUT DROP
sudo service iptables save
```

### Security Best Practices
1. **Use SSH Keys**: Disable password authentication
2. **Restrict Access**: Limit SSH access to your IP
3. **Regular Updates**: Keep system packages updated
4. **Monitor Logs**: Set up log monitoring and alerting
5. **Backup Data**: Regular backups of configuration and data

## 🚀 Performance Optimization

### Instance Optimization
```bash
# Set CPU governor to performance
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Disable CPU frequency scaling
sudo systemctl disable cpupower
```

### Network Optimization
```bash
# Optimize network settings
echo 'net.core.rmem_max = 16777216' | sudo tee -a /etc/sysctl.conf
echo 'net.core.wmem_max = 16777216' | sudo tee -a /etc/sysctl.conf
echo 'net.ipv4.tcp_rmem = 4096 87380 16777216' | sudo tee -a /etc/sysctl.conf
echo 'net.ipv4.tcp_wmem = 4096 65536 16777216' | sudo tee -a /etc/sysctl.conf
sudo sysctl -p
```

### Memory Optimization
```bash
# Disable swap for low-latency systems
sudo swapoff -a
sudo sed -i '/swap/d' /etc/fstab
```

## 🔍 Troubleshooting

### Common Issues

#### Service Won't Start
```bash
# Check service status
sudo systemctl status trading-system
sudo systemctl status telemetry

# Check logs
sudo journalctl -u trading-system --no-pager
sudo journalctl -u telemetry --no-pager
```

#### Port Already in Use
```bash
# Check what's using port 8000
sudo netstat -tlnp | grep :8000

# Kill process if needed
sudo kill -9 <PID>
```

#### Permission Issues
```bash
# Fix file permissions
sudo chown -R ec2-user:ec2-user /opt/trading-system
chmod +x /opt/trading-system/main
```

#### Memory Issues
```bash
# Check memory usage
free -h
htop

# Check for memory leaks
sudo dmesg | grep -i "out of memory"
```

### Performance Issues

#### High Latency
1. Check CPU usage: `htop`
2. Monitor network: `iotop`
3. Check for context switches: `vmstat 1`
4. Verify CPU governor: `cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor`

#### Low Throughput
1. Check system resources: `./monitor.sh`
2. Verify network configuration
3. Check for bottlenecks in telemetry data
4. Monitor buffer utilization

## 📈 Scaling Considerations

### Vertical Scaling
- Upgrade to larger instance types (c5.4xlarge, c6g.4xlarge)
- Use instances with more CPU cores
- Consider memory-optimized instances for large datasets

### Horizontal Scaling
- Deploy multiple instances behind a load balancer
- Use auto-scaling groups for dynamic workloads
- Implement distributed processing for high throughput

### Cost Optimization
- Use Spot Instances for non-critical workloads
- Implement auto-shutdown for development environments
- Monitor and optimize resource usage

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

## 📞 Support

For issues or questions:
1. Check the troubleshooting section above
2. Review service logs
3. Monitor system resources
4. Contact your system administrator

---

**Status**: ✅ **Production Ready** 