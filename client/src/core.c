/*
 * HelloWorld - Core Implementation
 */

#include "helloworld.h"

static void get_config_directory(char* buffer, size_t size) {
#ifdef _WIN32
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path))) {
        snprintf(buffer, size, "%s\\HelloWorld", path);
    } else {
        snprintf(buffer, size, ".\\HelloWorld");
    }
#else
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        home = pw ? pw->pw_dir : ".";
    }
    snprintf(buffer, size, "%s/.helloworld", home);
#endif
}

static int create_directory(const char* path) {
#ifdef _WIN32
    return CreateDirectoryA(path, NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
#else
    return mkdir(path, 0755) == 0 || errno == EEXIST;
#endif
}

static int file_exists(const char* path) {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path);
    return attr != INVALID_FILE_ATTRIBUTES;
#else
    return access(path, F_OK) == 0;
#endif
}

hw_ctx_t* hw_create(void) {
    hw_ctx_t* ctx = (hw_ctx_t*)calloc(1, sizeof(hw_ctx_t));
    if (!ctx) return NULL;
    
    ctx->status = HW_DISCONNECTED;
    ctx->mode = HW_MODE_PROXY;
    ctx->current_server = -1;
    ctx->stunnel_proc = HW_INVALID_PROCESS;
    ctx->ssh_proc = HW_INVALID_PROCESS;
    
    get_config_directory(ctx->config_dir, sizeof(ctx->config_dir));
    create_directory(ctx->config_dir);
    
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
    
    hw_load_config(ctx);
    return ctx;
}

void hw_destroy(hw_ctx_t* ctx) {
    if (!ctx) return;
    
    if (ctx->status == HW_CONNECTED) {
        hw_disconnect(ctx);
    }
    
    hw_save_config(ctx);
    
#ifdef _WIN32
    WSACleanup();
#endif
    
    free(ctx);
}

const char* hw_status_str(hw_status_t status) {
    switch (status) {
        case HW_DISCONNECTED: return "Disconnected";
        case HW_CONNECTING:   return "Connecting";
        case HW_CONNECTED:    return "Connected";
        case HW_ERROR:        return "Error";
        default:              return "Unknown";
    }
}

void hw_log(const char* level, const char* fmt, ...) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    
    fprintf(stderr, "[%02d:%02d:%02d] [%s] ", 
            t->tm_hour, t->tm_min, t->tm_sec, level);
    
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    
    fprintf(stderr, "\n");
}

int hw_add_server(hw_ctx_t* ctx, const char* name, const char* host,
                  uint16_t port, const char* key_path, const char* username) {
    if (!ctx || ctx->server_count >= HW_MAX_SERVERS) return -1;
    
    hw_server_t* srv = &ctx->servers[ctx->server_count];
    strncpy(srv->name, name, HW_MAX_NAME_LEN - 1);
    strncpy(srv->host, host, HW_MAX_HOST_LEN - 1);
    strncpy(srv->key_path, key_path, HW_MAX_PATH_LEN - 1);
    strncpy(srv->username, username, HW_MAX_NAME_LEN - 1);
    srv->port = port ? port : HW_DEFAULT_PORT;
    
    ctx->server_count++;
    if (ctx->current_server < 0) ctx->current_server = 0;
    
    return ctx->server_count - 1;
}

int hw_remove_server(hw_ctx_t* ctx, int index) {
    if (!ctx || index < 0 || index >= ctx->server_count) return -1;
    
    for (int i = index; i < ctx->server_count - 1; i++) {
        ctx->servers[i] = ctx->servers[i + 1];
    }
    ctx->server_count--;
    
    if (ctx->current_server >= ctx->server_count) {
        ctx->current_server = ctx->server_count - 1;
    }
    
    return 0;
}

int hw_select_server(hw_ctx_t* ctx, int index) {
    if (!ctx || index < 0 || index >= ctx->server_count) return -1;
    ctx->current_server = index;
    return 0;
}

