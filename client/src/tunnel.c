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

static HANDLE launch_process(const char* exe_path, const char* args) {
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    
    char cmdline[HW_BUFFER_SIZE];
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
    
    FILE* f = fopen(config_path, "w");
    if (!f) return -1;
    
    fprintf(f, "; HelloWorld Stunnel Configuration\n");
    fprintf(f, "; Auto-generated - do not edit manually\n\n");
#ifdef _WIN32
    // Windows stunnel doesn't use pid = in config
    // foreground must be 'yes' or 'quiet' on Windows, not 'no'
    fprintf(f, "foreground = quiet\n");
    fprintf(f, "debug = 7\n");
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
    fprintf(f, "accept = 127.0.0.1:%d\n", HW_LOCAL_PORT);
    fprintf(f, "connect = %s:%d\n", srv->host, srv->port);
    fprintf(f, "verify = 0\n");
    // Minimal config - verify=0 is enough for self-signed certs
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
    
    // Wait longer for stunnel to start and connect to server
    // Stunnel needs to establish TLS connection before binding to local port
    HW_SLEEP(5000);  // Increased from 2000 to 5000ms
    
    if (!is_process_running(ctx->stunnel_proc)) {
        DWORD exit_code = 0;
        GetExitCodeProcess(ctx->stunnel_proc, &exit_code);
        
        // Try to read stunnel error output if available
        char stunnel_error[512] = {0};
        char log_path[HW_MAX_PATH_LEN];
        snprintf(log_path, sizeof(log_path), "%s%sstunnel.log", ctx->config_dir, HW_PATH_SEP);
        FILE* log_file = fopen(log_path, "r");
        if (log_file) {
            fseek(log_file, -256, SEEK_END);
            fread(stunnel_error, 1, sizeof(stunnel_error) - 1, log_file);
            fclose(log_file);
        }
        
        hw_server_t* srv = &ctx->servers[ctx->current_server];
        snprintf(ctx->error_msg, sizeof(ctx->error_msg),
                 "STUNNEL FAILED (exit code: %lu)!\n\n"
                 "Config file: %s\n"
                 "Executable: %s\n\n"
                 "%s\n\n"
                 "Possible causes:\n"
                 "1. Invalid config file format\n"
                 "2. Port %d already in use\n"
                 "3. Cannot connect to server %s:%d\n"
                 "4. Stunnel installation corrupted\n\n"
                 "Try running manually:\n"
                 "\"%s\" \"%s\"",
                 exit_code, config_path, stunnel_exe,
                 stunnel_error[0] ? stunnel_error : "No additional error details available.",
                 HW_LOCAL_PORT, srv->host, srv->port, stunnel_exe, config_path);
        return -1;
    }
    
    // Verify port is actually listening
    // In client mode, stunnel only binds to local port AFTER successful TLS handshake
    // So we need to wait and retry multiple times
    hw_server_t* srv = &ctx->servers[ctx->current_server];
    int port_listening = 0;
    int max_retries = 6;  // Total wait: 5s initial + 6*2s = 17 seconds max
    int retry_count = 0;
    
#ifdef _WIN32
    // Check if port 2222 is listening by trying to connect
    // Retry multiple times as stunnel needs to complete TLS handshake first
    while (!port_listening && retry_count < max_retries && is_process_running(ctx->stunnel_proc)) {
        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock != INVALID_SOCKET) {
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = inet_addr("127.0.0.1");
            addr.sin_port = htons(HW_LOCAL_PORT);
            
            // Set socket to non-blocking for quick check
            u_long mode = 1;
            ioctlsocket(sock, FIONBIO, &mode);
            
            int result = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
            if (result == 0 || WSAGetLastError() == WSAEISCONN) {
                port_listening = 1;
            }
            closesocket(sock);
        }
        
        if (!port_listening) {
            retry_count++;
            if (retry_count < max_retries) {
                HW_SLEEP(2000);  // Wait 2 seconds between retries
            }
        }
    }
    
    if (!port_listening && is_process_running(ctx->stunnel_proc)) {
        // Port still not listening but process running - might be connection issue
        // Check stunnel logs for more details
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
                 "Stunnel started but port %d not listening after %d seconds.\n\n"
                 "Stunnel process is running but hasn't bound to local port.\n"
                 "This usually means stunnel cannot complete TLS handshake with server.\n\n"
                 "Server: %s:%d\n"
                 "Local port: %d\n\n"
                 "%s\n\n"
                 "Possible causes:\n"
                 "1. Cannot connect to server %s:%d (network/firewall issue)\n"
                 "2. Server stunnel not running or rejecting connections\n"
                 "3. TLS handshake failing (check server logs)\n"
                 "4. Server certificate/configuration mismatch\n\n"
                 "Troubleshooting:\n"
                 "- Check server: ssh into server and run 'helloworld-status'\n"
                 "- Check server logs: sudo tail -50 /var/log/helloworld-stunnel.log\n"
                 "- Test manually: \"C:\\Program Files (x86)\\stunnel\\bin\\stunnel.exe\" \"%s\"",
                 HW_LOCAL_PORT, 5 + (retry_count * 2),
                 srv->host, srv->port, HW_LOCAL_PORT,
                 stunnel_log[0] ? stunnel_log : "No stunnel log entries found.",
                 srv->host, srv->port, log_path);
        kill_process(ctx->stunnel_proc);
        ctx->stunnel_proc = HW_INVALID_PROCESS;
        return -1;
    } else if (!is_process_running(ctx->stunnel_proc)) {
        // Process exited - already handled above
        return -1;
    }
#endif
    
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
             "-o ConnectTimeout=10 "
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
             "-o ConnectTimeout=10 "
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
    
    HW_SLEEP(3000);
    
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
        
        snprintf(ctx->error_msg, sizeof(ctx->error_msg),
                 "SSH CONNECTION FAILED (exit code: %lu)!\n\n"
                 "=== DIAGNOSTIC INFO ===\n"
                 "SSH Command: ssh -N -D %d -p %d -i \"%s\" %s@127.0.0.1\n"
                 "Stunnel running: %s\n"
                 "Stunnel should forward: localhost:%d -> %s:%d\n\n"
                 "=== EXIT CODE %lu MEANS ===\n"
                 "%s\n\n"
                 "=== TROUBLESHOOTING ===\n"
                 "1. Test stunnel connection:\n"
                 "   - Check if port %d is listening: netstat -an | findstr :%d\n"
                 "   - Stunnel should forward to %s:443\n\n"
                 "2. Test SSH key:\n"
                 "   - Key file: %s\n"
                 "   - Test direct: ssh -i \"%s\" %s@%s\n"
                 "   - If direct works, stunnel is the problem\n\n"
                 "3. Check stunnel logs:\n"
                 "   - Look for errors in stunnel output\n"
                 "   - Verify stunnel config: %s\\stunnel.conf\n\n"
                 "4. Verify server:\n"
                 "   - Server: %s:%d\n"
                 "   - Username: %s\n"
                 "   - Key authorized: Check ~/.ssh/authorized_keys on server",
                 exit_code, HW_SOCKS_PORT, HW_LOCAL_PORT, srv->key_path, srv->username,
                 stunnel_running ? "YES" : "NO",
                 HW_LOCAL_PORT, srv->host, srv->port,
                 exit_code,
                 exit_code == 255 ? "Connection refused, authentication failed, or host key verification failed" :
                 exit_code == 1 ? "General SSH error" :
                 exit_code == 2 ? "SSH configuration error" :
                 "Unknown SSH error",
                 HW_LOCAL_PORT, HW_LOCAL_PORT, srv->host,
                 srv->key_path, srv->key_path, srv->username, srv->host,
                 ctx->config_dir, srv->host, srv->port, srv->username);
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

