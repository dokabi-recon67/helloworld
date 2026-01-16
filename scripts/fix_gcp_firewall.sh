#!/bin/bash
# Fix Google Cloud Platform Firewall for HelloWorld

echo "========================================"
echo "   Google Cloud Firewall Fix"
echo "========================================"
echo ""

# Check if gcloud is installed
if ! command -v gcloud &> /dev/null; then
    echo "⚠️  gcloud CLI not found"
    echo ""
    echo "To fix firewall via GCP Console:"
    echo "1. Go to: https://console.cloud.google.com/compute/firewalls"
    echo "2. Click 'CREATE FIREWALL RULE'"
    echo "3. Name: allow-helloworld-443"
    echo "4. Direction: Ingress"
    echo "5. Action: Allow"
    echo "6. Targets: All instances in the network"
    echo "7. Source IP ranges: 0.0.0.0/0"
    echo "8. Protocols and ports: TCP 443"
    echo "9. Click CREATE"
    echo ""
    exit 1
fi

# Get current project
PROJECT=$(gcloud config get-value project 2>/dev/null)
if [ -z "$PROJECT" ]; then
    echo "❌ No GCP project configured"
    echo "Run: gcloud config set project YOUR_PROJECT_ID"
    exit 1
fi

echo "Project: $PROJECT"
echo ""

# Check existing rules
echo "Checking existing firewall rules..."
EXISTING=$(gcloud compute firewall-rules list --filter="name~allow-https OR name~allow-443 OR name~helloworld" --format="value(name)" 2>/dev/null)

if [ -n "$EXISTING" ]; then
    echo "Found existing rules:"
    echo "$EXISTING"
    echo ""
    echo "✅ Firewall rule may already exist"
    echo "Checking if port 443 is allowed..."
    
    # Check if any rule allows 443
    if gcloud compute firewall-rules list --filter="allowed.ports:443" --format="value(name)" 2>/dev/null | grep -q .; then
        echo "✅ Port 443 appears to be allowed"
    else
        echo "⚠️  Port 443 may not be allowed"
    fi
else
    echo "⚠️  No firewall rules found for port 443"
    echo ""
    echo "Creating firewall rule..."
    
    # Create firewall rule
    gcloud compute firewall-rules create allow-helloworld-443 \
        --allow tcp:443 \
        --source-ranges 0.0.0.0/0 \
        --description "Allow HelloWorld stunnel on port 443" \
        --direction INGRESS \
        2>&1
    
    if [ $? -eq 0 ]; then
        echo ""
        echo "✅ Firewall rule created successfully!"
    else
        echo ""
        echo "❌ Failed to create firewall rule"
        echo "You may need to create it manually in GCP Console"
    fi
fi

echo ""
echo "========================================"
echo "   Next Steps"
echo "========================================"
echo ""
echo "1. Verify firewall rule exists:"
echo "   gcloud compute firewall-rules list --filter='name~443'"
echo ""
echo "2. Test connection from client"
echo ""
echo "3. Check server logs for new connections:"
echo "   sudo tail -f /var/log/helloworld-stunnel.log"
echo ""

