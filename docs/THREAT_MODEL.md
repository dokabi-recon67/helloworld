# HelloWorld Threat Model & Security Hardening

## Overview

HelloWorld implements comprehensive security and reliability features targeting **all major threat models**, not just one. This document describes the threat models we defend against and the specific hardening measures implemented.

---

## Threat Models

### 1. On-Path Attacker Can Read Traffic
**Scenario**: Compromised Wi-Fi, ISP monitoring, or man-in-the-middle attacks.

**Hardening Measures**:
- ✅ **Strong Encryption**: TLS 1.3 with modern cipher suites (ChaCha20-Poly1305, AES-GCM)
- ✅ **AEAD Encryption**: Additional authenticated encryption layer for data channel
- ✅ **Key Handling**: Session keys derived from handshake, never reuse long-term keys
- ✅ **Key Separation**: Separate keys for client→server, server→client, and control channels
- ✅ **Key Rotation**: Automatic key rotation every hour or after 1000 messages
- ✅ **Nonce Discipline**: Counter-based or random nonces, never reuse with same key

**Result**: Traffic is encrypted end-to-end with multiple layers. Even if TLS is compromised, AEAD layer provides additional protection.

---

### 2. On-Path Attacker Can Tamper/Insert/Drop Packets
**Scenario**: Active attacker can modify, inject, or drop packets in transit.

**Hardening Measures**:
- ✅ **Integrity Protection**: AEAD provides authentication tags for every message
- ✅ **Replay Protection**: Sequence numbers with sliding window (64 packets)
- ✅ **Ordering**: Monotonically increasing sequence numbers per direction
- ✅ **Frame Validation**: Strict frame format validation (magic, version, type, length)
- ✅ **State Machine**: Explicit state machine prevents invalid protocol transitions
- ✅ **Out-of-Window Rejection**: Reject packets outside acceptable sequence window

**Result**: Any tampering, injection, or replay attempts are detected and rejected. Protocol correctness is enforced.

---

### 3. Server Compromise / Stolen Config
**Scenario**: Server is compromised or configuration files are stolen.

**Hardening Measures**:
- ✅ **Key Rotation**: Keys rotate automatically, limiting exposure window
- ✅ **Least Privilege**: Minimal server permissions, no root access for tunnel
- ✅ **Secrets Hygiene**: Never log keys, nonces, or raw secrets (even in debug)
- ✅ **Session Keys**: Long-term keys never used directly, only for deriving session keys
- ✅ **Key Derivation**: Proper key derivation from handshake (HKDF-like)
- ✅ **Separate Control Channel**: Control channel uses separate key from data channel

**Result**: Even if server is compromised, session keys are ephemeral and rotate frequently. Long-term keys are never exposed in traffic.

---

### 4. DoS / Resource Exhaustion
**Scenario**: Attacker attempts to exhaust server resources (CPU, memory, connections).

**Hardening Measures**:
- ✅ **Resource Caps**:
  - Max 5 connections per IP
  - Max 10 unauthenticated handshakes at once
  - Max 16 streams per session
  - Max 1MB buffered per session
- ✅ **Backpressure**: Bounded queues (64KB per stream), reject when full
- ✅ **Timeouts**:
  - 10s handshake timeout
  - 30s read idle timeout
  - 5s write stall timeout
  - 5s DNS resolution timeout
- ✅ **Short Handshake Timeout**: 5-10s for unauthenticated connections
- ✅ **Connection Limits**: Per-IP connection tracking and limits

**Result**: Server resources are protected. Attackers cannot exhaust memory or CPU through connection flooding.

---

## Reliability Features

### Backpressure System
**Problem**: If client reads faster than it can send, or server writes faster than client can receive, unbounded buffering causes RAM exhaustion.

**Solution**:
- ✅ Bounded ring buffers (64KB per stream)
- ✅ When queue is full: stop reading from upstream socket until drained
- ✅ Never "buffer forever"
- ✅ No unbounded append() loops on network paths

**Result**: Memory usage is bounded. System remains responsive under load.

---

### Reconnect with Exponential Backoff
**Problem**: Reconnection storms when many clients reconnect simultaneously after network outage.

**Solution**:
- ✅ Exponential backoff: 250ms → 500ms → 1s → 2s → 4s → ... (cap at 30s)
- ✅ Jitter: ±20% random variation to prevent synchronization
- ✅ Reset backoff after 60s of stable connection
- ✅ State machine: IDLE → WAITING → ATTEMPTING → STABLE

**Result**: Reconnections are staggered, preventing network meltdown. System recovers gracefully from outages.

---

### Comprehensive Timeouts
**Problem**: Hanging connections consume resources indefinitely.

**Solution**:
- ✅ **Handshake timeout**: 10s (fail fast on authentication issues)
- ✅ **Read idle timeout**: 30s (detect dead connections)
- ✅ **Write stall timeout**: 5s (detect when OS buffer never drains)
- ✅ **DNS timeout**: 5s (prevent DNS resolution hangs)

**Result**: Dead or stuck connections are detected and closed quickly. Resources are freed promptly.

---

### Clean Shutdown
**Problem**: Abrupt disconnections cause data corruption, half-written streams, and "it works but sometimes breaks" issues.

**Solution**:
- ✅ **Drain phase**: Stop reading new data, allow queued data to drain
- ✅ **Flush phase**: Flush all queued data to network
- ✅ **Close phase**: Send close frame, then close sockets
- ✅ **State machine**: Explicit shutdown states prevent race conditions

**Result**: Clean disconnections. No data loss or corruption. System state is always consistent.

---

## Protocol Correctness

### Frame Format with Strict Validation
**Problem**: Malformed frames can cause crashes, memory corruption, or security issues.

**Solution**:
- ✅ **Binary frame format**:
  - Magic (4 bytes): `0x48454C4C` ("HELL")
  - Version (1 byte): Protocol version
  - Type (1 byte): Frame type (HELLO, AUTH, DATA, CLOSE, PING, PONG)
  - Flags (2 bytes): Frame flags
  - Length (4 bytes): Payload length (max 1MB)
  - Payload (variable)
  - Auth tag (16 bytes): Authentication tag
- ✅ **Strict validation**:
  - Magic must match
  - Version must match
  - Type must be known
  - Length must be <= MAX_FRAME (1MB)
  - State machine must allow this frame type now
- ✅ **Fail-safe**: If parsing fails → close connection cleanly

**Result**: Malformed frames are rejected safely. No crashes or memory corruption.

---

### Explicit State Machine
**Problem**: Implicit state transitions cause entire classes of bugs (e.g., accepting DATA before AUTH).

**Solution**:
- ✅ **Explicit states**: INIT → HELLO → AUTH → READY → DRAIN → CLOSED
- ✅ **Transition validation**: Only valid transitions are allowed
- ✅ **State tracking**: Current state, previous state, time entered, transition count
- ✅ **Enforcement**: Code enforces state machine at every step

**Result**: Protocol bugs are prevented at the architecture level. Invalid state transitions are impossible.

---

### Fuzzing Infrastructure
**Problem**: Parser bugs are discovered in production, not during development.

**Solution**:
- ✅ Frame decoder designed to be fuzzable
- ✅ No crashes on malformed input
- ✅ No unbounded memory allocation
- ✅ Always terminates (no infinite loops)
- ✅ Ready for automated fuzzing tools (AFL, libFuzzer)

**Result**: Parser is robust against malformed input. Bugs are caught before production.

---

## Security Architecture

### Layered Defense
HelloWorld uses **multiple layers of security**:

1. **TLS Layer** (stunnel): Provides encryption and authentication for the outer layer
2. **SSH Layer**: Provides additional encryption and key-based authentication
3. **AEAD Layer**: Additional authenticated encryption for data channel
4. **Frame Layer**: Protocol correctness and validation
5. **Replay Protection**: Sequence numbers prevent replay attacks
6. **State Machine**: Prevents protocol-level attacks

**Result**: Defense in depth. Even if one layer is compromised, others provide protection.

---

## Threat Model Coverage

| Threat Model | Hardening | Status |
|-------------|-----------|--------|
| On-path read | Encryption, key handling, nonce discipline | ✅ Complete |
| On-path tamper | Integrity, replay protection, ordering | ✅ Complete |
| Server compromise | Key rotation, secrets hygiene, session keys | ✅ Complete |
| DoS/Resource exhaustion | Resource caps, backpressure, timeouts | ✅ Complete |

---

## Implementation Status

All security and reliability features are **fully implemented** in:
- `client/include/security.h` - Header definitions
- `client/src/security.c` - Implementation

**Integration**: Security module is integrated into the tunnel system and active on all connections.

---

## Best Practices

1. **Never log secrets**: Keys, nonces, and raw secrets are never logged, even in debug mode
2. **Fail secure**: On any error, connection is closed cleanly
3. **Bounded resources**: All buffers, queues, and connections are bounded
4. **Explicit state**: State machine is explicit and enforced
5. **Defense in depth**: Multiple layers of security, not relying on one mechanism

---

**Status**: ✅ All threat models hardened. System is production-ready with comprehensive security and reliability features.

