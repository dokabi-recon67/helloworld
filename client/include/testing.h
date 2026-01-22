/*
 * HelloWorld - Testing & Verification Module
 * Property-based tests, evil peer tests, and correctness proofs
 */

#ifndef TESTING_H
#define TESTING_H

#include "security.h"
#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// PROTOCOL INVARIANTS (Property-Based Tests)
// ============================================================================

typedef struct {
    uint32_t frames_processed;
    uint32_t frames_rejected;
    uint32_t state_transitions;
    uint32_t invalid_transitions_blocked;
    uint32_t replay_attacks_blocked;
    uint32_t aead_failures;
    uint32_t nonce_collisions;
    uint32_t allocation_overflows;
} invariant_stats_t;

// Invariant: No frame accepted unless magic/version/type/len valid
bool test_invariant_frame_validation(void);

// Invariant: Len prefix never causes allocation > MAX_FRAME
bool test_invariant_max_frame_allocation(void);

// Invariant: State machine never accepts out-of-state message types
bool test_invariant_state_machine_transitions(void);

// Invariant: Replay window - duplicates rejected, in-order accepted, out-of-window rejected
bool test_invariant_replay_protection(void);

// Invariant: AEAD - any bit flip in ciphertext → rejection (no partial processing)
bool test_invariant_aead_integrity(void);

// Invariant: Nonce uniqueness - never repeats within a session key
bool test_invariant_nonce_uniqueness(void);

// Run all invariant tests
int run_all_invariant_tests(invariant_stats_t* stats);

// ============================================================================
// DIFFERENTIAL "EVIL PEER" TESTS
// ============================================================================

typedef struct {
    uint32_t valid_headers_invalid_length;
    uint32_t split_frames_weird_boundaries;
    uint32_t out_of_order_seqnos;
    uint32_t duplicated_seqnos;
    uint32_t stalled_mid_frame;
    uint32_t handshake_floods;
    uint32_t auth_floods;
} evil_peer_stats_t;

// Evil peer: sends valid headers with invalid lengths
bool test_evil_peer_invalid_lengths(void);

// Evil peer: splits frames across reads in weird boundaries
bool test_evil_peer_split_frames(void);

// Evil peer: sends out-of-order / duplicated seqnos
bool test_evil_peer_seqno_attacks(void);

// Evil peer: stalls mid-frame
bool test_evil_peer_stall_attack(void);

// Evil peer: floods handshake/auth
bool test_evil_peer_flood_attack(void);

// Run all evil peer tests
int run_all_evil_peer_tests(evil_peer_stats_t* stats);

// ============================================================================
// FUZZING TARGETS (libFuzzer/AFL++)
// ============================================================================

// Fuzzing target for frame decoder
int fuzz_frame_decoder(const uint8_t* data, size_t size);

// Fuzzing target for state machine transitions
int fuzz_state_machine(const uint8_t* data, size_t size);

// Fuzzing target for replay protection
int fuzz_replay_protection(const uint8_t* data, size_t size);

// ============================================================================
// HANDSHAKE HARDENING TESTS
// ============================================================================

// Test forward secrecy
bool test_forward_secrecy(void);

// Test anti-downgrade guarantees
bool test_anti_downgrade(void);

// Test channel binding
bool test_channel_binding(void);

// Test HKDF over transcript hash
bool test_hkdf_transcript(void);

// ============================================================================
// KEY/NONCE MANAGEMENT TESTS
// ============================================================================

// Test KeySchedule module isolation
bool test_keyschedule_isolation(void);

// Test nonce discipline enforcement
bool test_nonce_discipline_enforcement(void);

// Test rotation thresholds
bool test_rotation_thresholds(void);

// ============================================================================
// TORTURE TESTS (Real-World Scenarios)
// ============================================================================

typedef struct {
    double packet_loss_percent;
    uint32_t rtt_ms;
    uint32_t jitter_ms;
    bool burst_loss;
    bool reordering;
    bool duplication;
    uint32_t mtu;
} netem_profile_t;

// Torture test: netem profiles (3% loss, 200ms RTT, 50ms jitter)
int torture_test_netem_profile(const netem_profile_t* profile, uint32_t duration_seconds);

// Torture test: burst loss (Gilbert-Elliott)
int torture_test_burst_loss(uint32_t duration_seconds);

// Torture test: reordering + duplication
int torture_test_reordering_duplication(uint32_t duration_seconds);

// Torture test: MTU chaos (1280, 1400, 1500)
int torture_test_mtu_chaos(uint32_t duration_seconds);

// Torture test: long soak (12-24h under moderate throughput)
int torture_test_long_soak(uint32_t duration_hours);

// Torture test: kill -9 server mid-stream; verify client recovers cleanly
int torture_test_server_crash_recovery(void);

// Torture test: clock jumps (NTP adjustments)
int torture_test_clock_jumps(void);

// Run all torture tests
int run_all_torture_tests(void);

#endif // TESTING_H

