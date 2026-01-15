# Stealth Tunnel over TLS  
### Self-Hosted, DPI-Resistant, Open-Source Encrypted Tunnel

---

## 1. Purpose & Motivation

Many traditional VPN protocols (WireGuard, OpenVPN, IPSec) are increasingly blocked or throttled by ISPs using Deep Packet Inspection (DPI), especially in high-restriction regions.

This project provides a **self-hosted alternative** that:

- Avoids VPN-specific fingerprints
- Uses **standard HTTPS (TLS on TCP 443)**
- Relies only on **battle-tested, audited tools**
- Gives the user **full ownership and control**
- Can be deployed on **free cloud tiers**
- Is controlled by a **cross-platform GUI**

This is **not a commercial VPN**, not a mass circumvention service, and not anonymity software.  
It is a **personal, infrastructure-level encrypted tunnel**.

---

## 2. Core Design Philosophy

### ❌ What We Do NOT Do
- Write custom cryptography
- Create new tunneling protocols
- Use UDP-based transports
- Rely on third-party VPN providers
- Obfuscate traffic in unsafe ways

### ✅ What We Do Instead
- Compose **existing, secure primitives**
- Use **TLS to blend with HTTPS**
- Use **SSH for authentication and transport**
- Keep the system simple, inspectable, and reproducible

Security comes from **correct composition**, not secrecy.

---

## 3. High-Level Architecture

```
Client (Windows / macOS)
    |
    | TLS (TCP 443) – looks like HTTPS
    |
Cloud VM (Oracle / GCP)
    |
    | NAT + IP forwarding
    |
Public Internet
```

---

## 4. Transport Stack

- **TLS (stunnel)** – camouflage as HTTPS  
- **SSH** – encrypted authenticated transport  
- **Routing layer** – full tunnel or proxy mode  

---

## 5. Why This Works Against DPI

- TCP port 443
- Real TLS handshake
- No VPN protocol markers
- No UDP traffic
- No fixed signatures

---

## 6. Cloud Platform (Initial)

### Oracle Cloud Always Free
- Ampere A1 ARM
- Up to **18,000 GB-hours/month**
- Suitable for personal and light multi-client use

---

## 7. Server-Side Components

- stunnel
- OpenSSH
- IP forwarding
- iptables / nftables NAT

Only **443/TCP** is exposed publicly.

---

## 8. Client-Side Components

- Python 3.10+
- stunnel client
- OpenSSH client
- Cross-platform GUI (Windows + macOS)

---

## 9. Traffic Modes

### Full Tunnel
- System-wide routing
- VM IP becomes public IP
- Requires admin privileges

### Proxy Mode
- SOCKS proxy
- App-level routing
- Easier setup

---

## 10. Python GUI

The GUI:
- Starts/stops tunnel
- Selects server profiles
- Displays status, IP, latency
- Implements optional kill-switch

The GUI **does not implement cryptography**.

---

## 11. Security Model

- TLS encryption
- SSH key authentication
- No passwords
- Minimal attack surface

---

## 12. Repository Structure

```
stealth-tunnel/
├── server/
├── client/
├── docs/
├── README.md
└── LICENSE
```

---

## 13. Legal Scope

This project is infrastructure tooling only.  
Users are responsible for compliance with local laws.

---

## 14. License

MIT or Apache-2.0 (recommended)

---

## Status

Design complete – ready for implementation
