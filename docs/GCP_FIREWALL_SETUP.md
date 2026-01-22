# Google Cloud Platform Firewall Setup

## Quick Setup (GCP Console - No Installation Needed)

**Easiest method - no command line required:**

1. Go to: https://console.cloud.google.com/compute/firewalls
2. Click **"CREATE FIREWALL RULE"**
3. Fill in:
   - **Name:** `allow-helloworld-443`
   - **Direction:** `Ingress` (incoming)
   - **Action:** `Allow`
   - **Targets:** `All instances in the network`
   - **Source IP ranges:** `0.0.0.0/0` (all IPs)
   - **Protocols and ports:** Check `TCP` and enter `443`
4. Click **"CREATE"**

Done! Your firewall is now configured.

---

## Using gcloud CLI (Command Line)

### Step 1: Install gcloud CLI on Windows

**Option A: Using winget (Recommended)**
```powershell
winget install Google.CloudSDK
```

**Option B: Download Installer**
1. Go to: https://cloud.google.com/sdk/docs/install
2. Download the Windows installer
3. Run the installer and follow prompts

**Option C: Using Chocolatey**
```powershell
choco install gcloudsdk
```

### Step 2: Restart PowerShell

Close and reopen PowerShell after installation.

### Step 3: Initialize gcloud

```powershell
gcloud init
```

Follow the prompts:
- Select your Google account
- Choose your project
- Select default region (optional)

### Step 4: Authenticate (if needed)

```powershell
gcloud auth login
```

This opens a browser for authentication.

### Step 5: Set Your Project (if not set)

```powershell
gcloud config set project YOUR_PROJECT_ID
```

To find your project ID:
- Go to: https://console.cloud.google.com
- Look at the project dropdown at the top

### Step 6: Create Firewall Rule

```powershell
gcloud compute firewall-rules create allow-helloworld-443 --allow tcp:443 --source-ranges 0.0.0.0/0 --description "HelloWorld tunnel port"
```

### Step 7: Verify

```powershell
gcloud compute firewall-rules list --filter="name~helloworld"
```

You should see `allow-helloworld-443` in the list.

---

## Troubleshooting

### "gcloud: command not found"
- Make sure you restarted PowerShell after installation
- Check if gcloud is in your PATH: `$env:PATH`
- Try: `refreshenv` (if using Chocolatey)

### "Request had insufficient authentication scopes"
- This happens when running on the VM
- **Solution:** Run the command on your PC, not the VM

### "Could not fetch resource"
- Make sure you're authenticated: `gcloud auth list`
- Make sure project is set: `gcloud config get-value project`
- Try: `gcloud auth login` again

### "Permission denied"
- Make sure you have "Compute Admin" or "Security Admin" role
- Check IAM permissions in GCP Console

---

## Alternative: Use GCP Console

If you don't want to install gcloud CLI, just use the web console:

1. Go to: https://console.cloud.google.com/compute/firewalls
2. Click **"CREATE FIREWALL RULE"**
3. Configure as shown above
4. Click **"CREATE"**

No installation needed!

