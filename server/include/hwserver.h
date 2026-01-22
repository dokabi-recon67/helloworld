/*
 * HelloWorld Server - VM Setup & Management
 */

#ifndef HWSERVER_H
#define HWSERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <pwd.h>
#include <errno.h>

#define HWS_VERSION         "1.0.0"
#define HWS_MAX_PATH        512
#define HWS_BUFFER_SIZE     4096
#define HWS_STUNNEL_PORT    443
#define HWS_SSH_PORT        22

typedef enum {
    HWS_MODE_DIRECT = 0,
    HWS_MODE_VPN_CYCLE,
    HWS_MODE_TOR
} hws_exit_mode_t;

typedef enum {
    HWS_STOPPED = 0,
    HWS_RUNNING,
    HWS_ERROR
} hws_status_t;

typedef struct {
    hws_status_t status;
    hws_exit_mode_t exit_mode;
    char public_ip[64];
    char config_dir[HWS_MAX_PATH];
    int client_count;
    time_t start_time;
    pid_t stunnel_pid;
    pid_t tor_pid;
    bool nat_configured;
    char error_msg[256];
} hws_ctx_t;

hws_ctx_t* hws_create(void);
void hws_destroy(hws_ctx_t* ctx);

int hws_start(hws_ctx_t* ctx);
int hws_stop(hws_ctx_t* ctx);

int hws_setup_stunnel(hws_ctx_t* ctx);
int hws_setup_ssh(hws_ctx_t* ctx);
int hws_setup_nat(hws_ctx_t* ctx);
int hws_clear_nat(hws_ctx_t* ctx);

int hws_set_exit_mode(hws_ctx_t* ctx, hws_exit_mode_t mode);
int hws_start_tor(hws_ctx_t* ctx);
int hws_stop_tor(hws_ctx_t* ctx);
int hws_rotate_tor_circuit(hws_ctx_t* ctx);

int hws_fetch_public_ip(hws_ctx_t* ctx);
const char* hws_status_str(hws_status_t status);
const char* hws_mode_str(hws_exit_mode_t mode);
void hws_log(const char* level, const char* fmt, ...);

#endif

