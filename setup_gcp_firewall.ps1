# HelloWorld GCP Firewall Setup Script for Windows
# This script installs gcloud CLI and creates the firewall rule

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "   HelloWorld GCP Firewall Setup" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check if gcloud is already installed
$gcloudInstalled = $false
try {
    $null = gcloud version 2>&1
    $gcloudInstalled = $true
    Write-Host "[OK] gcloud CLI is already installed" -ForegroundColor Green
} catch {
    Write-Host "[!] gcloud CLI not found, will install..." -ForegroundColor Yellow
}

# Step 1: Install gcloud if not installed
if (-not $gcloudInstalled) {
    Write-Host ""
    Write-Host "Step 1: Installing gcloud CLI..." -ForegroundColor Cyan
    Write-Host "This may take a few minutes..." -ForegroundColor Yellow
    
    try {
        winget install Google.CloudSDK --accept-package-agreements --accept-source-agreements
        Write-Host "[OK] gcloud CLI installed successfully!" -ForegroundColor Green
        Write-Host ""
        Write-Host "IMPORTANT: Please close and reopen PowerShell, then run this script again." -ForegroundColor Yellow
        Write-Host "Or run: gcloud init" -ForegroundColor Yellow
        exit 0
    } catch {
        Write-Host "[ERROR] Failed to install gcloud CLI" -ForegroundColor Red
        Write-Host "Please install manually from: https://cloud.google.com/sdk/docs/install" -ForegroundColor Yellow
        exit 1
    }
}

# Step 2: Check if gcloud is initialized
Write-Host ""
Write-Host "Step 2: Checking gcloud configuration..." -ForegroundColor Cyan

$project = gcloud config get-value project 2>&1
if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($project)) {
    Write-Host "[!] gcloud is not initialized" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Please run: gcloud init" -ForegroundColor Cyan
    Write-Host "This will:" -ForegroundColor White
    Write-Host "  1. Open a browser for authentication" -ForegroundColor White
    Write-Host "  2. Let you select your GCP project" -ForegroundColor White
    Write-Host ""
    Write-Host "After that, run this script again." -ForegroundColor Yellow
    exit 0
}

Write-Host "[OK] Project: $project" -ForegroundColor Green

# Step 3: Check if firewall rule already exists
Write-Host ""
Write-Host "Step 3: Checking existing firewall rules..." -ForegroundColor Cyan

$existingRule = gcloud compute firewall-rules list --filter="name=allow-helloworld-443" --format="value(name)" 2>&1
if ($existingRule -match "allow-helloworld-443") {
    Write-Host "[OK] Firewall rule 'allow-helloworld-443' already exists!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Your firewall is already configured. You're all set!" -ForegroundColor Green
    exit 0
}

# Step 4: Create firewall rule
Write-Host ""
Write-Host "Step 4: Creating firewall rule..." -ForegroundColor Cyan

$result = gcloud compute firewall-rules create allow-helloworld-443 `
    --allow tcp:443 `
    --source-ranges 0.0.0.0/0 `
    --description "HelloWorld tunnel port" `
    --direction INGRESS 2>&1

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "   SUCCESS!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "Firewall rule created successfully!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Your GCP firewall is now configured to allow:" -ForegroundColor White
    Write-Host "  - Port 443 (TCP) from all IPs (0.0.0.0/0)" -ForegroundColor White
    Write-Host ""
    Write-Host "You can now connect using HelloWorld!" -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "[ERROR] Failed to create firewall rule" -ForegroundColor Red
    Write-Host ""
    Write-Host "Error output:" -ForegroundColor Yellow
    Write-Host $result -ForegroundColor Red
    Write-Host ""
    Write-Host "Alternative: Create it manually in GCP Console:" -ForegroundColor Yellow
    Write-Host "https://console.cloud.google.com/compute/firewalls" -ForegroundColor Cyan
    exit 1
}

