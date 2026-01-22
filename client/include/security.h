/*
 * HelloWorld - Security & Reliability Module
 * Comprehensive threat model hardening
 */

#ifndef SECURITY_H
#define SECURITY_H

#include "helloworld.h"
#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// THREAT MODEL DEFINITIONS
// ============================================================================

typedef enum {
    THREAT_ONPATH_READ = 1,        // On-path attacker can read traffic (compromised Wi-Fi)
    THREAT_ONPATH_TAMPER = 2,     // On-path attacker can tamper/insert/drop packets
    THREAT_SERVER_COMPROMISE = 4, // Server compromise / stolen config
    THREAT_DOS = 8                // DoS / resource exhaustion
} threat_model_t;

// Enable all threat models by default
#define THREAT_MODEL_ALL (THREAT_ONPATH_READ | THREAT_ONPATH_TAMPER | THREAT_SERVER_COMPROMISE | THREAT_DOS)

// ============================================================================
// BACKPRESSURE SYSTEM (Bounded Queues)
// ============================================================================

#define HW_QUEUE_SIZE 65536  // 64KB per stream
#define HW_MAX_QUEUES 8      // Max concurrent streams

typedef struct {
    uint8_t* buffer;
    size_t head;
    size_t tail;
    size_t size;
    size_t capacity;
    bool full;
} hw_ring_buffer_t;

int hw_queue_init(hw_ring_buffer_t* q, size_t capacity);
void hw_queue_free(hw_ring_buffer_t* q);
int hw_queue_push(hw_ring_buffer_t* q, const uint8_t* data, size_t len);
int hw_queue_pop(hw_ring_buffer_t* q, uint8_t* data, size_t max_len, size_t* out_len);
size_t hw_queue_available(hw_ring_buffer_t* q);
size_t hw_queue_space(hw_ring_buffer_t* q);
bool hw_queue_full(hw_ring_buffer_t* q);
bool hw_queue_empty(hw_ring_buffer_t* q);

// ============================================================================
// RECONNECT STATE MACHINE (Exponential Backoff)
// ============================================================================

typedef enum {
    RECONNECT_IDLE,
    RECONNECT_WAITING,
    RECONNECT_ATTEMPTING,
    RECONNECT_STABLE
} reconnect_state_t;

typedef struct {
    reconnect_state_t state;
    uint32_t backoff_ms;      // Current backoff (250ms → 500ms → 1s → 2s → ...)
    uint32_t max_backoff_ms;  // Cap at 30s
    time_t last_attempt;
    time_t stable_since;      // When connection became stable
    uint32_t attempt_count;
    bool jitter_enabled;
} reconnect_ctx_t;

int hw_reconnect_init(reconnect_ctx_t* ctx);
uint32_t hw_reconnect_get_backoff(reconnect_ctx_t* ctx);
void hw_reconnect_on_failure(reconnect_ctx_t* ctx);
void hw_reconnect_on_success(reconnect_ctx_t* ctx);
bool hw_reconnect_should_attempt(reconnect_ctx_t* ctx);
void hw_reconnect_reset(reconnect_ctx_t* ctx);

// ============================================================================
// TIMEOUTS
// ============================================================================

#define HW_TIMEOUT_HANDSHAKE_MS 10000   // 10s for handshake/auth
#define HW_TIMEOUT_READ_IDLE_MS 30000   // 30s read idle
#define HW_TIMEOUT_WRITE_STALL_MS 5000  // 5s write stall
#define HW_TIMEOUT_DNS_MS 5000          // 5s DNS resolution

typedef struct {
    time_t handshake_start;
    time_t last_read;
    time_t last_write;
    time_t dns_start;
    bool handshake_timeout;
    bool read_timeout;
    bool write_timeout;
    bool dns_timeout;
} timeout_ctx_t;

int hw_timeout_init(timeout_ctx_t* ctx);
void hw_timeout_reset_handshake(timeout_ctx_t* ctx);
void hw_timeout_reset_read(timeout_ctx_t* ctx);
void hw_timeout_reset_write(timeout_ctx_t* ctx);
void hw_timeout_reset_dns(timeout_ctx_t* ctx);
bool hw_timeout_check(timeout_ctx_t* ctx);

// ============================================================================
// CLEAN SHUTDOWN
// ============================================================================

typedef enum {
    SHUTDOWN_NONE,
    SHUTDOWN_DRAIN_REQUESTED,
    SHUTDOWN_DRAINING,
    SHUTDOWN_FLUSHING,
    SHUTDOWN_CLOSING,
    SHUTDOWN_COMPLETE
} shutdown_state_t;

int hw_shutdown_init(void);
int hw_shutdown_request_drain(hw_ctx_t* ctx);
int hw_shutdown_flush_queues(hw_ctx_t* ctx);
int hw_shutdown_close_sockets(hw_ctx_t* ctx);
bool hw_shutdown_is_complete(void);

// ============================================================================
// AEAD ENCRYPTION (ChaCha20-Poly1305 or AES-GCM)
// ============================================================================

#define HW_KEY_SIZE 32        // 256-bit keys
#define HW_NONCE_SIZE 12     // 96-bit nonces
#define HW_TAG_SIZE 16       // 128-bit authentication tag

typedef enum {
    HW_AEAD_CHACHA20_POLY1305,
    HW_AEAD_AES_256_GCM
} hw_aead_algorithm_t;

typedef struct {
    uint8_t key[HW_KEY_SIZE];
    uint8_t nonce[HW_NONCE_SIZE];
    uint64_t counter;        // For counter-based nonces
    hw_aead_algorithm_t alg;
    bool initialized;
} hw_aead_ctx_t;

int hw_aead_init(hw_aead_ctx_t* ctx, const uint8_t* key, hw_aead_algorithm_t alg);
int hw_aead_encrypt(hw_aead_ctx_t* ctx, const uint8_t* plaintext, size_t plaintext_len,
                     const uint8_t* aad, size_t aad_len,
                     uint8_t* ciphertext, uint8_t* tag);
int hw_aead_decrypt(hw_aead_ctx_t* ctx, const uint8_t* ciphertext, size_t ciphertext_len,
                     const uint8_t* aad, size_t aad_len,
                     const uint8_t* tag, uint8_t* plaintext);
void hw_aead_cleanup(hw_aead_ctx_t* ctx);

// ============================================================================
// REPLAY PROTECTION + ORDERING
// ============================================================================

#define HW_SEQNO_WINDOW_SIZE 64
#define HW_MAX_SEQNO_GAP 32

typedef struct {
    uint64_t next_seqno;
    uint64_t window[HW_SEQNO_WINDOW_SIZE];
    uint64_t window_base;
    bool initialized;
} replay_protection_t;

int hw_replay_init(replay_protection_t* ctx);
bool hw_replay_check(replay_protection_t* ctx, uint64_t seqno);
void hw_replay_accept(replay_protection_t* ctx, uint64_t seqno);
uint64_t hw_replay_get_next_seqno(replay_protection_t* ctx);

// ============================================================================
// KEY HANDLING (Session Keys, Rotation, Separation)
// ============================================================================

#define HW_SESSION_KEY_SIZE 32
#define HW_MAX_KEY_ROTATIONS 1000

typedef struct {
    uint8_t client_to_server[HW_KEY_SIZE];
    uint8_t server_to_client[HW_KEY_SIZE];
    uint8_t control_channel[HW_KEY_SIZE];
    uint64_t rotation_count;
    time_t last_rotation;
    time_t created;
} session_keys_t;

int hw_keys_init(session_keys_t* keys);
int hw_keys_derive_from_handshake(session_keys_t* keys, const uint8_t* shared_secret, size_t secret_len);
int hw_keys_rotate(session_keys_t* keys);
void hw_keys_cleanup(session_keys_t* keys);
bool hw_keys_should_rotate(session_keys_t* keys);

// ============================================================================
// NONCE DISCIPLINE
// ============================================================================

typedef struct {
    uint8_t nonce[HW_NONCE_SIZE];
    uint64_t counter;
    bool use_counter_mode;
    time_t last_used;
} nonce_ctx_t;

int hw_nonce_init(nonce_ctx_t* ctx, bool use_counter);
int hw_nonce_get_next(nonce_ctx_t* ctx, uint8_t* nonce_out);
bool hw_nonce_check_collision(nonce_ctx_t* ctx, const uint8_t* nonce);

// ============================================================================
// RESOURCE CAPS (DoS Resistance)
// ============================================================================

#define HW_MAX_CONNECTIONS_PER_IP 5
#define HW_MAX_UNAUTH_HANDSHAKES 10
#define HW_MAX_STREAMS_PER_SESSION 16
#define HW_MAX_BYTES_BUFFERED (1024 * 1024)  // 1MB per session

typedef struct {
    uint32_t connections_per_ip[HW_MAX_SERVERS];
    uint32_t unauthenticated_handshakes;
    uint32_t streams_per_session;
    uint64_t bytes_buffered;
    time_t last_cleanup;
} resource_caps_t;

int hw_resource_caps_init(resource_caps_t* caps);
bool hw_resource_caps_check_connection(resource_caps_t* caps, const char* ip);
bool hw_resource_caps_check_handshake(resource_caps_t* caps);
bool hw_resource_caps_check_stream(resource_caps_t* caps);
bool hw_resource_caps_check_buffer(resource_caps_t* caps, size_t additional_bytes);
void hw_resource_caps_release_connection(resource_caps_t* caps, const char* ip);
void hw_resource_caps_release_handshake(resource_caps_t* caps);
void hw_resource_caps_release_stream(resource_caps_t* caps);
void hw_resource_caps_release_buffer(resource_caps_t* caps, size_t bytes);

// ============================================================================
// FRAME FORMAT (Strict Validation)
// ============================================================================

#define HW_FRAME_MAGIC 0x48454C4C  // "HELL"
#define HW_FRAME_VERSION 1
#define HW_FRAME_MAX_SIZE (1024 * 1024)  // 1MB max frame

typedef enum {
    FRAME_TYPE_HELLO = 1,
    FRAME_TYPE_AUTH = 2,
    FRAME_TYPE_DATA = 3,
    FRAME_TYPE_CLOSE = 4,
    FRAME_TYPE_PING = 5,
    FRAME_TYPE_PONG = 6
} frame_type_t;

typedef struct {
    uint32_t magic;
    uint8_t version;
    uint8_t type;
    uint16_t flags;
    uint32_t length;
    uint8_t* payload;
    uint8_t tag[HW_TAG_SIZE];  // Auth tag
} hw_frame_t;

int hw_frame_encode(const hw_frame_t* frame, uint8_t* buffer, size_t buffer_size, size_t* out_len);
int hw_frame_decode(const uint8_t* buffer, size_t buffer_size, hw_frame_t* frame);
bool hw_frame_validate(const hw_frame_t* frame);
void hw_frame_free(hw_frame_t* frame);

// ============================================================================
// EXPLICIT STATE MACHINE
// ============================================================================

typedef enum {
    STATE_INIT,
    STATE_HELLO,
    STATE_AUTH,
    STATE_READY,
    STATE_DRAIN,
    STATE_CLOSED,
    STATE_ERROR
} connection_state_t;

typedef struct {
    connection_state_t current;
    connection_state_t previous;
    time_t state_entered;
    uint32_t transition_count;
} state_machine_t;

int hw_state_machine_init(state_machine_t* sm);
bool hw_state_machine_transition(state_machine_t* sm, connection_state_t new_state);
const char* hw_state_machine_get_name(connection_state_t state);
bool hw_state_machine_is_valid_transition(connection_state_t from, connection_state_t to);

// ============================================================================
// INTEGRATION FUNCTIONS
// ============================================================================

int hw_security_init(hw_ctx_t* ctx, threat_model_t threats);
void hw_security_cleanup(hw_ctx_t* ctx);
int hw_security_process_outgoing(hw_ctx_t* ctx, uint8_t* data, size_t* len, size_t max_len);
int hw_security_process_incoming(hw_ctx_t* ctx, uint8_t* data, size_t* len, size_t max_len);

#endif // SECURITY_H

