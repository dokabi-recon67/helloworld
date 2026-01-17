# HelloWorld Server Setup - Complete Google Cloud Guide

This guide walks you through setting up your own private tunnel server on Google Cloud Platform's **Free Tier**.

**Time needed:** 15-20 minutes

---

## Step 1: Create Google Cloud Account

1. Go to [console.cloud.google.com](https://console.cloud.google.com)
2. Click **"Get started for free"** or **"Sign in"** if you have an account
3. If new:
   - Enter your email
   - Add credit card (for verification only - you get $300 free credit)
   - Complete phone verification
4. Accept terms and create project (or use default)

---

## Step 2: Create a VM Instance

### 2.1 Navigate to Compute Engine

1. Click the **hamburger menu** (☰) in the top left
2. Go to **Compute Engine** → **VM instances**
3. Click **"Create Instance"** (blue button)

### 2.2 Configure Instance

#### Name and Region:

| Field | What to Enter |
|-------|---------------|
| **Name** | `helloworld-server` (or any name) |
| **Region** | `us-central1` (Iowa) - or any region you prefer |
| **Zone** | Leave as **"Any"** (Google will choose) |

#### Machine Configuration:

| Field | What to Select |
|-------|----------------|
| **Machine family** | **General-purpose** |
| **Series** | **E2** (low cost) |
| **Machine type** | **e2-micro** (Free tier eligible!) |
|   - vCPU | 1 shared core |
|   - Memory | 1 GB |

⚠️ **e2-micro is FREE** if you stay within free tier limits (1 instance per month, 30 GB disk).

#### Boot Disk:

1. Click **"Change"** under Boot disk
2. Select:
   - **Operating System:** `Ubuntu`
   - **Version:** `Ubuntu 24.04 LTS` (or latest)
   - **Boot disk type:** `Standard persistent disk`
   - **Size:** `30 GB` (free tier limit)
3. Click **"Select"**

#### Firewall:

✅ **Check both boxes:**
- ✅ **Allow HTTP traffic**
- ✅ **Allow HTTPS traffic**

This opens ports 80 and 443 automatically.

**Note:** The installer script will automatically detect if you're on Google Cloud and attempt to configure the GCP firewall. If it can't (because gcloud CLI isn't installed on the VM), you'll need to run this command **on your PC** (not the VM):

```bash
gcloud compute firewall-rules create allow-helloworld-443 \
  --allow tcp:443 \
  --source-ranges 0.0.0.0/0 \
  --description "Allow HelloWorld stunnel on port 443"
```

Or create it manually in [GCP Console > Firewall Rules](https://console.cloud.google.com/compute/firewalls).

### 2.3 Advanced Options - Auto Setup (RECOMMENDED!)

This automatically installs HelloWorld when your VM boots!

1. Click **"Advanced options"** to expand
2. Click **"Management"** tab
3. Find **"Automation"** section
4. Under **"Startup script"**, paste this:

```bash
#!/bin/bash
curl -sSL https://raw.githubusercontent.com/dokabi-recon67/helloworld/main/scripts/install_server.sh | sudo bash
```

**OR** download the cloud-init script from: https://raw.githubusercontent.com/dokabi-recon67/helloworld/main/scripts/cloud-init.yaml

### 2.4 Create the Instance

1. Review all settings
2. Click **"Create"** (blue button)
3. Wait 2-3 minutes for instance to show **"Running"** (green checkmark)

---

## Step 3: Get Your Server's IP Address

1. Once instance shows **"Running"**, look at the VM instances list
2. Find your instance (`helloworld-server`)
3. Look at the **"External IP"** column
4. Copy this IP address (example: `34.123.45.67`)
5. **Save it somewhere** - you'll need it for the client!

---

## Step 4: Set Up SSH Keys

### 4.1 Generate SSH Key on Your PC

**On Windows (PowerShell):**

```powershell
# Navigate to .ssh folder
cd $env:USERPROFILE\.ssh

# Generate key (if you don't have one)
ssh-keygen -t ed25519 -f helloworld_key -N '""'

# View public key
Get-Content helloworld_key.pub
```

**On Mac/Linux (Terminal):**

```bash
# Generate key
ssh-keygen -t ed25519 -f ~/.ssh/helloworld_key -N ""

# View public key
cat ~/.ssh/helloworld_key.pub
```

### 4.2 Add Public Key to Google Cloud VM

**Option A - Using Google Cloud Console (Easiest):**

1. Go to your VM instance page
2. Click **"SSH"** button (opens browser SSH)
3. In the browser terminal, run:
   ```bash
   mkdir -p ~/.ssh
   nano ~/.ssh/authorized_keys
   ```
4. Paste your **public key** (from `helloworld_key.pub`)
5. Press `Ctrl+X`, then `Y`, then `Enter` to save
6. Run: `chmod 600 ~/.ssh/authorized_keys`

**Option B - Using gcloud CLI:**

```bash
# Install gcloud CLI first, then:
gcloud compute instances add-metadata helloworld-server \
  --metadata-from-file ssh-keys=~/.ssh/helloworld_key.pub \
  --zone=YOUR_ZONE
```

**Option C - Manual SSH (if you have password access):**

```bash
# Connect with password first
ssh your-username@YOUR_SERVER_IP

# Then add your key
mkdir -p ~/.ssh
echo "YOUR_PUBLIC_KEY_HERE" >> ~/.ssh/authorized_keys
chmod 600 ~/.ssh/authorized_keys
```

---

## Step 5: Wait for Auto-Setup (If You Used Startup Script)

If you pasted the startup script, your server is setting itself up automatically!

1. Wait about **5 minutes** after the instance is "Running"
2. SSH into your server to check status:
   ```bash
   ssh -i ~/.ssh/helloworld_key your-username@YOUR_SERVER_IP
   helloworld-status
   ```

**Skip to Step 7** if you used the startup script.

---

## Step 6: Manual Setup (Only If You Didn't Use Startup Script)

If you didn't use the startup script, install manually:

### 6.1 Connect to Your Server

**On Windows (PowerShell):**

```powershell
# Navigate to where your key is
cd $env:USERPROFILE\.ssh

# Fix key permissions
icacls "helloworld_key" /inheritance:r /grant:r "$($env:USERNAME):R"

# Connect (replace YOUR_USERNAME and YOUR_IP)
ssh -i "helloworld_key" YOUR_USERNAME@YOUR_SERVER_IP
```

**On Mac/Linux (Terminal):**

```bash
# Fix key permissions
chmod 400 ~/.ssh/helloworld_key

# Connect (replace YOUR_USERNAME and YOUR_IP)
ssh -i ~/.ssh/helloworld_key YOUR_USERNAME@YOUR_SERVER_IP
```

### 6.2 Run the Installer

Once connected, run this single command:

```bash
curl -sSL https://raw.githubusercontent.com/dokabi-recon67/helloworld/main/scripts/install_server.sh | sudo bash
```

Wait for it to complete (about 2-3 minutes).

---

## Step 7: Verify Server is Running

SSH into your server and run:

```bash
helloworld-status
```

You should see:
- ✅ Stunnel (TLS): active
- ✅ SSH: active
- ✅ Tor: active (optional)
- Public IP: your server's IP

If everything shows "active", you're ready!

---

## Step 8: Download and Set Up the Client

### 8.1 Install Stunnel on Your PC

**Windows:**

```powershell
# Open PowerShell as Administrator
winget install stunnel
```

**OR** download from: https://www.stunnel.org/downloads.html

**Mac:**

```bash
brew install stunnel
```

**Linux:**

```bash
sudo apt install stunnel4  # Debian/Ubuntu
sudo yum install stunnel   # RHEL/CentOS
```

### 8.2 Download HelloWorld Client

Go to: https://github.com/dokabi-recon67/helloworld/releases

Download:
- **Windows:** `HelloWorld-win64.zip`
- **Mac:** `HelloWorld-macos.zip`

### 8.3 Extract and Run

**Windows:**
1. Right-click the ZIP → **"Extract All"**
2. Open the extracted folder
3. Double-click **`helloworld.exe`**

**Mac:**
1. Double-click the ZIP to extract
2. Open the extracted folder
3. Double-click **`HelloWorld.app`**
4. If blocked: Right-click → Open → Open

### 8.4 Add Your Server

1. In the HelloWorld app, click **"+ ADD"** button
2. Fill in:

| Field | What to Enter |
|-------|---------------|
| **Server Name** | Anything you want (e.g., "My Google Server") |
| **Host** | Your VM's External IP address |
| **Port** | `443` |
| **SSH Key** | Click Browse, select your `helloworld_key` file (NOT the .pub file!) |
| **Username** | Your Google Cloud username (usually your email username or `your-username`) |

3. Click **"Add Server"**

### 8.5 Connect!

1. Select your server from the dropdown
2. Click **"CONNECT"**
3. Wait a few seconds for the connection

---

## Step 9: Verify It's Working

1. Open your web browser
2. Go to: https://api.ipify.org
3. You should see **your VM's IP address**, NOT your home IP!

**If you see your VM's IP:** Congratulations! Everything is working! 🎉

**If you see your home IP:** Make sure "Full Tunnel Mode" is enabled in the app.

---

## Server Commands (Optional)

SSH into your server and run these commands:

```bash
# Check status of everything
helloworld-status

# Get a new Tor IP (when using Tor mode)
helloworld-rotate

# Restart HelloWorld service
sudo systemctl restart helloworld

# View setup log
cat /var/log/helloworld-setup.log
```

---

## Troubleshooting

### "Connection refused" or timeout
- Check that VM instance is **Running**
- Verify you're using the correct **External IP**
- Make sure firewall allows port 443:
  - **Local firewall (UFW/Firewalld):** Automatically configured by installer
  - **GCP firewall:** Automatically configured if gcloud CLI is available, otherwise run on your PC:
    ```bash
    gcloud compute firewall-rules create allow-helloworld-443 \
      --allow tcp:443 --source-ranges 0.0.0.0/0 \
      --description "Allow HelloWorld stunnel on port 443"
    ```
  - Or check in [GCP Console > Firewall Rules](https://console.cloud.google.com/compute/firewalls)
- Wait 5 minutes if you just created the instance

### "Permission denied" SSH error
- Make sure key file permissions are set correctly
- Windows: Run `icacls "key" /inheritance:r /grant:r "$($env:USERNAME):R"`
- Mac/Linux: Run `chmod 400 ~/.ssh/helloworld_key`
- Verify your public key is in `~/.ssh/authorized_keys` on the server

### "stunnel not found" error in client
- Install stunnel: `winget install stunnel` (Windows) or `brew install stunnel` (Mac)
- Restart HelloWorld app after installing

### App says "Connected" but IP didn't change
- Enable **"Full Tunnel Mode"** toggle in the app
- Restart the app and reconnect

### Can't SSH into server
- Check your username (it's usually your Google account username)
- Verify the External IP is correct
- Make sure your public key is in `~/.ssh/authorized_keys` on the server

### Startup script didn't run
- Check the log: `sudo cat /var/log/helloworld-setup.log`
- Run manual install (Step 6)

---

## Google Cloud Free Tier Limits

| Resource | Free Amount |
|----------|-------------|
| e2-micro instances | 1 per month (744 hours) |
| Standard persistent disk | 30 GB |
| Outbound data | 1 GB/month |
| Ingress data | Free |

HelloWorld works well within these limits for personal use!

---

## Optional: Reserve Your IP Address

By default, your External IP may change if you stop/restart the instance. To keep it permanent:

1. Go to **VPC network** → **External IP addresses**
2. Click **"Reserve"**
3. Enter a name (e.g., `helloworld-ip`)
4. Select your VM instance
5. Click **"Reserve"**

Now your IP stays the same forever!

---

## Need Help?

- **GitHub Issues:** https://github.com/dokabi-recon67/helloworld/issues
- **Google Cloud Docs:** https://cloud.google.com/compute/docs

