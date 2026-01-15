/*
 * HelloWorld - Stealth Tunnel Client
 * Pure C Implementation
 */

#ifndef HELLOWORLD_H
#define HELLOWORLD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <stdarg.h>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <shlobj.h>
    #pragma comment(lib, "ws2_32.lib")
    #pragma comment(lib, "shell32.lib")
    #define HW_PATH_SEP "\\"
    #define HW_SLEEP(ms) Sleep(ms)
    typedef HANDLE hw_process_t;
    #define HW_INVALID_PROCESS NULL
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <sys/wait.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <signal.h>
    #include <pwd.h>
    #define HW_PATH_SEP "/"
    #define HW_SLEEP(ms) usleep((ms) * 1000)
    typedef pid_t hw_process_t;
    #define HW_INVALID_PROCESS -1
#endif

#define HW_VERSION          "1.0.0"
#define HW_APP_NAME         "HelloWorld"
#define HW_MAX_SERVERS      16
#define HW_MAX_NAME_LEN     64
#define HW_MAX_HOST_LEN     256
#define HW_MAX_PATH_LEN     512
#define HW_BUFFER_SIZE      8192
#define HW_DEFAULT_PORT     443
#define HW_LOCAL_PORT       2222
#define HW_SOCKS_PORT       1080

typedef enum {
    HW_DISCONNECTED = 0,
    HW_CONNECTING,
    HW_CONNECTED,
    HW_ERROR
} hw_status_t;

typedef enum {
    HW_MODE_PROXY = 0,
    HW_MODE_FULL_TUNNEL
} hw_mode_t;

typedef struct {
    char name[HW_MAX_NAME_LEN];
    char host[HW_MAX_HOST_LEN];
    uint16_t port;
    char key_path[HW_MAX_PATH_LEN];
    char username[HW_MAX_NAME_LEN];
} hw_server_t;

typedef struct {
    uint64_t bytes_up;
    uint64_t bytes_down;
    time_t start_time;
    double latency;
    char public_ip[64];
} hw_stats_t;

typedef struct {
    hw_status_t status;
    hw_mode_t mode;
    hw_server_t servers[HW_MAX_SERVERS];
    int server_count;
    int current_server;
    hw_stats_t stats;
    bool kill_switch;
    hw_process_t stunnel_proc;
    hw_process_t ssh_proc;
    char config_dir[HW_MAX_PATH_LEN];
    char error_msg[256];
} hw_ctx_t;

hw_ctx_t* hw_create(void);
void hw_destroy(hw_ctx_t* ctx);

int hw_connect(hw_ctx_t* ctx);
int hw_disconnect(hw_ctx_t* ctx);

int hw_add_server(hw_ctx_t* ctx, const char* name, const char* host, 
                  uint16_t port, const char* key_path, const char* username);
int hw_remove_server(hw_ctx_t* ctx, int index);
int hw_select_server(hw_ctx_t* ctx, int index);

int hw_load_config(hw_ctx_t* ctx);
int hw_save_config(hw_ctx_t* ctx);

int hw_fetch_public_ip(hw_ctx_t* ctx);
double hw_ping(const char* host);

int hw_setup_system_proxy(hw_ctx_t* ctx);
int hw_clear_system_proxy(hw_ctx_t* ctx);
int hw_setup_full_tunnel(hw_ctx_t* ctx);
int hw_clear_full_tunnel(hw_ctx_t* ctx);

const char* hw_status_str(hw_status_t status);
void hw_log(const char* level, const char* fmt, ...);

#endif
