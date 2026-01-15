# Server Setup Guide

Step-by-step guide to set up HelloWorld on your cloud VM.

## Step 1: Create a Cloud VM

### Oracle Cloud (Free Tier - Recommended)

1. Go to https://cloud.oracle.com
2. Create a free account
3. Navigate to Compute > Instances > Create Instance
4. Choose:
   - Shape: VM.Standard.A1.Flex (ARM) or VM.Standard.E2.1.Micro
   - Image: Ubuntu 22.04 or Oracle Linux 8
   - Network: Create new VCN with public subnet
5. Download the SSH key pair when prompted
6. Wait for instance to start
7. Note the Public IP address

### Google Cloud (Free Tier)

1. Go to https://console.cloud.google.com
2. Navigate to Compute Engine > VM Instances > Create
3. Choose:
   - Machine type: e2-micro
   - Boot disk: Ubuntu 22.04 LTS
   - Firewall: Allow HTTPS traffic
4. Create and note the External IP

## Step 2: Open Port 443

### Oracle Cloud

1. Go to Networking > Virtual Cloud Networks
2. Click your VCN
3. Click Security Lists > Default Security List
4. Add Ingress Rule:
   - Source: 0.0.0.0/0
   - Protocol: TCP
   - Destination Port: 443

### Google Cloud

1. Go to VPC Network > Firewall
2. Create Firewall Rule:
   - Direction: Ingress
   - Targets: All instances
   - Source: 0.0.0.0/0
   - Protocols: tcp:443

### On the VM itself

```bash
# Ubuntu/Debian
sudo ufw allow 443/tcp

# RHEL/Oracle Linux
sudo firewall-cmd --permanent --add-port=443/tcp
sudo firewall-cmd --reload
```

## Step 3: Connect to Your VM

```bash
# Using the key from cloud provider
ssh -i /path/to/your-key.pem ubuntu@YOUR_VM_IP

# Or for Oracle Linux
ssh -i /path/to/your-key.pem opc@YOUR_VM_IP
```

## Step 4: Install HelloWorld Server

```bash
# Download
wget https://github.com/yourusername/helloworld/releases/latest/download/helloworld-server.tar.gz

# Extract
tar xzf helloworld-server.tar.gz
cd helloworld-server

# Build
mkdir build && cd build
cmake ..
make

# Install
sudo make install

# Run setup
sudo helloworld-server setup
```

## Step 5: Add Your SSH Key

On your local machine, generate a key for HelloWorld:

```bash
# Generate new key
ssh-keygen -t ed25519 -f ~/.ssh/helloworld -N ""

# View public key
cat ~/.ssh/helloworld.pub
```

Copy the public key output.

On your VM:

```bash
# Add key to authorized_keys
echo "PASTE_YOUR_PUBLIC_KEY_HERE" >> ~/.ssh/authorized_keys

# Set permissions
chmod 600 ~/.ssh/authorized_keys
```

## Step 6: Start the Server

```bash
sudo helloworld-server start
```

Expected output:
```
Starting HelloWorld server...
Server started successfully
Public IP: xxx.xxx.xxx.xxx
```

## Step 7: Verify

```bash
# Check status
sudo helloworld-server status

# Check stunnel is running
ps aux | grep stunnel

# Check port 443 is listening
sudo ss -tlnp | grep 443
```

## Troubleshooting

### "Address already in use"

Another service is using port 443:

```bash
sudo ss -tlnp | grep 443
# Kill the process or change HelloWorld port
```

### "Permission denied"

Run commands with sudo:

```bash
sudo helloworld-server start
```

### "stunnel not found"

Install stunnel:

```bash
# Ubuntu/Debian
sudo apt-get install stunnel4

# RHEL/Oracle Linux
sudo yum install stunnel
```

### Connection refused from client

1. Check firewall rules (cloud console and VM)
2. Verify stunnel is running
3. Check server logs:

```bash
journalctl -u stunnel
tail -f /var/log/stunnel.log
```

## Next Steps

1. Install HelloWorld client on your computer
2. Add server to client (hostname, key path, username)
3. Connect and verify your IP changed

See SETUP_CLIENT.md for client setup instructions.

