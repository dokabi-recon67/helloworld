# HelloWorld Server Setup - Complete Oracle Cloud Guide

This guide walks you through setting up your own private tunnel server on Oracle Cloud's **Always Free** tier. No credit card charges ever.

---

## Step 1: Create Oracle Cloud Account

1. Go to [cloud.oracle.com](https://cloud.oracle.com)
2. Click **Sign Up**
3. Enter your email and country
4. Verify your email
5. Fill in your details (name, address)
6. Add a credit card (required for verification, but **you will NOT be charged** for Always Free resources)
7. Complete phone verification
8. Wait for account activation (usually instant, sometimes up to 24 hours)

---

## Step 2: Create a Virtual Cloud Network (VCN)

Before creating your VM, you need a network.

1. Log into Oracle Cloud Console
2. Click the hamburger menu (top left)
3. Go to **Networking** > **Virtual Cloud Networks**
4. Click **Start VCN Wizard**
5. Select **Create VCN with Internet Connectivity**
6. Click **Start VCN Wizard**
7. Configure:

| Setting | Value |
|---------|-------|
| **VCN Name** | `helloworld-vcn` |
| **Compartment** | Your root compartment |
| **IPv4 CIDR Block** | `10.0.0.0/16` (default, leave as is) |
| **Use DNS hostnames in this VCN** | **Checked** (recommended) |

### Adding IPv6 Support (Optional but Recommended)

8. Scroll down to **Add IPv6 Prefixes**
9. Click **Add IPv6 Prefix**
10. Configure:

| Setting | Value |
|---------|-------|
| **Prefix type** | **Oracle allocated IPv6 /56 prefix** |
| **IPv6 prefix** | Leave blank (auto-assigned) |

This gives you a `/56` block with ~4 sextillion IPv6 addresses (more than enough!).

11. Click **Next**, then **Create**
12. Wait for it to complete

**Note on CIDR:** The default `10.0.0.0/16` gives you 65,536 private IPv4 addresses. The `/56` IPv6 prefix is also free and future-proofs your setup.

---

## Step 3: Configure Security List (Open Port 443)

This allows your tunnel traffic through.

1. In your new VCN, click **Public Subnet-helloworld-vcn**
2. Click the **Default Security List**
3. Click **Add Ingress Rules**

### Rule 1: IPv4 TCP 443

| Setting | Value |
|---------|-------|
| Stateless | No (unchecked) |
| Source Type | CIDR |
| Source CIDR | `0.0.0.0/0` |
| IP Protocol | TCP |
| Destination Port Range | `443` |
| Description | HelloWorld Tunnel IPv4 |

Click **+ Another Ingress Rule** to add more rules:

### Rule 2: IPv6 TCP 443

| Setting | Value |
|---------|-------|
| Stateless | No (unchecked) |
| Source Type | CIDR |
| Source CIDR | `::/0` |
| IP Protocol | TCP |
| Destination Port Range | `443` |
| Description | HelloWorld Tunnel IPv6 |

### Rule 3: IPv4 UDP 443 (Optional)

| Setting | Value |
|---------|-------|
| Source Type | CIDR |
| Source CIDR | `0.0.0.0/0` |
| IP Protocol | UDP |
| Destination Port Range | `443` |
| Description | HelloWorld UDP IPv4 |

### Rule 4: IPv6 UDP 443 (Optional)

| Setting | Value |
|---------|-------|
| Source Type | CIDR |
| Source CIDR | `::/0` |
| IP Protocol | UDP |
| Destination Port Range | `443` |
| Description | HelloWorld UDP IPv6 |

4. Click **Add Ingress Rules**

**Summary of rules to add:**
- `0.0.0.0/0` TCP 443 (IPv4)
- `::/0` TCP 443 (IPv6)
- `0.0.0.0/0` UDP 443 (IPv4, optional)
- `::/0` UDP 443 (IPv6, optional)

---

## Step 4: Create Your Compute Instance

1. Click hamburger menu > **Compute** > **Instances**
2. Click **Create Instance**

### Basic Settings

| Setting | Recommended Value |
|---------|-------------------|
| **Name** | `helloworld-server` (or anything you like) |
| **Compartment** | Your root compartment |
| **Availability Domain** | Any AD that shows "Always Free-eligible" |

### Image and Shape

Click **Change Image**:
- Select **Oracle Linux 9** (recommended) or **Ubuntu 22.04**
- Click **Select Image**

Click **Change Shape**:
- **Instance Type:** Virtual Machine
- **Shape Series:** Ampere (ARM-based, Always Free)
- Select **VM.Standard.A1.Flex**
- Configure OCPUs and Memory:

| Resource | Free Tier Limit | Recommended |
|----------|-----------------|-------------|
| OCPUs | Up to 4 | **2** |
| Memory | Up to 24 GB | **12 GB** |

Click **Select Shape**

### Networking

| Setting | Value |
|---------|-------|
| Virtual Cloud Network | `helloworld-vcn` (the one you created) |
| Subnet | Public Subnet |
| Public IPv4 Address | **Assign a public IPv4 address** (IMPORTANT!) |
| Public IPv6 Address | **Assign a public IPv6 address** (if you added IPv6 to VCN) |

### SSH Keys (IMPORTANT!)

Select **Generate a key pair**:
1. Click **Save Private Key** - downloads a `.key` file
2. Click **Save Public Key** - downloads a `.pub` file
3. **SAVE THESE FILES SECURELY** - you need them to connect!

Or if you have existing keys, select **Upload public key files** or **Paste public keys**

### Boot Volume

Leave defaults (46.6 GB is fine and free)

### Management (OPTIONAL - Auto Setup)

This is optional but **highly recommended** - it auto-installs HelloWorld when your VM boots!

1. Expand **Management** section (click "Show advanced options" if needed)

2. **Instance metadata service:**
   - Leave as default (IMDSv1 and IMDSv2)
   - Don't check "Require authorization header" unless you know what you're doing

3. **Initialization script (Cloud-init):**
   - Select **Paste cloud-init script**
   - Copy the entire contents from: [cloud-init.yaml](https://raw.githubusercontent.com/dokabi-recon67/helloworld/main/scripts/cloud-init.yaml)
   - Paste it in the text box

This script automatically:
- Updates the system
- Installs stunnel, SSH, Tor
- Generates TLS certificates
- Configures firewall (opens port 443)
- Starts all services
- Sets up Tor for optional anonymous mode

4. **Tagging (Optional):**
   - Key: `project`
   - Value: `helloworld`

### Create

Click **Create** and wait 2-5 minutes for your instance to be **Running**

**If you used cloud-init:** Your server is already set up! Skip to Step 6 (connecting).

**If you didn't use cloud-init:** You'll need to run the install script manually in Step 7.

---

## Step 5: Note Your Public IP

Once your instance shows **Running**:
1. Look for **Public IP Address** on the instance details page
2. Copy this IP - you'll need it for the client!

Example: `129.153.xxx.xxx`

---

## Step 6: Connect to Your Server

### Windows (PowerShell)

```powershell
# Navigate to where you saved your key
cd Downloads

# Set correct permissions on key file
icacls "ssh-key-*.key" /inheritance:r /grant:r "$($env:USERNAME):R"

# Connect (replace IP and key filename)
ssh -i "ssh-key-2024-01-15.key" opc@YOUR_PUBLIC_IP
```

### Mac/Linux (Terminal)

```bash
# Set correct permissions
chmod 400 ~/Downloads/ssh-key-*.key

# Connect (replace IP and key filename)  
ssh -i ~/Downloads/ssh-key-*.key opc@YOUR_PUBLIC_IP
```

**Note:** For Oracle Linux, username is `opc`. For Ubuntu, it's `ubuntu`.

---

## Step 7: Install HelloWorld Server

Once connected via SSH, run this single command:

```bash
curl -sSL https://raw.githubusercontent.com/dokabi-recon67/helloworld/main/scripts/install_server.sh | sudo bash
```

This automatically:
- Installs all dependencies
- Configures stunnel for TLS wrapping
- Sets up SSH tunnel endpoint
- Configures firewall rules
- Starts all services

---

## Step 8: Configure HelloWorld Client

On your PC/Mac:

1. Open HelloWorld app
2. Click **+ ADD SERVER**
3. Enter:
   - **Name:** Anything (e.g., "Oracle Server")
   - **Host:** Your VM's public IP
   - **Port:** 443
   - **SSH Key:** Browse to your `.key` file
   - **Username:** `opc` (Oracle Linux) or `ubuntu` (Ubuntu)
4. Click **Add Server**
5. Click **CONNECT**

---

## Step 9: Verify It Works

1. Open your browser
2. Go to [api.ipify.org](https://api.ipify.org)
3. You should see your **Oracle VM's IP**, not your home IP!

---

## Troubleshooting

### "Connection refused" or timeout
- Check Security List has port 443 open (Step 3)
- Verify instance is **Running**
- Check you're using the correct public IP

### "Permission denied" SSH error
- Make sure key file permissions are set (chmod 400)
- Verify you're using the correct username (`opc` or `ubuntu`)
- Confirm you're using the private key (`.key`), not public (`.pub`)

### Instance won't create
- Try a different Availability Domain
- A1.Flex is limited - if unavailable, try `VM.Standard.E2.1.Micro` (x86, also free)

### "Host key verification failed"
```bash
ssh-keygen -R YOUR_SERVER_IP
```
Then try connecting again.

---

## Oracle Cloud Always Free Limits

| Resource | Free Amount |
|----------|-------------|
| A1 Flex Compute | 4 OCPUs, 24 GB RAM total |
| E2.1 Micro | 2 instances |
| Boot Volume | 200 GB total |
| Block Volume | 200 GB total |
| Object Storage | 20 GB |
| Outbound Data | 10 TB/month |

You can run HelloWorld well within these limits!

---

## Security Best Practices

1. **Never share your private SSH key**
2. **Don't disable the firewall** on your VM
3. **Keep your VM updated:**
   ```bash
   sudo dnf update -y  # Oracle Linux
   sudo apt update && sudo apt upgrade -y  # Ubuntu
   ```
4. **Only open ports you need** (443 is all HelloWorld needs)

---

## Optional: Reserve Your IP (Recommended)

By default, if you stop/start your instance, the IP may change. To keep it:

1. Go to **Networking** > **IP Management** > **Reserved Public IPs**
2. Click **Reserve Public IP Address**
3. Name it (e.g., `helloworld-ip`)
4. Click **Reserve**
5. Go back to your instance
6. Under **Attached VNICs**, click the VNIC
7. Click **IPv4 Addresses**
8. Click the three dots menu > **Edit**
9. Select your reserved IP
10. Save

Now your IP stays the same forever!

---

## Need Help?

- GitHub Issues: https://github.com/dokabi-recon67/helloworld/issues
- Oracle Cloud Docs: https://docs.oracle.com/en-us/iaas/Content/home.htm
