/*
 * HelloWorld - Observability Module
 * Flight recorder for debugging and analysis
 */

#ifndef OBSERVABILITY_H
#define OBSERVABILITY_H

#include "security.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// ============================================================================
// FLIGHT RECORDER (Ring-Buffer Event Log)
// ============================================================================

#define FLIGHT_RECORDER_SIZE 4096  // 4k events per session

typedef enum {
    EVENT_STATE_TRANSITION,
    EVENT_SEQNO_WINDOW_MOVE,
    EVENT_KEY_ROTATION,
    EVENT_QUEUE_OCCUPANCY,
    EVENT_TIMEOUT,
    EVENT_DISCONNECT,
    EVENT_FRAME_RECEIVED,
    EVENT_FRAME_SENT,
    EVENT_REPLAY_DETECTED,
    EVENT_AEAD_FAILURE,
    EVENT_NONCE_COLLISION,
    EVENT_ALLOCATION_OVERFLOW
} event_type_t;

typedef struct {
    event_type_t type;
    time_t timestamp;
    uint32_t session_id;
    union {
        struct {
            connection_state_t from;
            connection_state_t to;
        } state_transition;
        struct {
            uint64_t old_base;
            uint64_t new_base;
        } seqno_window;
        struct {
            int direction;
            uint64_t rotation_count;
        } key_rotation;
        struct {
            size_t queue_size;
            size_t queue_capacity;
        } queue_occupancy;
        struct {
            const char* timeout_type;
        } timeout;
        struct {
            const char* reason;
        } disconnect;
        struct {
            frame_type_t frame_type;
            uint32_t length;
        } frame;
        struct {
            uint64_t seqno;
        } replay;
    } data;
} flight_recorder_event_t;

typedef struct {
    flight_recorder_event_t events[FLIGHT_RECORDER_SIZE];
    size_t head;
    size_t tail;
    size_t count;
    bool full;
    uint32_t session_id;
} flight_recorder_t;

// Initialize flight recorder
int flight_recorder_init(flight_recorder_t* fr, uint32_t session_id);

// Record an event
int flight_recorder_record(flight_recorder_t* fr, event_type_t type, const void* event_data);

// Dump events to buffer (last N events or last 60 seconds)
int flight_recorder_dump(flight_recorder_t* fr, char* buffer, size_t buffer_size, 
                         uint32_t max_events, time_t time_window_seconds);

// Dump on error (last 60 seconds)
int flight_recorder_dump_on_error(flight_recorder_t* fr, char* buffer, size_t buffer_size);

// Get event count
size_t flight_recorder_count(flight_recorder_t* fr);

// Clear flight recorder
void flight_recorder_clear(flight_recorder_t* fr);

#endif // OBSERVABILITY_H

