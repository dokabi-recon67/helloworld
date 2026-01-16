/*
 * HelloWorld - Tunnel Management
 */

#include "helloworld.h"

#ifdef _WIN32
#include <shlobj.h>

static HANDLE launch_process(const char* cmd) {
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    
    char cmdline[HW_BUFFER_SIZE];
    strncpy(cmdline, cmd, sizeof(cmdline) - 1);
    
    if (CreateProcessA(NULL, cmdline, NULL, NULL, FALSE,
                       CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hThread);
        return pi.hProcess;
    }
    return NULL;
}

static void kill_process(HANDLE proc) {
    if (proc && proc != INVALID_HANDLE_VALUE) {
        TerminateProcess(proc, 0);
        WaitForSingleObject(proc, 3000);
        CloseHandle(proc);
    }
}

static int is_process_running(HANDLE proc) {
    if (!proc || proc == INVALID_HANDLE_VALUE) return 0;
    DWORD code;
    return GetExitCodeProcess(proc, &code) && code == STILL_ACTIVE;
}

#else

static pid_t launch_process(const char* cmd) {
    pid_t pid = fork();
    if (pid == 0) {
        setsid();
        execl("/bin/sh", "sh", "-c", cmd, NULL);
        _exit(127);
    }
    return pid;
}

static void kill_process(pid_t pid) {
    if (pid > 0) {
        kill(pid, SIGTERM);
        usleep(100000);
        kill(pid, SIGKILL);
        waitpid(pid, NULL, WNOHANG);
    }
}

static int is_process_running(pid_t pid) {
    if (pid <= 0) return 0;
    return kill(pid, 0) == 0;
}

#endif

static int write_stunnel_config(hw_ctx_t* ctx) {
    if (ctx->current_server < 0) return -1;
    
    hw_server_t* srv = &ctx->servers[ctx->current_server];
    char config_path[HW_MAX_PATH_LEN];
    snprintf(config_path, sizeof(config_path), "%s%sstunnel.conf", 
             ctx->config_dir, HW_PATH_SEP);
    
    FILE* f = fopen(config_path, "w");
    if (!f) return -1;
    
    fprintf(f, "pid = \n");
    fprintf(f, "client = yes\n");
    fprintf(f, "sni = %s\n", srv->host);
    fprintf(f, "\n");
    fprintf(f, "[ssh]\n");
    fprintf(f, "accept = 127.0.0.1:%d\n", HW_LOCAL_PORT);
    fprintf(f, "connect = %s:%d\n", srv->host, srv->port);
    
    fclose(f);
    return 0;
}

static int check_stunnel_installed(void) {
#ifdef _WIN32
    char path[MAX_PATH];
    DWORD result = SearchPathA(NULL, "stunnel.exe", NULL, MAX_PATH, path, NULL);
    if (result > 0) {
        DWORD attrs = GetFileAttributesA(path);
        if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
            return 1;
        }
    }
    
    const char* common_paths[] = {
        "C:\\Program Files (x86)\\stunnel\\bin\\stunnel.exe",
        "C:\\Program Files\\stunnel\\bin\\stunnel.exe",
        "C:\\stunnel\\bin\\stunnel.exe"
    };
    for (int i = 0; i < 3; i++) {
        DWORD attrs = GetFileAttributesA(common_paths[i]);
        if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
            return 1;
        }
    }
    
    char program_files[MAX_PATH];
    if (SHGetSpecialFolderPathA(NULL, program_files, CSIDL_PROGRAM_FILES, FALSE)) {
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s\\stunnel\\bin\\stunnel.exe", program_files);
        DWORD attrs = GetFileAttributesA(full_path);
        if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
            return 1;
        }
    }
    
    char program_files_x86[MAX_PATH];
    if (SHGetSpecialFolderPathA(NULL, program_files_x86, CSIDL_PROGRAM_FILESX86, FALSE)) {
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s\\stunnel\\bin\\stunnel.exe", program_files_x86);
        DWORD attrs = GetFileAttributesA(full_path);
        if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
            return 1;
        }
    }
    
    return 0;
#else
    return system("which stunnel > /dev/null 2>&1") == 0;
#endif
}

static int start_stunnel(hw_ctx_t* ctx) {
    if (!check_stunnel_installed()) {
        snprintf(ctx->error_msg, sizeof(ctx->error_msg), 
                 "stunnel not found! Please install stunnel:\n"
                 "Download from: https://www.stunnel.org/downloads.html\n"
                 "Or use: winget install stunnel");
        return -1;
    }
    
    if (write_stunnel_config(ctx) != 0) {
        snprintf(ctx->error_msg, sizeof(ctx->error_msg), 
                 "Failed to write stunnel config");
        return -1;
    }
    
    char config_path[HW_MAX_PATH_LEN];
    char cmd[HW_BUFFER_SIZE];
    
    snprintf(config_path, sizeof(config_path), "%s%sstunnel.conf",
             ctx->config_dir, HW_PATH_SEP);
    
#ifdef _WIN32
    char stunnel_path[MAX_PATH] = "stunnel";
    char found_path[MAX_PATH] = {0};
    
    char path[MAX_PATH];
    DWORD result = SearchPathA(NULL, "stunnel.exe", NULL, MAX_PATH, path, NULL);
    if (result > 0) {
        DWORD attrs = GetFileAttributesA(path);
        if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
            snprintf(found_path, sizeof(found_path), "\"%s\"", path);
        }
    }
    
    if (!found_path[0]) {
        const char* common_paths[] = {
            "C:\\Program Files (x86)\\stunnel\\bin\\stunnel.exe",
            "C:\\Program Files\\stunnel\\bin\\stunnel.exe",
            "C:\\stunnel\\bin\\stunnel.exe"
        };
        for (int i = 0; i < 3; i++) {
            DWORD attrs = GetFileAttributesA(common_paths[i]);
            if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                snprintf(found_path, sizeof(found_path), "\"%s\"", common_paths[i]);
                break;
            }
        }
    }
    
    if (!found_path[0]) {
        char program_files_x86[MAX_PATH];
        if (SHGetSpecialFolderPathA(NULL, program_files_x86, CSIDL_PROGRAM_FILESX86, FALSE)) {
            char full_path[MAX_PATH];
            snprintf(full_path, sizeof(full_path), "%s\\stunnel\\bin\\stunnel.exe", program_files_x86);
            DWORD attrs = GetFileAttributesA(full_path);
            if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                snprintf(found_path, sizeof(found_path), "\"%s\"", full_path);
            }
        }
    }
    
    if (!found_path[0]) {
        char program_files[MAX_PATH];
        if (SHGetSpecialFolderPathA(NULL, program_files, CSIDL_PROGRAM_FILES, FALSE)) {
            char full_path[MAX_PATH];
            snprintf(full_path, sizeof(full_path), "%s\\stunnel\\bin\\stunnel.exe", program_files);
            DWORD attrs = GetFileAttributesA(full_path);
            if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                snprintf(found_path, sizeof(found_path), "\"%s\"", full_path);
            }
        }
    }
    
    if (found_path[0]) {
        strcpy(stunnel_path, found_path);
    }
    
    snprintf(cmd, sizeof(cmd), "%s \"%s\"", stunnel_path, config_path);
#else
    snprintf(cmd, sizeof(cmd), "stunnel \"%s\"", config_path);
#endif
    
    ctx->stunnel_proc = launch_process(cmd);
    if (ctx->stunnel_proc == HW_INVALID_PROCESS) {
        snprintf(ctx->error_msg, sizeof(ctx->error_msg),
                 "Failed to start stunnel process");
        return -1;
    }
    
    HW_SLEEP(1500);
    
    if (!is_process_running(ctx->stunnel_proc)) {
        snprintf(ctx->error_msg, sizeof(ctx->error_msg),
                 "stunnel exited unexpectedly.\nCheck if port %d is available.", HW_LOCAL_PORT);
        return -1;
    }
    
    return 0;
}

static int start_ssh(hw_ctx_t* ctx) {
    if (ctx->current_server < 0) return -1;
    
    hw_server_t* srv = &ctx->servers[ctx->current_server];
    char cmd[HW_BUFFER_SIZE];
    
#ifdef _WIN32
    snprintf(cmd, sizeof(cmd),
             "ssh -N -D %d -p %d -i \"%s\" "
             "-o StrictHostKeyChecking=no "
             "-o UserKnownHostsFile=NUL "
             "-o ServerAliveInterval=30 "
             "-o ServerAliveCountMax=3 "
             "-o ConnectTimeout=10 "
             "-o BatchMode=yes "
             "%s@127.0.0.1",
             HW_SOCKS_PORT, HW_LOCAL_PORT, srv->key_path, srv->username);
#else
    snprintf(cmd, sizeof(cmd),
             "ssh -N -D %d -p %d -i \"%s\" "
             "-o StrictHostKeyChecking=no "
             "-o UserKnownHostsFile=/dev/null "
             "-o ServerAliveInterval=30 "
             "-o ServerAliveCountMax=3 "
             "-o ConnectTimeout=10 "
             "-o BatchMode=yes "
             "%s@127.0.0.1",
             HW_SOCKS_PORT, HW_LOCAL_PORT, srv->key_path, srv->username);
#endif
    
    ctx->ssh_proc = launch_process(cmd);
    if (ctx->ssh_proc == HW_INVALID_PROCESS) {
        snprintf(ctx->error_msg, sizeof(ctx->error_msg),
                 "Failed to start SSH. Check if OpenSSH is installed.");
        return -1;
    }
    
    HW_SLEEP(3000);
    
    if (!is_process_running(ctx->ssh_proc)) {
        snprintf(ctx->error_msg, sizeof(ctx->error_msg),
                 "SSH connection failed!\n\n"
                 "Possible causes:\n"
                 "- Wrong username (try: opc, ubuntu, or your user)\n"
                 "- SSH key not authorized on server\n"
                 "- Server not reachable on port 443\n"
                 "- Firewall blocking connection");
        return -1;
    }
    
    return 0;
}

int hw_connect(hw_ctx_t* ctx) {
    if (!ctx) return -1;
    if (ctx->current_server < 0) {
        snprintf(ctx->error_msg, sizeof(ctx->error_msg), "No server selected");
        return -1;
    }
    if (ctx->status == HW_CONNECTED) return 0;
    
    ctx->status = HW_CONNECTING;
    hw_log("INFO", "Connecting to %s", ctx->servers[ctx->current_server].name);
    
    if (start_stunnel(ctx) != 0) {
        ctx->status = HW_ERROR;
        return -1;
    }
    
    if (start_ssh(ctx) != 0) {
        kill_process(ctx->stunnel_proc);
        ctx->stunnel_proc = HW_INVALID_PROCESS;
        ctx->status = HW_ERROR;
        return -1;
    }
    
    if (ctx->mode == HW_MODE_FULL_TUNNEL) {
        hw_setup_full_tunnel(ctx);
    } else {
        hw_setup_system_proxy(ctx);
    }
    
    ctx->stats.start_time = time(NULL);
    ctx->stats.bytes_up = 0;
    ctx->stats.bytes_down = 0;
    ctx->status = HW_CONNECTED;
    
    hw_fetch_public_ip(ctx);
    hw_log("INFO", "Connected - Public IP: %s", ctx->stats.public_ip);
    
    return 0;
}

int hw_disconnect(hw_ctx_t* ctx) {
    if (!ctx) return -1;
    if (ctx->status == HW_DISCONNECTED) return 0;
    
    hw_log("INFO", "Disconnecting");
    
    if (ctx->mode == HW_MODE_FULL_TUNNEL) {
        hw_clear_full_tunnel(ctx);
    } else {
        hw_clear_system_proxy(ctx);
    }
    
    kill_process(ctx->ssh_proc);
    ctx->ssh_proc = HW_INVALID_PROCESS;
    
    kill_process(ctx->stunnel_proc);
    ctx->stunnel_proc = HW_INVALID_PROCESS;
    
    ctx->status = HW_DISCONNECTED;
    memset(ctx->stats.public_ip, 0, sizeof(ctx->stats.public_ip));
    
    return 0;
}

int hw_setup_system_proxy(hw_ctx_t* ctx) {
#ifdef _WIN32
    HKEY key;
    if (RegOpenKeyExA(HKEY_CURRENT_USER,
                      "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings",
                      0, KEY_WRITE, &key) == ERROR_SUCCESS) {
        DWORD enable = 1;
        char proxy[64];
        snprintf(proxy, sizeof(proxy), "socks=127.0.0.1:%d", HW_SOCKS_PORT);
        
        RegSetValueExA(key, "ProxyEnable", 0, REG_DWORD, (BYTE*)&enable, sizeof(enable));
        RegSetValueExA(key, "ProxyServer", 0, REG_SZ, (BYTE*)proxy, (DWORD)strlen(proxy) + 1);
        RegCloseKey(key);
        
        SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
                           (LPARAM)"Environment", SMTO_ABORTIFHUNG, 5000, NULL);
    }
#else
    char cmd[256];
    snprintf(cmd, sizeof(cmd), 
             "networksetup -setsocksfirewallproxy Wi-Fi 127.0.0.1 %d", HW_SOCKS_PORT);
    system(cmd);
    system("networksetup -setsocksfirewallproxystate Wi-Fi on");
#endif
    return 0;
}

int hw_clear_system_proxy(hw_ctx_t* ctx) {
    (void)ctx;
#ifdef _WIN32
    HKEY key;
    if (RegOpenKeyExA(HKEY_CURRENT_USER,
                      "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings",
                      0, KEY_WRITE, &key) == ERROR_SUCCESS) {
        DWORD disable = 0;
        RegSetValueExA(key, "ProxyEnable", 0, REG_DWORD, (BYTE*)&disable, sizeof(disable));
        RegCloseKey(key);
        
        SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
                           (LPARAM)"Environment", SMTO_ABORTIFHUNG, 5000, NULL);
    }
#else
    system("networksetup -setsocksfirewallproxystate Wi-Fi off");
#endif
    return 0;
}

int hw_setup_full_tunnel(hw_ctx_t* ctx) {
    if (!ctx || ctx->current_server < 0) return -1;
    
    hw_server_t* srv = &ctx->servers[ctx->current_server];
    
#ifdef _WIN32
    char cmd[HW_BUFFER_SIZE];
    snprintf(cmd, sizeof(cmd),
             "route add 0.0.0.0 mask 128.0.0.0 127.0.0.1 && "
             "route add 128.0.0.0 mask 128.0.0.0 127.0.0.1");
    system(cmd);
#else
    char cmd[HW_BUFFER_SIZE];
    
    snprintf(cmd, sizeof(cmd),
             "sudo route add -net 0.0.0.0/1 127.0.0.1 && "
             "sudo route add -net 128.0.0.0/1 127.0.0.1");
    system(cmd);
#endif
    
    hw_setup_system_proxy(ctx);
    return 0;
}

int hw_clear_full_tunnel(hw_ctx_t* ctx) {
    (void)ctx;
    
#ifdef _WIN32
    system("route delete 0.0.0.0 mask 128.0.0.0");
    system("route delete 128.0.0.0 mask 128.0.0.0");
#else
    system("sudo route delete -net 0.0.0.0/1");
    system("sudo route delete -net 128.0.0.0/1");
#endif
    
    hw_clear_system_proxy(ctx);
    return 0;
}

