#!/bin/bash
# AWS EC2 Setup Script for Low-Latency Trading System
# Run this script on a fresh EC2 instance

set -e  # Exit on any error

echo "🚀 Setting up Low-Latency Trading System on AWS EC2"
echo "=================================================="

# Update system packages
echo "📦 Updating system packages..."
sudo yum update -y

# Install system dependencies
echo "🔧 Installing system dependencies..."
sudo yum install -y \
    gcc-c++ \
    python3 \
    python3-pip \
    tmux \
    git \
    htop \
    iotop \
    sysstat \
    make \
    cmake3 \
    boost-devel \
    openssl-devel \
    zlib-devel

# Create symlink for cmake
sudo ln -sf /usr/bin/cmake3 /usr/bin/cmake

# Create application directory
echo "📁 Setting up application directory..."
sudo mkdir -p /opt/trading-system
sudo chown ec2-user:ec2-user /opt/trading-system
cd /opt/trading-system

# Clone repository (replace with your actual repo URL)
echo "📥 Cloning repository..."
git clone https://github.com/yourusername/low-latency-trading.git .
# Or if you're uploading files directly, skip this step

# Build C++ trading system
echo "🔨 Building C++ trading system..."
make clean
make -j$(nproc)

# Install Python dependencies for telemetry
echo "🐍 Installing Python dependencies..."
cd telemetry
pip3 install --user -r requirements.txt
cd ..

# Install Python dependencies for benchmark
echo "📊 Installing benchmark dependencies..."
cd benchmark
pip3 install --user -r requirements.txt
cd ..

# Create systemd service for the trading system
echo "⚙️ Creating systemd service for trading system..."
sudo tee /etc/systemd/system/trading-system.service > /dev/null <<EOF
[Unit]
Description=Low-Latency Trading System
After=network.target

[Service]
Type=simple
User=ec2-user
WorkingDirectory=/opt/trading-system
ExecStart=/opt/trading-system/main
Restart=always
RestartSec=5
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF

# Create systemd service for telemetry
echo "📈 Creating systemd service for telemetry..."
sudo tee /etc/systemd/system/telemetry.service > /dev/null <<EOF
[Unit]
Description=Telemetry Service
After=network.target

[Service]
Type=simple
User=ec2-user
WorkingDirectory=/opt/trading-system/telemetry
ExecStart=/usr/bin/python3 -m uvicorn metrics:app --host 0.0.0.0 --port 8000
Restart=always
RestartSec=5
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF

# Reload systemd and enable services
echo "🔄 Enabling services..."
sudo systemctl daemon-reload
sudo systemctl enable trading-system
sudo systemctl enable telemetry

# Create log directories
echo "📝 Setting up logging..."
sudo mkdir -p /var/log/trading-system
sudo chown ec2-user:ec2-user /var/log/trading-system

# Create startup script
echo "🚀 Creating startup script..."
tee start-services.sh > /dev/null <<EOF
#!/bin/bash
# Start all trading system services

echo "Starting Low-Latency Trading System..."

# Start trading system
sudo systemctl start trading-system
echo "✅ Trading system started"

# Start telemetry service
sudo systemctl start telemetry
echo "✅ Telemetry service started"

# Show status
echo "📊 Service Status:"
sudo systemctl status trading-system --no-pager
sudo systemctl status telemetry --no-pager

echo "🌐 Telemetry dashboard available at: http://\$(curl -s http://169.254.169.254/latest/meta-data/public-ipv4):8000"
EOF

chmod +x start-services.sh

# Create monitoring script
echo "📊 Creating monitoring script..."
tee monitor.sh > /dev/null <<EOF
#!/bin/bash
# Monitor trading system performance

echo "📊 Trading System Monitoring"
echo "============================"

# Check service status
echo "🔍 Service Status:"
sudo systemctl status trading-system --no-pager | head -10
sudo systemctl status telemetry --no-pager | head -10

# Check telemetry endpoint
echo "📈 Telemetry Health Check:"
curl -s http://localhost:8000/health | python3 -m json.tool 2>/dev/null || echo "Telemetry service not responding"

# System resources
echo "💻 System Resources:"
echo "CPU Usage:"
top -bn1 | grep "Cpu(s)" | awk '{print \$2}' | cut -d'%' -f1

echo "Memory Usage:"
free -h | grep Mem | awk '{print "Used: " \$3 " / " \$2 " (" \$3/\$2*100 "%)"}'

echo "Disk Usage:"
df -h / | tail -1 | awk '{print "Used: " \$3 " / " \$2 " (" \$5 ")"}'

# Network connections
echo "🌐 Network Connections:"
netstat -tlnp | grep :8000 || echo "No connections on port 8000"
EOF

chmod +x monitor.sh

# Create stop script
echo "🛑 Creating stop script..."
tee stop-services.sh > /dev/null <<EOF
#!/bin/bash
# Stop all trading system services

echo "Stopping Low-Latency Trading System..."

# Stop telemetry service
sudo systemctl stop telemetry
echo "✅ Telemetry service stopped"

# Stop trading system
sudo systemctl stop trading-system
echo "✅ Trading system stopped"

echo "📊 Final Service Status:"
sudo systemctl status trading-system --no-pager
sudo systemctl status telemetry --no-pager
EOF

chmod +x stop-services.sh

# Set up firewall (if using iptables)
echo "🔥 Configuring firewall..."
sudo yum install -y iptables-services
sudo systemctl enable iptables
sudo systemctl start iptables

# Allow SSH, HTTP for telemetry, and other necessary ports
sudo iptables -A INPUT -p tcp --dport 22 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 8000 -j ACCEPT
sudo iptables -A INPUT -p icmp -j ACCEPT
sudo iptables -A INPUT -i lo -j ACCEPT
sudo iptables -P INPUT DROP
sudo iptables -P FORWARD DROP
sudo iptables -P OUTPUT ACCEPT

# Save iptables rules
sudo service iptables save

echo "✅ AWS EC2 setup completed!"
echo ""
echo "📋 Next Steps:"
echo "1. Configure EC2 Security Group to allow:"
echo "   - TCP 22 (SSH)"
echo "   - TCP 8000 (Telemetry Dashboard)"
echo ""
echo "2. Start services:"
echo "   ./start-services.sh"
echo ""
echo "3. Monitor system:"
echo "   ./monitor.sh"
echo ""
echo "4. Access telemetry dashboard:"
echo "   http://YOUR_EC2_PUBLIC_IP:8000"
echo ""
echo "5. View logs:"
echo "   sudo journalctl -u trading-system -f"
echo "   sudo journalctl -u telemetry -f" 