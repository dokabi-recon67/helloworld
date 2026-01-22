/*
 * HelloWorld - KeySchedule Module
 * Isolated key/nonce management - impossible to misuse
 */

#ifndef KEYSCHEDULE_H
#define KEYSCHEDULE_H

#include "security.h"
#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// KEY SCHEDULE MODULE (Isolated Key/Nonce Management)
// ============================================================================

typedef struct {
    // Per-direction keys
    hw_aead_ctx_t client_to_server;
    hw_aead_ctx_t server_to_client;
    hw_aead_ctx_t control_channel;
    
    // Nonce counters (per direction)
    uint64_t c2s_nonce_counter;
    uint64_t s2c_nonce_counter;
    uint64_t ctrl_nonce_counter;
    
    // Rotation thresholds
    uint64_t bytes_sent_c2s;
    uint64_t bytes_sent_s2c;
    uint64_t bytes_sent_ctrl;
    uint64_t messages_sent_c2s;
    uint64_t messages_sent_s2c;
    uint64_t messages_sent_ctrl;
    time_t last_rotation;
    
    // Rotation limits
    uint64_t max_bytes_before_rotation;      // e.g., 1-4 GB
    uint64_t max_messages_before_rotation;   // e.g., 1000
    time_t max_time_before_rotation;        // e.g., 3600 seconds
    
    // Overlap window (accept previous key for a few seconds/messages)
    time_t overlap_window_start;
    uint32_t overlap_messages_remaining;
    session_keys_t previous_keys;
    bool overlap_active;
    
    bool initialized;
} keyschedule_t;

// Initialize KeySchedule module
int keyschedule_init(keyschedule_t* ks, const uint8_t* shared_secret, size_t secret_len);

// Seal a frame (encrypt + authenticate)
// Direction: 0 = client→server, 1 = server→client, 2 = control
int keyschedule_seal(keyschedule_t* ks, int direction, 
                     const uint8_t* plaintext, size_t plaintext_len,
                     const uint8_t* aad, size_t aad_len,
                     uint8_t* ciphertext, uint8_t* tag);

// Open a frame (decrypt + verify)
int keyschedule_open(keyschedule_t* ks, int direction,
                     const uint8_t* ciphertext, size_t ciphertext_len,
                     const uint8_t* aad, size_t aad_len,
                     const uint8_t* tag,
                     uint8_t* plaintext);

// Check if rotation is needed (by bytes, messages, or time)
bool keyschedule_should_rotate(keyschedule_t* ks, int direction);

// Rotate keys (with overlap window)
int keyschedule_rotate(keyschedule_t* ks, int direction);

// Cleanup
void keyschedule_cleanup(keyschedule_t* ks);

// Get rotation statistics
void keyschedule_get_stats(keyschedule_t* ks, int direction,
                           uint64_t* bytes_sent, uint64_t* messages_sent,
                           time_t* last_rotation);

#endif // KEYSCHEDULE_H

