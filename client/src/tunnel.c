/*
 * HelloWorld - Tunnel Management
 */

#include "helloworld.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#ifdef _WIN32
#include <shlobj.h>
#include <tlhelp32.h>

// Kill any existing stunnel processes on port 2222
static void kill_existing_stunnel(void) {
#ifdef _WIN32
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return;
    
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    
    if (Process32First(snapshot, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, "stunnel.exe") == 0) {
                HANDLE proc = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                if (proc) {
                    TerminateProcess(proc, 0);
                    WaitForSingleObject(proc, 2000);
                    CloseHandle(proc);
                }
            }
        } while (Process32Next(snapshot, &pe));
    }
    
    CloseHandle(snapshot);
    HW_SLEEP(1000);  // Wait for port to be released
#endif
}

static HANDLE launch_process(const char* exe_path, const char* args) {
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    
    char cmdline[HW_BUFFER_SIZE];
    // For stunnel, launch via cmd.exe /c to ensure proper environment and socket binding
    // This is a workaround for Windows stunnel binding issues
    int is_stunnel = (strstr(exe_path, "stunnel") != NULL);
    
    if (is_stunnel && args && args[0]) {
        // Launch stunnel via cmd.exe /c - this ensures proper socket binding on Windows
        // Use CREATE_NO_WINDOW to make it completely headless
        snprintf(cmdline, sizeof(cmdline), "/c \"\"%s\" %s\"", exe_path, args);
        DWORD flags = CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP;
        if (CreateProcessA("C:\\Windows\\System32\\cmd.exe", cmdline, NULL, NULL, FALSE,
                           flags, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hThread);
            return pi.hProcess;
        }
        // Fallback to direct launch if cmd.exe method fails
    }
    
    // For non-stunnel processes, use normal launch
    DWORD flags = CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP;
    
    // When lpApplicationName is provided, lpCommandLine should be just the arguments
    // When lpApplicationName is NULL, lpCommandLine should be the full command line
    if (args && args[0]) {
        // Try with lpApplicationName first (just arguments in cmdline)
        if (CreateProcessA(exe_path, (LPSTR)args, NULL, NULL, FALSE,
                           flags, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hThread);
            return pi.hProcess;
        }
        
        // Fallback: full command line without lpApplicationName
        snprintf(cmdline, sizeof(cmdline), "\"%s\" %s", exe_path, args);
        if (CreateProcessA(NULL, cmdline, NULL, NULL, FALSE,
                           flags, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hThread);
            return pi.hProcess;
        }
    } else {
        // No arguments - try with lpApplicationName
        if (CreateProcessA(exe_path, NULL, NULL, NULL, FALSE,
                           flags, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hThread);
            return pi.hProcess;
        }
        
        // Fallback: full command line
        snprintf(cmdline, sizeof(cmdline), "\"%s\"", exe_path);
        if (CreateProcessA(NULL, cmdline, NULL, NULL, FALSE,
                           flags, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hThread);
            return pi.hProcess;
        }
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
    if (!GetExitCodeProcess(proc, &code)) {
        // Handle is invalid, process probably exited
        return 0;
    }
    return code == STILL_ACTIVE;
}

// Find stunnel process by name (since we launch via cmd.exe, we need to find the actual stunnel.exe)
static HANDLE find_stunnel_process(void) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return NULL;
    
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    HANDLE stunnel_handle = NULL;
    
    if (Process32First(snapshot, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, "stunnel.exe") == 0) {
                // Found stunnel.exe - open handle to it
                stunnel_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                break;
            }
        } while (Process32Next(snapshot, &pe));
    }
    
    CloseHandle(snapshot);
    return stunnel_handle;
}

#else

static pid_t launch_process(const char* exe_path, const char* args) {
    pid_t pid = fork();
    if (pid == 0) {
        setsid();
        if (args && args[0]) {
            char cmd[HW_BUFFER_SIZE];
            snprintf(cmd, sizeof(cmd), "%s %s", exe_path, args);
            execl("/bin/sh", "sh", "-c", cmd, NULL);
        } else {
            execl("/bin/sh", "sh", "-c", exe_path, NULL);
        }
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
    
    // Use stealth configuration for browser-like TLS fingerprint
    if (hw_generate_stealth_stunnel_config(ctx, config_path) == 0) {
        return 0;  // Stealth config generated successfully
    }
    
    // Fallback to standard config if stealth fails
    FILE* f = fopen(config_path, "w");
    if (!f) return -1;
    
    fprintf(f, "; HelloWorld Stunnel Configuration\n");
    fprintf(f, "; Auto-generated - do not edit manually\n\n");
#ifdef _WIN32
    // Windows stunnel - run in background (headless mode)
    // Use 'quiet' mode to suppress console output and run in background
    fprintf(f, "foreground = quiet\n");
    fprintf(f, "debug = 0\n");  // No debug output - completely silent
    char log_path[HW_MAX_PATH_LEN];
    snprintf(log_path, sizeof(log_path), "%s%sstunnel.log", ctx->config_dir, HW_PATH_SEP);
    fprintf(f, "output = %s\n", log_path);
#else
    fprintf(f, "pid =\n");
    fprintf(f, "foreground = no\n");
    fprintf(f, "debug = 0\n");
    fprintf(f, "output =\n");
#endif
    fprintf(f, "\n");
    fprintf(f, "[ssh]\n");
    fprintf(f, "client = yes\n");
    // On Windows, use 0.0.0.0 instead of 127.0.0.1 - some stunnel versions have binding issues with 127.0.0.1
    // We'll still connect from 127.0.0.1, but stunnel listens on all interfaces
    fprintf(f, "accept = 0.0.0.0:%d\n", HW_LOCAL_PORT);
    fprintf(f, "delay = no\n");
    fprintf(f, "connect = %s:%d\n", srv->host, srv->port);
    fprintf(f, "verify = 0\n");
    fprintf(f, "sslVersion = all\n");
    fprintf(f, "options = NO_SSLv2\n");
    fprintf(f, "options = NO_SSLv3\n");
    // TIMEOUTclose = 0 prevents premature closure when SSH hasn't sent data yet
    // Minimal config - verify=0 is enough for self-signed certs
    // sslVersion = all ensures compatibility with server
    // Extra options (verifyChain, verifyPeer, sslVersion) can cause issues on Windows
    
    fclose(f);
    return 0;
}

static int check_stunnel_installed(void) {
#ifdef _WIN32
    char path[MAX_PATH];
    DWORD result = SearchPathA(NULL, "stunnel.exe", NULL, MAX_PATH, path, NULL);
    if (result > 0 && result < MAX_PATH) {
        DWORD attrs = GetFileAttributesA(path);
        if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
            return 1;
        }
    }
    
    // Check common installation paths first (most likely locations)
    const char* common_paths[] = {
        "C:\\Program Files (x86)\\stunnel\\bin\\stunnel.exe",
        "C:\\Program Files\\stunnel\\bin\\stunnel.exe",
        "C:\\stunnel\\bin\\stunnel.exe",
        "C:\\Program Files (x86)\\stunnel\\stunnel.exe",
        "C:\\Program Files\\stunnel\\stunnel.exe"
    };
    for (int i = 0; i < 5; i++) {
        DWORD attrs = GetFileAttributesA(common_paths[i]);
        if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
            return 1;
        }
    }
    
    // Check Program Files (x86) using system folder
    char program_files_x86[MAX_PATH];
    if (SHGetSpecialFolderPathA(NULL, program_files_x86, CSIDL_PROGRAM_FILESX86, FALSE)) {
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s\\stunnel\\bin\\stunnel.exe", program_files_x86);
        DWORD attrs = GetFileAttributesA(full_path);
        if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
            return 1;
        }
        // Also check root of stunnel folder
        snprintf(full_path, sizeof(full_path), "%s\\stunnel\\stunnel.exe", program_files_x86);
        attrs = GetFileAttributesA(full_path);
        if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
            return 1;
        }
    }
    
    // Check Program Files using system folder
    char program_files[MAX_PATH];
    if (SHGetSpecialFolderPathA(NULL, program_files, CSIDL_PROGRAM_FILES, FALSE)) {
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s\\stunnel\\bin\\stunnel.exe", program_files);
        DWORD attrs = GetFileAttributesA(full_path);
        if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
            return 1;
        }
        // Also check root of stunnel folder
        snprintf(full_path, sizeof(full_path), "%s\\stunnel\\stunnel.exe", program_files);
        attrs = GetFileAttributesA(full_path);
        if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
            return 1;
        }
    }
    
    // Check AppData Local (portable installations)
    char appdata_local[MAX_PATH];
    if (SHGetSpecialFolderPathA(NULL, appdata_local, CSIDL_LOCAL_APPDATA, FALSE)) {
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s\\stunnel\\bin\\stunnel.exe", appdata_local);
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
    
    // Kill any existing stunnel processes to free port 2222
    kill_existing_stunnel();
    
    if (write_stunnel_config(ctx) != 0) {
        snprintf(ctx->error_msg, sizeof(ctx->error_msg), 
                 "Failed to write stunnel config");
        return -1;
    }
    
    char config_path[HW_MAX_PATH_LEN];
    char config_path_abs[HW_MAX_PATH_LEN];
    char cmd[HW_BUFFER_SIZE];
    
    snprintf(config_path, sizeof(config_path), "%s%sstunnel.conf",
             ctx->config_dir, HW_PATH_SEP);
    
    // Get absolute path for stunnel (Windows needs this)
#ifdef _WIN32
    if (GetFullPathNameA(config_path, sizeof(config_path_abs), config_path_abs, NULL) > 0) {
        strncpy(config_path, config_path_abs, sizeof(config_path) - 1);
        config_path[sizeof(config_path) - 1] = '\0';
    }
#endif
    
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
            "C:\\stunnel\\bin\\stunnel.exe",
            "C:\\Program Files (x86)\\stunnel\\stunnel.exe",
            "C:\\Program Files\\stunnel\\stunnel.exe"
        };
        for (int i = 0; i < 5; i++) {
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
            } else {
                // Also check root of stunnel folder
                snprintf(full_path, sizeof(full_path), "%s\\stunnel\\stunnel.exe", program_files_x86);
                attrs = GetFileAttributesA(full_path);
                if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                    snprintf(found_path, sizeof(found_path), "\"%s\"", full_path);
                }
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
            } else {
                // Also check root of stunnel folder
                snprintf(full_path, sizeof(full_path), "%s\\stunnel\\stunnel.exe", program_files);
                attrs = GetFileAttributesA(full_path);
                if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                    snprintf(found_path, sizeof(found_path), "\"%s\"", full_path);
                }
            }
        }
    }
    
    if (found_path[0]) {
        strcpy(stunnel_path, found_path);
    } else {
        // If check_stunnel_installed passed but we can't find the path, this is a bug
        snprintf(ctx->error_msg, sizeof(ctx->error_msg),
                 "stunnel detected but path not found. Please reinstall stunnel.");
        return -1;
    }
    
    // Remove quotes from stunnel_path for CreateProcessA lpApplicationName
    char stunnel_exe[MAX_PATH];
    strncpy(stunnel_exe, stunnel_path, sizeof(stunnel_exe) - 1);
    stunnel_exe[sizeof(stunnel_exe) - 1] = '\0';
    // Remove surrounding quotes if present
    if (stunnel_exe[0] == '"' && stunnel_exe[strlen(stunnel_exe) - 1] == '"') {
        stunnel_exe[strlen(stunnel_exe) - 1] = '\0';
        memmove(stunnel_exe, stunnel_exe + 1, strlen(stunnel_exe));
    }
    
    // Build command line with properly escaped path
    // stunnel.exe expects the config file as a single argument
    // Escape any spaces in the path
    char escaped_path[HW_MAX_PATH_LEN * 2];
    char* dst = escaped_path;
    const char* src = config_path;
    *dst++ = '"';
    while (*src && (dst - escaped_path) < (int)sizeof(escaped_path) - 2) {
        if (*src == '\\') {
            *dst++ = '\\';
            *dst++ = '\\';
        } else if (*src == '"') {
            *dst++ = '\\';
            *dst++ = '"';
        } else {
            *dst++ = *src;
        }
        src++;
    }
    *dst++ = '"';
    *dst = '\0';
    
    // Use lpApplicationName with stunnel_exe, and config_path as argument
    ctx->stunnel_proc = launch_process(stunnel_exe, escaped_path);
    if (ctx->stunnel_proc == HW_INVALID_PROCESS) {
        DWORD err = GetLastError();
        char err_msg[256] = {0};
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL, err, 0, err_msg, sizeof(err_msg), NULL);
        snprintf(ctx->error_msg, sizeof(ctx->error_msg),
                 "Failed to start stunnel process.\n\n"
                 "Executable: %s\n"
                 "Config: %s\n"
                 "Error code: %lu\n"
                 "Error: %s\n\n"
                 "Troubleshooting:\n"
                 "1. Verify stunnel is installed: %s\n"
                 "2. Check if config file exists: %s\n"
                 "3. Try running manually: \"%s\" \"%s\"",
                 stunnel_exe, config_path, err, err_msg[0] ? err_msg : "Unknown error",
                 stunnel_exe, config_path, stunnel_exe, config_path);
        return -1;
    }
#else
    snprintf(cmd, sizeof(cmd), "stunnel \"%s\"", config_path);
    ctx->stunnel_proc = launch_process("stunnel", cmd);
    if (ctx->stunnel_proc == HW_INVALID_PROCESS) {
        snprintf(ctx->error_msg, sizeof(ctx->error_msg),
                 "Failed to start stunnel process.\n"
                 "Command: %s\n"
                 "Make sure stunnel is installed correctly.",
                 cmd);
        return -1;
    }
#endif
    
    // Wait for stunnel to start and bind to local port
    // In client mode, stunnel binds to local port immediately
    // It does NOT pre-connect to server - connection happens when SSH connects
    // So we just need to verify stunnel is running and has bound the port
    HW_SLEEP(3000);  // Wait 3 seconds for cmd.exe to finish launching stunnel
    
    // CRITICAL FIX: When launching via cmd.exe /c, we get cmd.exe's handle, not stunnel's
    // cmd.exe exits immediately after launching stunnel, so we need to find the actual stunnel process
    HANDLE actual_stunnel = find_stunnel_process();
    if (actual_stunnel) {
        // Close the cmd.exe handle and use the actual stunnel handle
        if (ctx->stunnel_proc != HW_INVALID_PROCESS) {
            CloseHandle(ctx->stunnel_proc);
        }
        ctx->stunnel_proc = actual_stunnel;
    }
    
    HW_SLEEP(2000);  // Wait 2 more seconds for stunnel to fully initialize
    
    hw_server_t* srv = &ctx->servers[ctx->current_server];
    
    // Critical check: stunnel process must be running
    if (!is_process_running(ctx->stunnel_proc)) {
        DWORD exit_code = 0;
        GetExitCodeProcess(ctx->stunnel_proc, &exit_code);
        char log_path[HW_MAX_PATH_LEN];
        char stunnel_log[512] = {0};
        snprintf(log_path, sizeof(log_path), "%s%sstunnel.log", ctx->config_dir, HW_PATH_SEP);
        FILE* log_file = fopen(log_path, "r");
        if (log_file) {
            fseek(log_file, -256, SEEK_END);
            fread(stunnel_log, 1, sizeof(stunnel_log) - 1, log_file);
            fclose(log_file);
        }
        snprintf(ctx->error_msg, sizeof(ctx->error_msg),
                 "Stunnel process exited (exit code: %lu)!\n\n"
                 "Stunnel started but then exited immediately.\n\n"
                 "Server: %s:%d\n"
                 "Local port: %d\n\n"
                 "%s\n\n"
                 "Possible causes:\n"
                 "1. Invalid stunnel configuration\n"
                 "2. Port %d already in use\n"
                 "3. Stunnel installation issue\n\n"
                 "Check stunnel log: %s",
                 exit_code, srv->host, srv->port, HW_LOCAL_PORT,
                 stunnel_log[0] ? stunnel_log : "No additional error details.",
                 HW_LOCAL_PORT, log_path);
        kill_process(ctx->stunnel_proc);
        ctx->stunnel_proc = HW_INVALID_PROCESS;
        return -1;
    }
    
    // Check if port 2222 is actually listening (faster than waiting for log)
    // Stunnel should bind immediately, so check socket directly first
    int port_bound = 0;
    char log_path[HW_MAX_PATH_LEN];
    snprintf(log_path, sizeof(log_path), "%s%sstunnel.log", ctx->config_dir, HW_PATH_SEP);
    
    // First, try socket check immediately (fastest method)
#ifdef _WIN32
    SOCKET test_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (test_sock != INVALID_SOCKET) {
        struct sockaddr_in test_addr;
        memset(&test_addr, 0, sizeof(test_addr));
        test_addr.sin_family = AF_INET;
        test_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        test_addr.sin_port = htons(HW_LOCAL_PORT);
        
        // Try to connect - if port is listening, connect will succeed or return WSAEWOULDBLOCK
        int test_result = connect(test_sock, (struct sockaddr*)&test_addr, sizeof(test_addr));
        int test_err = WSAGetLastError();
        closesocket(test_sock);
        
        if (test_result == 0 || test_err == WSAEWOULDBLOCK || test_err == WSAEINPROGRESS) {
            port_bound = 1;  // Port is listening!
        }
    }
#endif
    
    // If socket check didn't work, check log (but only wait 5 seconds max)
    if (!port_bound) {
        int max_wait_retries = 5;  // Only wait 5 seconds max (5 * 1s)
        int wait_count = 0;
        
        while (!port_bound && wait_count < max_wait_retries && is_process_running(ctx->stunnel_proc)) {
            FILE* log_file = fopen(log_path, "r");
            if (log_file) {
                char log_buffer[2048] = {0};
                fseek(log_file, -1024, SEEK_END);
                fread(log_buffer, 1, sizeof(log_buffer) - 1, log_file);
                fclose(log_file);
                
                // Check if stunnel bound to port 2222
                if (strstr(log_buffer, "bound to") != NULL && strstr(log_buffer, "2222") != NULL) {
                    if (strstr(log_buffer, "Accepting") != NULL || strstr(log_buffer, "LISTENING") != NULL) {
                        port_bound = 1;
                        break;
                    }
                }
            }
            
            if (!port_bound) {
                wait_count++;
                if (wait_count < max_wait_retries) {
                    HW_SLEEP(1000);  // Wait 1 second and check again
                }
            }
        }
    }
    
    // Final verification: process running AND port bound in log
    if (!is_process_running(ctx->stunnel_proc)) {
        DWORD exit_code = 0;
        GetExitCodeProcess(ctx->stunnel_proc, &exit_code);
        snprintf(ctx->error_msg, sizeof(ctx->error_msg),
                 "Stunnel process exited while waiting for port binding!\n\n"
                 "Exit code: %lu\n"
                 "Server: %s:%d\n"
                 "Check stunnel log: %s",
                 exit_code, srv->host, srv->port, log_path);
        kill_process(ctx->stunnel_proc);
        ctx->stunnel_proc = HW_INVALID_PROCESS;
        return -1;
    }
    
    if (!port_bound) {
        // Port not bound after waiting - read log for details
        char stunnel_log[1024] = {0};
        FILE* log_file = fopen(log_path, "r");
        if (log_file) {
            fseek(log_file, -512, SEEK_END);
            fread(stunnel_log, 1, sizeof(stunnel_log) - 1, log_file);
            fclose(log_file);
        }
        
        int wait_count_used = 5;  // Default if we don't know
        snprintf(ctx->error_msg, sizeof(ctx->error_msg),
                 "Stunnel running but port %d not bound after %d seconds.\n\n"
                 "Stunnel process is running but hasn't bound to local port.\n"
                 "This usually indicates a stunnel configuration error.\n\n"
                 "Server: %s:%d\n"
                 "Local port: %d\n\n"
                 "%s\n\n"
                 "Possible causes:\n"
                 "1. Stunnel configuration file error\n"
                 "2. Port %d already in use by another process\n"
                 "3. Stunnel installation issue\n\n"
                 "Check stunnel log: %s",
                 HW_LOCAL_PORT, wait_count_used,
                 srv->host, srv->port, HW_LOCAL_PORT,
                 stunnel_log[0] ? stunnel_log : "No stunnel log entries found.",
                 HW_LOCAL_PORT, log_path);
        kill_process(ctx->stunnel_proc);
        ctx->stunnel_proc = HW_INVALID_PROCESS;
        return -1;
    }
    
    // Port binding already verified above (socket check first, then log check)
    // No need to check again - proceed
    
    // Stunnel is running and port is actually bound - ready to proceed
    // In client mode, stunnel will connect to server when SSH connects
    // No need to verify server connection now - that happens lazily
    
    return 0;
}

static int start_ssh(hw_ctx_t* ctx) {
    if (ctx->current_server < 0) return -1;
    
    hw_server_t* srv = &ctx->servers[ctx->current_server];
    char args[HW_BUFFER_SIZE];
    
#ifdef _WIN32
    snprintf(args, sizeof(args),
             "-vvv -N -D %d -p %d -i \"%s\" "
             "-o StrictHostKeyChecking=no "
             "-o UserKnownHostsFile=NUL "
             "-o ServerAliveInterval=30 "
             "-o ServerAliveCountMax=3 "
             "-o ConnectTimeout=30 "
             "-o BatchMode=yes "
             "-o LogLevel=DEBUG3 "
             "%s@127.0.0.1",
             HW_SOCKS_PORT, HW_LOCAL_PORT, srv->key_path, srv->username);
#else
    snprintf(args, sizeof(args),
             "-vvv -N -D %d -p %d -i \"%s\" "
             "-o StrictHostKeyChecking=no "
             "-o UserKnownHostsFile=/dev/null "
             "-o ServerAliveInterval=30 "
             "-o ServerAliveCountMax=3 "
             "-o ConnectTimeout=30 "
             "-o BatchMode=yes "
             "-o LogLevel=DEBUG3 "
             "%s@127.0.0.1",
             HW_SOCKS_PORT, HW_LOCAL_PORT, srv->key_path, srv->username);
#endif
    
    ctx->ssh_proc = launch_process("ssh", args);
    if (ctx->ssh_proc == HW_INVALID_PROCESS) {
        DWORD err = GetLastError();
        char err_msg[256] = {0};
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL, err, 0, err_msg, sizeof(err_msg), NULL);
        snprintf(ctx->error_msg, sizeof(ctx->error_msg),
                 "Failed to start SSH process.\n\n"
                 "Windows Error: %lu - %s\n\n"
                 "SSH Command: ssh %s\n\n"
                 "Check if OpenSSH is installed:\n"
                 "1. Open PowerShell as Administrator\n"
                 "2. Run: Get-WindowsCapability -Online | Where-Object Name -like 'OpenSSH*'\n"
                 "3. Install if needed: Add-WindowsCapability -Online -Name OpenSSH.Client~~~~0.0.1.0",
                 err, err_msg[0] ? err_msg : "Unknown error", args);
        return -1;
    }
    
    // SSH connects immediately - stunnel is already listening on port 2222
    // In client mode, stunnel accepts connections lazily and connects to server on-demand
    // No need to wait - SSH will connect and stunnel will handle the TLS tunnel
    // The connection happens in this order:
    // 1. SSH connects to localhost:2222 (stunnel is listening)
    // 2. Stunnel accepts SSH connection and establishes TLS to server
    // 3. SSH traffic flows through TLS tunnel
    // No delay needed - stunnel is ready
    
    // Check if SSH process is still running (if it exited immediately, connection was refused)
    if (!is_process_running(ctx->ssh_proc)) {
        DWORD exit_code = 0;
        GetExitCodeProcess(ctx->ssh_proc, &exit_code);
        if (exit_code == 255) {
            // SSH exited immediately with 255 - connection refused
            // This means SSH couldn't connect to localhost:2222
            snprintf(ctx->error_msg, sizeof(ctx->error_msg),
                     "SSH CONNECTION REFUSED (exit code: 255)!\n\n"
                     "SSH cannot connect to localhost:2222 where stunnel should be listening.\n\n"
                     "=== DIAGNOSTIC INFO ===\n"
                     "SSH Command: ssh -N -D %d -p %d -i \"%s\" %s@127.0.0.1\n"
                     "Stunnel running: %s\n"
                     "Stunnel config: %s\\stunnel.conf\n\n"
                     "=== POSSIBLE CAUSES ===\n"
                     "1. Port 2222 not actually listening (check: netstat -an | findstr :2222)\n"
                     "2. Windows Firewall blocking localhost:2222\n"
                     "3. Stunnel bound but not accepting connections\n"
                     "4. Another process using port 2222\n\n"
                     "=== TROUBLESHOOTING ===\n"
                     "1. Check if port 2222 is listening:\n"
                     "   netstat -an | findstr :2222\n\n"
                     "2. Test stunnel manually:\n"
                     "   \"C:\\Program Files (x86)\\stunnel\\bin\\stunnel.exe\" \"%s\\stunnel.conf\"\n\n"
                     "3. Test SSH connection to stunnel:\n"
                     "   ssh -vvv -p 2222 -i \"%s\" %s@127.0.0.1\n\n"
                     "4. Check stunnel log: %s\\stunnel.log",
                     HW_SOCKS_PORT, HW_LOCAL_PORT, srv->key_path, srv->username,
                     is_process_running(ctx->stunnel_proc) ? "YES" : "NO",
                     ctx->config_dir, ctx->config_dir, srv->key_path, srv->username, ctx->config_dir);
            return -1;
        }
    }
    
    // Wait for SSH connection with periodic checks (non-blocking approach)
    // Check every 2 seconds instead of one long 20-second wait
    int ssh_wait_retries = 15;  // 15 * 2s = 30 seconds total
    int ssh_wait_count = 0;
    int ssh_connected = 0;
    
    while (ssh_wait_count < ssh_wait_retries && !ssh_connected) {
        HW_SLEEP(2000);  // Wait 2 seconds between checks
        ssh_wait_count++;
        
        // Check if SSH is still running (if it exited, connection failed)
        if (!is_process_running(ctx->ssh_proc)) {
            // SSH exited - will be handled below
            break;
        }
        
        // SSH is still running - connection likely established
        // Verify stunnel is also still running
        if (is_process_running(ctx->stunnel_proc)) {
            ssh_connected = 1;  // Both processes running = success
            break;
        }
    }
    
    // Verify stunnel is still running (critical - it might have died)
    if (!is_process_running(ctx->stunnel_proc)) {
        // Stunnel died during SSH connection attempt - this is bad
        kill_process(ctx->ssh_proc);
        ctx->ssh_proc = HW_INVALID_PROCESS;
        snprintf(ctx->error_msg, sizeof(ctx->error_msg),
                 "Stunnel process exited during SSH connection!\n\n"
                 "This usually means stunnel cannot connect to the server.\n\n"
                 "Server: %s:%d\n"
                 "Check stunnel log: %s\\stunnel.log\n\n"
                 "Possible causes:\n"
                 "1. Server stunnel not running\n"
                 "2. Firewall blocking connection\n"
                 "3. Server IP changed\n"
                 "4. TLS handshake failing",
                 srv->host, srv->port, ctx->config_dir);
        return -1;
    }
    
    // SSH connection failed - check why
    if (!is_process_running(ctx->ssh_proc)) {
        DWORD exit_code = 0;
        GetExitCodeProcess(ctx->ssh_proc, &exit_code);
        
        // Check if stunnel is running
        int stunnel_running = 0;
#ifdef _WIN32
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32 pe;
            pe.dwSize = sizeof(PROCESSENTRY32);
            if (Process32First(snapshot, &pe)) {
                do {
                    if (_stricmp(pe.szExeFile, "stunnel.exe") == 0) {
                        stunnel_running = 1;
                        break;
                    }
                } while (Process32Next(snapshot, &pe));
            }
            CloseHandle(snapshot);
        }
#endif
        
        // Read stunnel log to see what happened when SSH tried to connect
        char log_path[HW_MAX_PATH_LEN];
        char stunnel_log[2048] = {0};
        snprintf(log_path, sizeof(log_path), "%s%sstunnel.log", ctx->config_dir, HW_PATH_SEP);
        FILE* log_file = fopen(log_path, "r");
        if (log_file) {
            fseek(log_file, -1024, SEEK_END);  // Read last 1KB
            fread(stunnel_log, 1, sizeof(stunnel_log) - 1, log_file);
            fclose(log_file);
        }
        
        // Check for connection errors in stunnel log
        int connection_error = 0;
        char error_details[512] = {0};
        // Look for any connection attempts or errors
        if (strstr(stunnel_log, "Connection refused") != NULL ||
            strstr(stunnel_log, "connect") != NULL && (strstr(stunnel_log, "failed") != NULL || strstr(stunnel_log, "error") != NULL) ||
            strstr(stunnel_log, "SSL_connect") != NULL ||
            strstr(stunnel_log, "TLS") != NULL && strstr(stunnel_log, "error") != NULL ||
            strstr(stunnel_log, "LOG3") != NULL ||
            strstr(stunnel_log, "LOG4") != NULL) {
            connection_error = 1;
            // Extract relevant error lines - look for LOG3/LOG4 (errors/warnings)
            char* error_line = strstr(stunnel_log, "LOG3");
            if (!error_line) error_line = strstr(stunnel_log, "LOG4");
            if (!error_line) error_line = strstr(stunnel_log, "SSL_connect");
            if (!error_line) error_line = strstr(stunnel_log, "Connection refused");
            if (error_line) {
                char* end_line = strchr(error_line, '\n');
                if (end_line) {
                    size_t len = end_line - error_line;
                    if (len < sizeof(error_details)) {
                        strncpy(error_details, error_line, len);
                        error_details[len] = '\0';
                    }
                } else {
                    // No newline, just copy what we have
                    strncpy(error_details, error_line, sizeof(error_details) - 1);
                    error_details[sizeof(error_details) - 1] = '\0';
                }
            }
        }
        
        // Also check if stunnel log shows NO connection attempts at all
        // If stunnel is bound but never tries to connect, that's a problem
        int has_connection_attempt = 0;
        // Check for any connection-related activity in the log
        // In client mode, stunnel only connects when a client (SSH) connects to local port
        // So we need to check if there's activity AFTER SSH tried to connect
        if (strstr(stunnel_log, "SSL_connect") != NULL ||
            strstr(stunnel_log, "Connecting") != NULL ||
            strstr(stunnel_log, "Connected") != NULL ||
            strstr(stunnel_log, "Connection") != NULL ||
            strstr(stunnel_log, "LOG5") != NULL ||  // Info level - connection established
            strstr(stunnel_log, "LOG6") != NULL ||  // Info level - connection info
            strstr(stunnel_log, "LOG7") != NULL) {  // Debug level - connection details
            has_connection_attempt = 1;
        }
        
        // Check if stunnel log shows it's waiting for connections but none are happening
        // This would indicate stunnel is bound but SSH can't connect to it
        int stunnel_waiting = 0;
        if (strstr(stunnel_log, "Accepting new connections") != NULL ||
            strstr(stunnel_log, "bound to") != NULL) {
            stunnel_waiting = 1;
        }
        
        snprintf(ctx->error_msg, sizeof(ctx->error_msg),
                 "SSH CONNECTION FAILED (exit code: %lu)!\n\n"
                 "=== DIAGNOSTIC INFO ===\n"
                 "SSH Command: ssh -N -D %d -p %d -i \"%s\" %s@127.0.0.1\n"
                 "Stunnel running: %s\n"
                 "Stunnel should forward: localhost:%d -> %s:%d\n\n"
                 "%s\n\n"
                 "=== EXIT CODE %lu MEANS ===\n"
                 "%s\n\n"
                 "=== LIKELY CAUSE ===\n"
                 "%s\n\n"
                 "=== STUNNEL LOG ANALYSIS ===\n"
                 "%s\n\n"
                 "=== TROUBLESHOOTING ===\n"
                 "1. Check server stunnel is running:\n"
                 "   ssh into server and run: helloworld-status\n\n"
                 "2. Verify server IP is correct:\n"
                 "   Current: %s:%d\n"
                 "   Get new IP: curl -s https://api.ipify.org (on server)\n\n"
                 "3. Test direct SSH (bypass stunnel):\n"
                 "   ssh -i \"%s\" %s@%s\n"
                 "   If this works, stunnel connection is the problem\n\n"
                 "4. Check firewall:\n"
                 "   Server port 443 must be open from your IP\n"
                 "   GCP: Check firewall rules in console\n\n"
                 "5. Check stunnel log for details:\n"
                 "   %s",
                 exit_code, HW_SOCKS_PORT, HW_LOCAL_PORT, srv->key_path, srv->username,
                 stunnel_running ? "YES" : "NO",
                 HW_LOCAL_PORT, srv->host, srv->port,
                 error_details[0] ? error_details : "No stunnel connection errors in log (check manually)",
                 exit_code,
                 exit_code == 255 ? "Connection refused, authentication failed, or host key verification failed" :
                 exit_code == 1 ? "General SSH error" :
                 exit_code == 2 ? "SSH configuration error" :
                 "Unknown SSH error",
                 connection_error ? "Stunnel cannot connect to server. Check server status, IP, and firewall." :
                 !has_connection_attempt && stunnel_waiting ? "Stunnel is bound and waiting, but SSH cannot connect to local port 2222. This suggests SSH is being refused by stunnel or there's a port conflict." :
                 !has_connection_attempt ? "Stunnel is running but NOT attempting to connect to server when SSH connects. This suggests stunnel is not forwarding properly." :
                 exit_code == 255 ? "SSH connection refused. Possible causes: 1) SSH key authentication failed, 2) Stunnel not forwarding properly, 3) Server SSH not accessible." :
                 "Unknown error - check logs for details",
                 has_connection_attempt ? "Stunnel log shows connection attempts." :
                 stunnel_waiting ? "Stunnel is bound to port 2222 and waiting, but no connection activity when SSH tries to connect. SSH may be failing before reaching stunnel." :
                 "Stunnel log shows NO connection attempts when SSH connects. Stunnel may not be forwarding properly.",
                 srv->host, srv->port,
                 srv->key_path, srv->username, srv->host,
                 log_path);
        return -1;
    }
    
    return 0;
}

// Start stunnel in background when app launches (headless, no server needed yet)
int hw_start_stunnel_background(hw_ctx_t* ctx) {
    if (!ctx) return -1;
    
    // If stunnel is already running, don't restart it
    if (ctx->stunnel_proc != HW_INVALID_PROCESS) {
        HANDLE existing = find_stunnel_process();
        if (existing) {
            if (ctx->stunnel_proc != HW_INVALID_PROCESS && ctx->stunnel_proc != existing) {
                CloseHandle(ctx->stunnel_proc);
            }
            ctx->stunnel_proc = existing;
            return 0;  // Already running
        }
    }
    
    // Kill any existing stunnel processes
    kill_existing_stunnel();
    
    // Write a minimal stunnel config that just listens on port 2222
    // We'll update it with server info when user connects
    char config_path[HW_MAX_PATH_LEN];
    snprintf(config_path, sizeof(config_path), "%s%sstunnel.conf", 
             ctx->config_dir, HW_PATH_SEP);
    
    FILE* f = fopen(config_path, "w");
    if (!f) return -1;
    
    fprintf(f, "; HelloWorld Stunnel Configuration (Background Mode)\n");
    fprintf(f, "; Auto-generated - do not edit manually\n\n");
#ifdef _WIN32
    fprintf(f, "foreground = quiet\n");
    fprintf(f, "debug = 0\n");
    char log_path[HW_MAX_PATH_LEN];
    snprintf(log_path, sizeof(log_path), "%s%sstunnel.log", ctx->config_dir, HW_PATH_SEP);
    fprintf(f, "output = %s\n", log_path);
#else
    fprintf(f, "pid =\n");
    fprintf(f, "foreground = no\n");
    fprintf(f, "debug = 0\n");
    fprintf(f, "output =\n");
#endif
    fprintf(f, "\n");
    fprintf(f, "[ssh]\n");
    fprintf(f, "client = yes\n");
    fprintf(f, "accept = 0.0.0.0:%d\n", HW_LOCAL_PORT);
    fprintf(f, "delay = no\n");
    fprintf(f, "connect = 127.0.0.1:443\n");  // Placeholder - will be updated on connect
    fprintf(f, "verify = 0\n");
    fprintf(f, "sslVersion = all\n");
    fprintf(f, "options = NO_SSLv2\n");
    fprintf(f, "options = NO_SSLv3\n");
    fclose(f);
    
    // Launch stunnel in headless mode
    if (!check_stunnel_installed()) {
        return -1;  // Silent fail - stunnel will be checked on connect
    }
    
    char stunnel_path[MAX_PATH] = "stunnel";
    char found_path[MAX_PATH] = {0};
    
#ifdef _WIN32
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
            "C:\\stunnel\\bin\\stunnel.exe",
            "C:\\Program Files (x86)\\stunnel\\stunnel.exe",
            "C:\\Program Files\\stunnel\\stunnel.exe"
        };
        for (int i = 0; i < 5; i++) {
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
            } else {
                snprintf(full_path, sizeof(full_path), "%s\\stunnel\\stunnel.exe", program_files_x86);
                attrs = GetFileAttributesA(full_path);
                if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                    snprintf(found_path, sizeof(found_path), "\"%s\"", full_path);
                }
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
            } else {
                snprintf(full_path, sizeof(full_path), "%s\\stunnel\\stunnel.exe", program_files);
                attrs = GetFileAttributesA(full_path);
                if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                    snprintf(found_path, sizeof(found_path), "\"%s\"", full_path);
                }
            }
        }
    }
    
    if (found_path[0]) {
        strcpy(stunnel_path, found_path);
    } else {
        return -1;  // Silent fail
    }
    
    char stunnel_exe[MAX_PATH];
    strncpy(stunnel_exe, stunnel_path, sizeof(stunnel_exe) - 1);
    stunnel_exe[sizeof(stunnel_exe) - 1] = '\0';
    if (stunnel_exe[0] == '"' && stunnel_exe[strlen(stunnel_exe) - 1] == '"') {
        stunnel_exe[strlen(stunnel_exe) - 1] = '\0';
        memmove(stunnel_exe, stunnel_exe + 1, strlen(stunnel_exe));
    }
    
    char escaped_path[HW_MAX_PATH_LEN * 2];
    char* dst = escaped_path;
    const char* src = config_path;
    *dst++ = '"';
    while (*src && (dst - escaped_path) < (int)sizeof(escaped_path) - 2) {
        if (*src == '\\') {
            *dst++ = '\\';
            *dst++ = '\\';
        } else if (*src == '"') {
            *dst++ = '\\';
            *dst++ = '"';
        } else {
            *dst++ = *src;
        }
        src++;
    }
    *dst++ = '"';
    *dst = '\0';
    
    ctx->stunnel_proc = launch_process(stunnel_exe, escaped_path);
    if (ctx->stunnel_proc == HW_INVALID_PROCESS) {
        return -1;  // Silent fail
    }
    
    HW_SLEEP(2000);  // Wait for stunnel to start
    
    // Find actual stunnel process
    HANDLE actual_stunnel = find_stunnel_process();
    if (actual_stunnel) {
        if (ctx->stunnel_proc != HW_INVALID_PROCESS) {
            CloseHandle(ctx->stunnel_proc);
        }
        ctx->stunnel_proc = actual_stunnel;
    }
    
    HW_SLEEP(1000);  // Wait for port binding
    
    return 0;
#else
    char cmd[HW_BUFFER_SIZE];
    snprintf(cmd, sizeof(cmd), "stunnel \"%s\"", config_path);
    ctx->stunnel_proc = launch_process("stunnel", cmd);
    if (ctx->stunnel_proc == HW_INVALID_PROCESS) {
        return -1;
    }
    return 0;
#endif
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
    
    // Activate FULL stealth system (10/10 rating) - ALL features enabled
    hw_activate_full_stealth(ctx);
    
    // Start active dummy traffic generation
    hw_generate_active_dummy_traffic(ctx);
    
    hw_fetch_public_ip(ctx);
    hw_fetch_server_time(ctx);
    hw_fetch_dns_info(ctx);
    
    hw_log("INFO", "Connected - Public IP: %s (Full Stealth Active)", ctx->stats.public_ip);
    
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
    
    // Restore DNS settings
    hw_restore_dns(ctx);
    
    // Kill SSH first, then stunnel
    kill_process(ctx->ssh_proc);
    ctx->ssh_proc = HW_INVALID_PROCESS;
    
    // Wait a bit for SSH to fully terminate before killing stunnel
    HW_SLEEP(500);
    
    kill_process(ctx->stunnel_proc);
    ctx->stunnel_proc = HW_INVALID_PROCESS;
    
    // Wait for processes to fully terminate before allowing reconnect
    HW_SLEEP(1000);
    
    // Clean up any leftover processes (Windows sometimes leaves zombie processes)
#ifdef _WIN32
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(snapshot, &pe)) {
            do {
                if (_stricmp(pe.szExeFile, "stunnel.exe") == 0 || 
                    _stricmp(pe.szExeFile, "ssh.exe") == 0) {
                    HANDLE proc = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                    if (proc) {
                        TerminateProcess(proc, 0);
                        CloseHandle(proc);
                    }
                }
            } while (Process32Next(snapshot, &pe));
        }
        CloseHandle(snapshot);
    }
    HW_SLEEP(500);  // Final wait for cleanup
#endif
    
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

