# Security Policy

## Reporting Vulnerabilities

If you discover a security vulnerability, please report it privately by opening a GitHub issue with the title "SECURITY" (do not include exploit details in public issues).

## What This Project Does NOT Store

This repository contains **no credentials, keys, or personal data**. All sensitive information is generated locally when you run the software:

| Generated Locally | Location | Never Committed |
|------------------|----------|-----------------|
| SSH Keys | Your computer | Your private keys stay on your machine |
| TLS Certificates | Your server | Auto-generated during setup |
| Server IP | Your server | You enter this in the app |
| Config files | Your devices | Contains your server info |

## Files That Should NEVER Be Committed

If you fork this repo, ensure these are never committed:

```
# SSH Keys
*.pem
*.key
*.ppk
id_rsa
id_ed25519
id_*

# Certificates
*.crt
*.cer
*.p12

# Config with credentials
config.json
config.txt
servers.json
profiles.json
*.conf

# Environment
.env
.env.*
secrets/
```

The `.gitignore` in this repo blocks these by default.

## Before You Push

Always run this before pushing to a public fork:

```bash
# Check for potential secrets
git diff --cached --name-only | xargs grep -l -E "(BEGIN (RSA|DSA|EC|OPENSSH) PRIVATE KEY|password|secret|api_key|token)" 2>/dev/null

# Check for IP addresses (review manually - some may be examples)
git diff --cached | grep -E "\b\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}\b"
```

## Security Best Practices

### For Users

1. **Keep your SSH private key safe** - Never share it, never commit it
2. **Use a dedicated key** - Generate a new key for HelloWorld
3. **Update regularly** - Keep your server and client updated
4. **Firewall properly** - Only open port 443, nothing else
5. **Use key-only SSH** - Password authentication is disabled by our installer

### For Contributors

1. **No hardcoded IPs** - Use placeholders like `YOUR_SERVER_IP`
2. **No real credentials** - Use example values
3. **No personal paths** - Use generic paths like `/path/to/key`
4. **Review diffs** - Check every commit for accidental secrets

## Encryption Used

| Layer | Protocol | Purpose |
|-------|----------|---------|
| Outer | TLS 1.3 | DPI resistance, looks like HTTPS |
| Inner | SSH | Authentication, encryption |
| Optional | Tor | Anonymous exit traffic |

All cryptography uses well-audited, battle-tested libraries:
- **stunnel** for TLS
- **OpenSSH** for SSH transport
- **Tor** for onion routing (optional)

No custom cryptography is implemented in this project.

## Known Limitations

- Traffic patterns may still be analyzable (timing, volume)
- TLS fingerprinting may identify stunnel
- Not designed for anonymity against state-level adversaries
- Server IP is visible to anyone who monitors your connection

For strong anonymity, use Tor Browser directly.

