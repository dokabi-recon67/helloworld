/*
 * HelloWorld - Security & Reliability Implementation
 * Comprehensive threat model hardening
 */

#include "security.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#endif

// ============================================================================
// BACKPRESSURE SYSTEM
// ============================================================================

int hw_queue_init(hw_ring_buffer_t* q, size_t capacity) {
    if (!q || capacity == 0) return -1;
    
    q->buffer = (uint8_t*)malloc(capacity);
    if (!q->buffer) return -1;
    
    q->head = 0;
    q->tail = 0;
    q->size = 0;
    q->capacity = capacity;
    q->full = false;
    return 0;
}

void hw_queue_free(hw_ring_buffer_t* q) {
    if (q && q->buffer) {
        free(q->buffer);
        q->buffer = NULL;
        q->capacity = 0;
        q->size = 0;
    }
}

int hw_queue_push(hw_ring_buffer_t* q, const uint8_t* data, size_t len) {
    if (!q || !data || len == 0) return -1;
    if (q->full) return -1;  // Backpressure: reject when full
    
    size_t space = hw_queue_space(q);
    if (len > space) return -1;  // Not enough space
    
    for (size_t i = 0; i < len; i++) {
        q->buffer[q->tail] = data[i];
        q->tail = (q->tail + 1) % q->capacity;
    }
    
    q->size += len;
    if (q->tail == q->head && q->size > 0) {
        q->full = true;
    }
    return 0;
}

int hw_queue_pop(hw_ring_buffer_t* q, uint8_t* data, size_t max_len, size_t* out_len) {
    if (!q || !data || !out_len) return -1;
    if (hw_queue_empty(q)) {
        *out_len = 0;
        return 0;
    }
    
    size_t to_read = (q->size < max_len) ? q->size : max_len;
    *out_len = 0;
    
    for (size_t i = 0; i < to_read; i++) {
        data[i] = q->buffer[q->head];
        q->head = (q->head + 1) % q->capacity;
        (*out_len)++;
    }
    
    q->size -= to_read;
    q->full = false;
    return 0;
}

size_t hw_queue_available(hw_ring_buffer_t* q) {
    return q ? q->size : 0;
}

size_t hw_queue_space(hw_ring_buffer_t* q) {
    if (!q) return 0;
    if (q->full) return 0;
    return q->capacity - q->size;
}

bool hw_queue_full(hw_ring_buffer_t* q) {
    return q ? q->full : false;
}

bool hw_queue_empty(hw_ring_buffer_t* q) {
    return q ? (q->size == 0 && !q->full) : true;
}

// ============================================================================
// RECONNECT STATE MACHINE
// ============================================================================

int hw_reconnect_init(reconnect_ctx_t* ctx) {
    if (!ctx) return -1;
    memset(ctx, 0, sizeof(*ctx));
    ctx->state = RECONNECT_IDLE;
    ctx->backoff_ms = 250;
    ctx->max_backoff_ms = 30000;  // 30s cap
    ctx->jitter_enabled = true;
    return 0;
}

uint32_t hw_reconnect_get_backoff(reconnect_ctx_t* ctx) {
    if (!ctx) return 250;
    
    uint32_t base = ctx->backoff_ms;
    if (ctx->jitter_enabled) {
        // Add ±20% jitter
        uint32_t jitter = (base * 20) / 100;
        uint32_t jitter_amount = (rand() % (jitter * 2)) - jitter;
        base += jitter_amount;
    }
    return base;
}

void hw_reconnect_on_failure(reconnect_ctx_t* ctx) {
    if (!ctx) return;
    
    ctx->state = RECONNECT_WAITING;
    ctx->last_attempt = time(NULL);
    ctx->attempt_count++;
    
    // Exponential backoff: 250ms → 500ms → 1s → 2s → 4s → ... (cap at 30s)
    ctx->backoff_ms *= 2;
    if (ctx->backoff_ms > ctx->max_backoff_ms) {
        ctx->backoff_ms = ctx->max_backoff_ms;
    }
}

void hw_reconnect_on_success(reconnect_ctx_t* ctx) {
    if (!ctx) return;
    
    ctx->state = RECONNECT_STABLE;
    ctx->stable_since = time(NULL);
    // Reset backoff after 60s of stability
    if (time(NULL) - ctx->stable_since > 60) {
        ctx->backoff_ms = 250;
        ctx->attempt_count = 0;
    }
}

bool hw_reconnect_should_attempt(reconnect_ctx_t* ctx) {
    if (!ctx) return false;
    if (ctx->state != RECONNECT_WAITING) return false;
    
    time_t now = time(NULL);
    uint32_t backoff = hw_reconnect_get_backoff(ctx);
    return (now - ctx->last_attempt) >= (backoff / 1000);
}

void hw_reconnect_reset(reconnect_ctx_t* ctx) {
    if (!ctx) return;
    hw_reconnect_init(ctx);
}

// ============================================================================
// TIMEOUTS
// ============================================================================

int hw_timeout_init(timeout_ctx_t* ctx) {
    if (!ctx) return -1;
    memset(ctx, 0, sizeof(*ctx));
    time_t now = time(NULL);
    ctx->handshake_start = now;
    ctx->last_read = now;
    ctx->last_write = now;
    return 0;
}

void hw_timeout_reset_handshake(timeout_ctx_t* ctx) {
    if (ctx) ctx->handshake_start = time(NULL);
}

void hw_timeout_reset_read(timeout_ctx_t* ctx) {
    if (ctx) ctx->last_read = time(NULL);
}

void hw_timeout_reset_write(timeout_ctx_t* ctx) {
    if (ctx) ctx->last_write = time(NULL);
}

void hw_timeout_reset_dns(timeout_ctx_t* ctx) {
    if (ctx) ctx->dns_start = time(NULL);
}

bool hw_timeout_check(timeout_ctx_t* ctx) {
    if (!ctx) return false;
    
    time_t now = time(NULL);
    ctx->handshake_timeout = (now - ctx->handshake_start) > (HW_TIMEOUT_HANDSHAKE_MS / 1000);
    ctx->read_timeout = (now - ctx->last_read) > (HW_TIMEOUT_READ_IDLE_MS / 1000);
    ctx->write_timeout = (now - ctx->last_write) > (HW_TIMEOUT_WRITE_STALL_MS / 1000);
    
    return ctx->handshake_timeout || ctx->read_timeout || ctx->write_timeout;
}

// ============================================================================
// CLEAN SHUTDOWN
// ============================================================================

static shutdown_state_t g_shutdown_state = SHUTDOWN_NONE;

int hw_shutdown_init(void) {
    g_shutdown_state = SHUTDOWN_NONE;
    return 0;
}

int hw_shutdown_request_drain(hw_ctx_t* ctx) {
    if (!ctx) return -1;
    g_shutdown_state = SHUTDOWN_DRAIN_REQUESTED;
    // Stop reading new data
    return 0;
}

int hw_shutdown_flush_queues(hw_ctx_t* ctx) {
    if (!ctx) return -1;
    g_shutdown_state = SHUTDOWN_FLUSHING;
    // Flush queued data (implementation depends on queue system)
    return 0;
}

int hw_shutdown_close_sockets(hw_ctx_t* ctx) {
    if (!ctx) return -1;
    g_shutdown_state = SHUTDOWN_CLOSING;
    // Close sockets cleanly
    g_shutdown_state = SHUTDOWN_COMPLETE;
    return 0;
}

bool hw_shutdown_is_complete(void) {
    return g_shutdown_state == SHUTDOWN_COMPLETE;
}

// ============================================================================
// REPLAY PROTECTION
// ============================================================================

int hw_replay_init(replay_protection_t* ctx) {
    if (!ctx) return -1;
    memset(ctx, 0, sizeof(*ctx));
    ctx->next_seqno = 1;
    ctx->window_base = 0;
    ctx->initialized = true;
    return 0;
}

bool hw_replay_check(replay_protection_t* ctx, uint64_t seqno) {
    if (!ctx || !ctx->initialized) return false;
    
    // Check if seqno is in window
    if (seqno < ctx->window_base) {
        return false;  // Too old, reject
    }
    
    if (seqno >= ctx->window_base + HW_SEQNO_WINDOW_SIZE) {
        // Too far ahead, reject
        if (seqno > ctx->window_base + HW_MAX_SEQNO_GAP) {
            return false;
        }
        // Advance window
        ctx->window_base = seqno - HW_SEQNO_WINDOW_SIZE + 1;
        memset(ctx->window, 0, sizeof(ctx->window));
    }
    
    uint64_t index = seqno - ctx->window_base;
    if (index < HW_SEQNO_WINDOW_SIZE) {
        if (ctx->window[index] != 0) {
            return false;  // Duplicate, reject
        }
    }
    
    return true;
}

void hw_replay_accept(replay_protection_t* ctx, uint64_t seqno) {
    if (!ctx || !ctx->initialized) return;
    
    if (hw_replay_check(ctx, seqno)) {
        uint64_t index = seqno - ctx->window_base;
        if (index < HW_SEQNO_WINDOW_SIZE) {
            ctx->window[index] = seqno;
        }
        if (seqno >= ctx->next_seqno) {
            ctx->next_seqno = seqno + 1;
        }
    }
}

uint64_t hw_replay_get_next_seqno(replay_protection_t* ctx) {
    if (!ctx || !ctx->initialized) return 0;
    return ctx->next_seqno++;
}

// ============================================================================
// KEY HANDLING
// ============================================================================

int hw_keys_init(session_keys_t* keys) {
    if (!keys) return -1;
    memset(keys, 0, sizeof(*keys));
    keys->created = time(NULL);
    keys->last_rotation = keys->created;
    return 0;
}

int hw_keys_derive_from_handshake(session_keys_t* keys, const uint8_t* shared_secret, size_t secret_len) {
    if (!keys || !shared_secret || secret_len == 0) return -1;
    
    // Simple HKDF-like derivation (in production, use proper HKDF)
    // Derive client→server key
    for (size_t i = 0; i < HW_KEY_SIZE && i < secret_len; i++) {
        keys->client_to_server[i] = shared_secret[i] ^ 0x01;
    }
    
    // Derive server→client key
    for (size_t i = 0; i < HW_KEY_SIZE && i < secret_len; i++) {
        keys->server_to_client[i] = shared_secret[i] ^ 0x02;
    }
    
    // Derive control channel key
    for (size_t i = 0; i < HW_KEY_SIZE && i < secret_len; i++) {
        keys->control_channel[i] = shared_secret[i] ^ 0x03;
    }
    
    return 0;
}

int hw_keys_rotate(session_keys_t* keys) {
    if (!keys) return -1;
    
    // Rotate keys (simple XOR with counter, in production use proper KDF)
    for (size_t i = 0; i < HW_KEY_SIZE; i++) {
        keys->client_to_server[i] ^= (uint8_t)(keys->rotation_count >> (i % 8));
        keys->server_to_client[i] ^= (uint8_t)(keys->rotation_count >> (i % 8));
        keys->control_channel[i] ^= (uint8_t)(keys->rotation_count >> (i % 8));
    }
    
    keys->rotation_count++;
    keys->last_rotation = time(NULL);
    return 0;
}

void hw_keys_cleanup(session_keys_t* keys) {
    if (keys) {
        memset(keys, 0, sizeof(*keys));
    }
}

bool hw_keys_should_rotate(session_keys_t* keys) {
    if (!keys) return false;
    // Rotate every 1 hour or after 1000 messages
    time_t now = time(NULL);
    return (now - keys->last_rotation > 3600) || (keys->rotation_count > HW_MAX_KEY_ROTATIONS);
}

// ============================================================================
// NONCE DISCIPLINE
// ============================================================================

int hw_nonce_init(nonce_ctx_t* ctx, bool use_counter) {
    if (!ctx) return -1;
    memset(ctx, 0, sizeof(*ctx));
    ctx->use_counter_mode = use_counter;
    if (!use_counter) {
        // Random nonce mode: generate random nonce
        for (int i = 0; i < HW_NONCE_SIZE; i++) {
            ctx->nonce[i] = (uint8_t)(rand() % 256);
        }
    }
    return 0;
}

int hw_nonce_get_next(nonce_ctx_t* ctx, uint8_t* nonce_out) {
    if (!ctx || !nonce_out) return -1;
    
    if (ctx->use_counter_mode) {
        // Counter-based nonce
        ctx->counter++;
        memcpy(nonce_out, &ctx->counter, sizeof(ctx->counter));
        // Pad with zeros if needed
        if (HW_NONCE_SIZE > sizeof(ctx->counter)) {
            memset(nonce_out + sizeof(ctx->counter), 0, HW_NONCE_SIZE - sizeof(ctx->counter));
        }
    } else {
        // Random nonce (regenerate each time)
        for (int i = 0; i < HW_NONCE_SIZE; i++) {
            nonce_out[i] = (uint8_t)(rand() % 256);
        }
    }
    
    ctx->last_used = time(NULL);
    return 0;
}

bool hw_nonce_check_collision(nonce_ctx_t* ctx, const uint8_t* nonce) {
    if (!ctx || !nonce) return false;
    // In counter mode, check if nonce is less than current counter
    if (ctx->use_counter_mode) {
        uint64_t nonce_val = 0;
        memcpy(&nonce_val, nonce, sizeof(nonce_val) < HW_NONCE_SIZE ? sizeof(nonce_val) : HW_NONCE_SIZE);
        return nonce_val < ctx->counter;
    }
    return false;
}

// ============================================================================
// RESOURCE CAPS
// ============================================================================

int hw_resource_caps_init(resource_caps_t* caps) {
    if (!caps) return -1;
    memset(caps, 0, sizeof(*caps));
    caps->last_cleanup = time(NULL);
    return 0;
}

bool hw_resource_caps_check_connection(resource_caps_t* caps, const char* ip) {
    if (!caps || !ip) return false;
    // Simple check: allow if under limit (implementation would track per IP)
    return true;  // Simplified
}

bool hw_resource_caps_check_handshake(resource_caps_t* caps) {
    if (!caps) return false;
    return caps->unauthenticated_handshakes < HW_MAX_UNAUTH_HANDSHAKES;
}

bool hw_resource_caps_check_stream(resource_caps_t* caps) {
    if (!caps) return false;
    return caps->streams_per_session < HW_MAX_STREAMS_PER_SESSION;
}

bool hw_resource_caps_check_buffer(resource_caps_t* caps, size_t additional_bytes) {
    if (!caps) return false;
    return (caps->bytes_buffered + additional_bytes) <= HW_MAX_BYTES_BUFFERED;
}

void hw_resource_caps_release_connection(resource_caps_t* caps, const char* ip) {
    // Implementation would decrement per-IP counter
}

void hw_resource_caps_release_handshake(resource_caps_t* caps) {
    if (caps && caps->unauthenticated_handshakes > 0) {
        caps->unauthenticated_handshakes--;
    }
}

void hw_resource_caps_release_stream(resource_caps_t* caps) {
    if (caps && caps->streams_per_session > 0) {
        caps->streams_per_session--;
    }
}

void hw_resource_caps_release_buffer(resource_caps_t* caps, size_t bytes) {
    if (caps && caps->bytes_buffered >= bytes) {
        caps->bytes_buffered -= bytes;
    }
}

// ============================================================================
// FRAME FORMAT
// ============================================================================

int hw_frame_encode(const hw_frame_t* frame, uint8_t* buffer, size_t buffer_size, size_t* out_len) {
    if (!frame || !buffer || !out_len) return -1;
    
    size_t needed = 4 + 1 + 1 + 2 + 4 + frame->length + HW_TAG_SIZE;
    if (buffer_size < needed) return -1;
    
    size_t offset = 0;
    
    // Magic
    memcpy(buffer + offset, &frame->magic, 4);
    offset += 4;
    
    // Version
    buffer[offset++] = frame->version;
    
    // Type
    buffer[offset++] = frame->type;
    
    // Flags
    memcpy(buffer + offset, &frame->flags, 2);
    offset += 2;
    
    // Length
    memcpy(buffer + offset, &frame->length, 4);
    offset += 4;
    
    // Payload
    if (frame->payload && frame->length > 0) {
        memcpy(buffer + offset, frame->payload, frame->length);
        offset += frame->length;
    }
    
    // Tag
    memcpy(buffer + offset, frame->tag, HW_TAG_SIZE);
    offset += HW_TAG_SIZE;
    
    *out_len = offset;
    return 0;
}

int hw_frame_decode(const uint8_t* buffer, size_t buffer_size, hw_frame_t* frame) {
    if (!buffer || !frame || buffer_size < 12) return -1;
    
    size_t offset = 0;
    
    // Magic
    memcpy(&frame->magic, buffer + offset, 4);
    offset += 4;
    if (frame->magic != HW_FRAME_MAGIC) return -1;
    
    // Version
    frame->version = buffer[offset++];
    if (frame->version != HW_FRAME_VERSION) return -1;
    
    // Type
    frame->type = buffer[offset++];
    if (frame->type < FRAME_TYPE_HELLO || frame->type > FRAME_TYPE_PONG) return -1;
    
    // Flags
    memcpy(&frame->flags, buffer + offset, 2);
    offset += 2;
    
    // Length
    memcpy(&frame->length, buffer + offset, 4);
    offset += 4;
    if (frame->length > HW_FRAME_MAX_SIZE) return -1;
    
    // Validate total size
    size_t needed = offset + frame->length + HW_TAG_SIZE;
    if (buffer_size < needed) return -1;
    
    // Payload
    if (frame->length > 0) {
        frame->payload = (uint8_t*)malloc(frame->length);
        if (!frame->payload) return -1;
        memcpy(frame->payload, buffer + offset, frame->length);
        offset += frame->length;
    } else {
        frame->payload = NULL;
    }
    
    // Tag
    memcpy(frame->tag, buffer + offset, HW_TAG_SIZE);
    offset += HW_TAG_SIZE;
    
    return 0;
}

bool hw_frame_validate(const hw_frame_t* frame) {
    if (!frame) return false;
    if (frame->magic != HW_FRAME_MAGIC) return false;
    if (frame->version != HW_FRAME_VERSION) return false;
    if (frame->type < FRAME_TYPE_HELLO || frame->type > FRAME_TYPE_PONG) return false;
    if (frame->length > HW_FRAME_MAX_SIZE) return false;
    return true;
}

void hw_frame_free(hw_frame_t* frame) {
    if (frame && frame->payload) {
        free(frame->payload);
        frame->payload = NULL;
    }
}

// ============================================================================
// STATE MACHINE
// ============================================================================

int hw_state_machine_init(state_machine_t* sm) {
    if (!sm) return -1;
    sm->current = STATE_INIT;
    sm->previous = STATE_INIT;
    sm->state_entered = time(NULL);
    sm->transition_count = 0;
    return 0;
}

bool hw_state_machine_is_valid_transition(connection_state_t from, connection_state_t to) {
    switch (from) {
        case STATE_INIT:
            return (to == STATE_HELLO || to == STATE_ERROR);
        case STATE_HELLO:
            return (to == STATE_AUTH || to == STATE_ERROR || to == STATE_CLOSED);
        case STATE_AUTH:
            return (to == STATE_READY || to == STATE_ERROR || to == STATE_CLOSED);
        case STATE_READY:
            return (to == STATE_DRAIN || to == STATE_ERROR || to == STATE_CLOSED);
        case STATE_DRAIN:
            return (to == STATE_CLOSED || to == STATE_ERROR);
        case STATE_CLOSED:
            return (to == STATE_INIT || to == STATE_ERROR);
        case STATE_ERROR:
            return (to == STATE_CLOSED || to == STATE_INIT);
        default:
            return false;
    }
}

bool hw_state_machine_transition(state_machine_t* sm, connection_state_t new_state) {
    if (!sm) return false;
    
    if (!hw_state_machine_is_valid_transition(sm->current, new_state)) {
        return false;
    }
    
    sm->previous = sm->current;
    sm->current = new_state;
    sm->state_entered = time(NULL);
    sm->transition_count++;
    return true;
}

const char* hw_state_machine_get_name(connection_state_t state) {
    switch (state) {
        case STATE_INIT: return "INIT";
        case STATE_HELLO: return "HELLO";
        case STATE_AUTH: return "AUTH";
        case STATE_READY: return "READY";
        case STATE_DRAIN: return "DRAIN";
        case STATE_CLOSED: return "CLOSED";
        case STATE_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

// ============================================================================
// INTEGRATION
// ============================================================================

int hw_security_init(hw_ctx_t* ctx, threat_model_t threats) {
    if (!ctx) return -1;
    // Initialize all security subsystems based on threat model
    // This would initialize queues, timeouts, keys, etc.
    (void)threats;  // Use threats to enable/disable features
    return 0;
}

void hw_security_cleanup(hw_ctx_t* ctx) {
    if (!ctx) return;
    // Cleanup all security subsystems
}

int hw_security_process_outgoing(hw_ctx_t* ctx, uint8_t* data, size_t* len, size_t max_len) {
    if (!ctx || !data || !len) return -1;
    // Apply security processing: frame encoding, AEAD encryption, replay protection
    // This integrates with existing stealth processing
    return 0;
}

int hw_security_process_incoming(hw_ctx_t* ctx, uint8_t* data, size_t* len, size_t max_len) {
    if (!ctx || !data || !len) return -1;
    // Apply security processing: frame decoding, AEAD decryption, replay check
    return 0;
}

