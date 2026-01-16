# HelloWorld Server Setup - Complete Oracle Cloud Guide

This guide walks you through setting up your own private tunnel server on Oracle Cloud's **Always Free** tier. No credit card charges ever.

**Time needed:** 15-20 minutes

---

## Step 1: Create Oracle Cloud Account

1. Go to [cloud.oracle.com](https://cloud.oracle.com)
2. Click **Sign Up** (top right)
3. Enter your email and country
4. Check your email and click the verification link
5. Fill in your details:
   - First name, Last name
   - Address, City, Country
   - Phone number
6. Add a credit card
   - **Don't worry!** This is only for verification
   - You will **NOT be charged** for Always Free resources
7. Complete phone verification (they'll text you a code)
8. Wait for account activation (usually instant, sometimes up to 24 hours)
9. Once activated, sign in to the Oracle Cloud Console

---

## Step 2: Create a Virtual Cloud Network (VCN)

A VCN is like your private network in the cloud.

### 2.1 Open VCN Wizard

1. Click the **hamburger menu** (☰) in the top left
2. Go to **Networking** → **Virtual Cloud Networks**
3. Click **Start VCN Wizard** (blue button)
4. Select **"Create VCN with Internet Connectivity"**
5. Click **Start VCN Wizard**

### 2.2 Configure VCN

| Field | What to Enter |
|-------|---------------|
| **VCN Name** | `helloworld-vcn` |
| **Compartment** | Your root compartment (default) |
| **IPv4 CIDR Block** | `10.0.0.0/16` (leave default) |
| **Use DNS hostnames in this VCN** | ✅ Check this box |

### 2.3 Add IPv6 (Optional but Recommended)

1. Scroll down to **"Add IPv6 Prefixes"**
2. Click **"Add IPv6 Prefix"**
3. Select **"Oracle allocated IPv6 /56 prefix"**
4. Leave the prefix field empty (auto-assigned)

### 2.4 Create

1. Click **Next**
2. Review your settings
3. Click **Create**
4. Wait for it to complete (shows green checkmarks)
5. Click **View VCN** when done

---

## Step 3: Open Port 443 (Security Rules)

This allows your tunnel traffic through the firewall.

### 3.1 Navigate to Security List

1. In your VCN page, find **Subnets** section
2. Click **"Public Subnet-helloworld-vcn"**
3. Under **Security Lists**, click **"Default Security List for helloworld-vcn"**
4. Click **"Add Ingress Rules"**

### 3.2 Add IPv4 Rule (Required)

Click **"Add Ingress Rules"** and enter:

| Field | Value |
|-------|-------|
| **Stateless** | Leave unchecked |
| **Source Type** | CIDR |
| **Source CIDR** | `0.0.0.0/0` |
| **IP Protocol** | TCP |
| **Source Port Range** | Leave empty (All) |
| **Destination Port Range** | `443` |
| **Description** | `HelloWorld Tunnel IPv4` |

### 3.3 Add IPv6 Rule (If you enabled IPv6)

Click **"+ Another Ingress Rule"** and enter:

| Field | Value |
|-------|-------|
| **Stateless** | Leave unchecked |
| **Source Type** | CIDR |
| **Source CIDR** | `::/0` |
| **IP Protocol** | TCP |
| **Source Port Range** | Leave empty (All) |
| **Destination Port Range** | `443` |
| **Description** | `HelloWorld Tunnel IPv6` |

### 3.4 Save

Click **"Add Ingress Rules"** to save all rules.

---

## Step 4: Create Your VM Instance

### 4.1 Navigate to Instances

1. Click the **hamburger menu** (☰)
2. Go to **Compute** → **Instances**
3. Click **"Create Instance"** (blue button)

### 4.2 Name and Placement

| Field | What to Enter |
|-------|---------------|
| **Name** | `helloworld-server` (or any name you like) |
| **Compartment** | Your root compartment |
| **Availability Domain** | Pick any AD (AD-1, AD-2, or AD-3) |

### 4.3 Image and Shape

#### Change Image:
1. Click **"Change Image"**
2. Select **Oracle Linux** → **Oracle Linux 9**
3. Click **"Select Image"**

#### Change Shape:
1. Click **"Change Shape"**
2. Select **"Ampere"** (ARM processor - Always Free!)
3. Check **"VM.Standard.A1.Flex"**
4. Configure resources:

| Resource | Recommended (Free) |
|----------|-------------------|
| **Number of OCPUs** | `2` (can use up to 4 free) |
| **Amount of memory (GB)** | `12` (can use up to 24 free) |

5. Click **"Select Shape"**

### 4.4 Networking (IMPORTANT!)

#### Primary VNIC:

| Field | What to Select |
|-------|----------------|
| **VNIC name** | `helloworld` |
| **Virtual cloud network** | Select **"helloworld-vcn"** (the one you created) |

#### Subnet:

**Option A - Use existing subnet (Recommended):**
1. Select **"Select existing subnet"**
2. Choose **"Public Subnet-helloworld-vcn"**

**Option B - If no subnets appear:**
1. Select **"Create new public subnet"**
2. Enter:
   - **New subnet name:** `helloworld-public`
   - **CIDR block:** `10.0.0.0/24`

#### IP Addresses:

| Field | What to Do |
|-------|------------|
| **Private IPv4 address** | Select **"Automatically assign"** |
| **Public IPv4 address** | ✅ **CHECK THIS BOX** (Critical!) |
| **IPv6 address** | ✅ Check if available (optional) |

⚠️ **If you can't check "Public IPv4"**: Make sure you selected a **PUBLIC** subnet, not private!

### 4.5 SSH Keys (VERY IMPORTANT!)

1. Select **"Generate a key pair for me"**
2. Click **"Download private key"** → **SAVE THIS FILE!**
   - File name will be like: `ssh-key-2026-01-16.key`
3. Click **"Download public key"** → Save this too
   - File name will be like: `ssh-key-2026-01-16.key.pub`

⚠️ **Oracle will NOT show these keys again!** Save them somewhere safe!

### 4.6 Boot Volume

Leave all defaults (46.6 GB is fine and free)

### 4.7 Management - Auto Setup (RECOMMENDED!)

This automatically installs HelloWorld when your VM boots!

1. Scroll down and click **"Show advanced options"**
2. Go to **"Management"** tab
3. Find **"Cloud-init script"**
4. Select **"Choose cloud-init script file"** or **"Paste cloud-init script"**
5. Download and upload this file: [cloud-init.yaml](https://helloworld-tv5t.onrender.com/cloud-init.yaml)

   **Or paste the script directly:** Go to https://raw.githubusercontent.com/dokabi-recon67/helloworld/main/scripts/cloud-init.yaml and copy all text.

6. **Instance metadata service:** Leave as default (don't check "Require authorization header")

7. **Tagging (optional):**
   - Key: `project`
   - Value: `helloworld`

### 4.8 Create the Instance

1. Review all your settings
2. Click **"Create"** (blue button at bottom)
3. Wait 2-5 minutes for instance to show **"Running"**

---

## Step 5: Get Your Server's IP Address

1. Once your instance shows **"Running"**, look at the instance details page
2. Find **"Public IP address"** in the **"Instance access"** section
3. Copy this IP address (example: `129.153.xxx.xxx`)
4. Save it somewhere - you'll need it for the client!

---

## Step 6: Wait for Auto-Setup (If You Used Cloud-Init)

If you uploaded the cloud-init script, your server is setting itself up automatically!

Wait about **5 minutes** after the instance is "Running" for everything to install.

**Skip to Step 8** if you used cloud-init.

---

## Step 7: Manual Setup (Only If You Didn't Use Cloud-Init)

If you didn't use the cloud-init script, install manually:

### 7.1 Connect to Your Server

**On Windows (PowerShell):**
```powershell
# Navigate to where you saved your key
cd Downloads

# Fix key permissions
icacls "ssh-key-2026-01-16.key" /inheritance:r /grant:r "$($env:USERNAME):R"

# Connect (replace with YOUR IP)
ssh -i "ssh-key-2026-01-16.key" opc@YOUR_PUBLIC_IP
```

**On Mac/Linux (Terminal):**
```bash
# Fix key permissions
chmod 400 ~/Downloads/ssh-key-2026-01-16.key

# Connect (replace with YOUR IP)
ssh -i ~/Downloads/ssh-key-2026-01-16.key opc@YOUR_PUBLIC_IP
```

### 7.2 Run the Installer

Once connected, run this single command:
```bash
curl -sSL https://raw.githubusercontent.com/dokabi-recon67/helloworld/main/scripts/install_server.sh | sudo bash
```

Wait for it to complete (about 2-3 minutes).

---

## Step 8: Download and Set Up the Client

### 8.1 Download HelloWorld Client

Go to: https://github.com/dokabi-recon67/helloworld/releases

Download:
- **Windows:** `HelloWorld-win64.zip`
- **Mac:** `HelloWorld-macos.zip`

### 8.2 Extract and Run

**Windows:**
1. Right-click the ZIP → **"Extract All"**
2. Open the extracted folder
3. Double-click **`helloworld.exe`**

**Mac:**
1. Double-click the ZIP to extract
2. Open the extracted folder
3. Double-click **`HelloWorld.app`**
4. If blocked: Right-click → Open → Open

### 8.3 Add Your Server

1. In the HelloWorld app, click **"+ ADD"** button
2. Fill in:

| Field | What to Enter |
|-------|---------------|
| **Server Name** | Anything you want (e.g., "My Oracle Server") |
| **Host** | Your VM's public IP address |
| **Port** | `443` |
| **SSH Key** | Click Browse, select your `.key` file |
| **Username** | `opc` (for Oracle Linux) |

3. Click **"Add Server"**

### 8.4 Connect!

1. Select your server from the dropdown
2. Click **"CONNECT"**
3. Wait a few seconds for the connection

---

## Step 9: Verify It's Working

1. Open your web browser
2. Go to: https://api.ipify.org
3. You should see **your VM's IP address**, NOT your home IP!

**If you see your VM's IP:** Congratulations! Everything is working!

**If you see your home IP:** Make sure "Full Tunnel Mode" is enabled in the app.

---

## Server Commands (Optional)

SSH into your server and run these commands:

```bash
# Check status of everything
helloworld-status

# Get a new Tor IP (when using Tor mode)
helloworld-rotate

# View setup log
cat /var/log/helloworld-setup.log
```

---

## Troubleshooting

### "Connection refused" or timeout
- Check Security List has port 443 open (Step 3)
- Verify instance is **Running**
- Make sure you're using the correct public IP
- Wait 5 minutes if you just created the instance

### "Permission denied" SSH error
- Make sure key file permissions are set correctly
- Windows: Run the `icacls` command shown above
- Mac/Linux: Run `chmod 400 your-key.key`
- Verify you're using `opc` as username (not `root`)

### Can't check "Public IPv4 address"
- You selected a private subnet
- Go back to Networking and select a PUBLIC subnet

### Instance won't create / "Out of capacity"
- Try a different Availability Domain (AD-1, AD-2, or AD-3)
- A1.Flex shape has limited availability
- Try `VM.Standard.E2.1.Micro` shape instead (also free, x86)

### Cloud-init didn't run
- Check the log: `sudo cat /var/log/cloud-init-output.log`
- Run manual install (Step 7)

### App says "Connected" but IP didn't change
- Enable **"Full Tunnel Mode"** toggle in the app
- Restart the app and reconnect

---

## Optional: Reserve Your IP Address

By default, your public IP may change if you stop/restart the instance. To keep it permanent:

1. Go to **Networking** → **IP Management** → **Reserved Public IPs**
2. Click **"Reserve Public IP Address"**
3. Enter a name (e.g., `helloworld-ip`)
4. Click **"Reserve"**
5. Go to your instance → **Attached VNICs** → Click the VNIC
6. Click **IPv4 Addresses** → Click the ⋮ menu → **Edit**
7. Change to your reserved IP
8. Save

Now your IP stays the same forever!

---

## Oracle Cloud Free Tier Limits

| Resource | Free Amount |
|----------|-------------|
| A1 Flex Compute | 4 OCPUs, 24 GB RAM total |
| E2.1 Micro | 2 instances |
| Boot Volume | 200 GB total |
| Outbound Data | 10 TB/month |

HelloWorld works well within these limits!

---

## Need Help?

- **GitHub Issues:** https://github.com/dokabi-recon67/helloworld/issues
- **Oracle Cloud Docs:** https://docs.oracle.com/en-us/iaas/Content/home.htm
