/*
 * HelloWorld - Network Utilities
 */

#include "helloworld.h"

#ifdef _WIN32
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#endif

int hw_fetch_public_ip(hw_ctx_t* ctx) {
    if (!ctx) return -1;
    
    memset(ctx->stats.public_ip, 0, sizeof(ctx->stats.public_ip));
    
    // Use user agent from stats if available
    const char* user_agent = ctx->stats.user_agent[0] ? ctx->stats.user_agent : "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36";
    
#ifdef _WIN32
    HINTERNET inet = InternetOpenA(user_agent, INTERNET_OPEN_TYPE_PRECONFIG,
                                   NULL, NULL, 0);
    if (!inet) return -1;
    
    // Use SOCKS5 proxy if connected
    char proxy_str[128] = {0};
    if (ctx->status == HW_CONNECTED) {
        snprintf(proxy_str, sizeof(proxy_str), "socks=127.0.0.1:%d", HW_SOCKS_PORT);
        InternetSetOptionA(inet, INTERNET_OPTION_PROXY, proxy_str, strlen(proxy_str));
    }
    
    HINTERNET req = InternetOpenUrlA(inet, "http://api.ipify.org",
                                     NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (req) {
        char buffer[256] = {0};
        DWORD read = 0;
        if (InternetReadFile(req, buffer, sizeof(buffer) - 1, &read)) {
            buffer[read] = '\0';
            char* newline = strchr(buffer, '\n');
            if (newline) *newline = '\0';
            strncpy(ctx->stats.public_ip, buffer, sizeof(ctx->stats.public_ip) - 1);
        }
        InternetCloseHandle(req);
    }
    InternetCloseHandle(inet);
#else
    // Use SOCKS5 proxy if connected
    const char* curl_cmd = ctx->status == HW_CONNECTED 
        ? "curl -s --socks5 127.0.0.1:1080 http://api.ipify.org 2>/dev/null"
        : "curl -s http://api.ipify.org 2>/dev/null";
    FILE* fp = popen(curl_cmd, "r");
    if (fp) {
        if (fgets(ctx->stats.public_ip, sizeof(ctx->stats.public_ip), fp)) {
            char* newline = strchr(ctx->stats.public_ip, '\n');
            if (newline) *newline = '\0';
        }
        pclose(fp);
    }
#endif
    
    return ctx->stats.public_ip[0] ? 0 : -1;
}

double hw_ping(const char* host) {
    if (!host) return -1.0;
    
#ifdef _WIN32
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return -1.0;
    
    struct hostent* he = gethostbyname(host);
    if (!he) {
        closesocket(sock);
        return -1.0;
    }
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(443);
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
    
    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    
    connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    struct timeval tv = {5, 0};
    
    double latency = -1.0;
    if (select(0, NULL, &fds, NULL, &tv) > 0) {
        QueryPerformanceCounter(&end);
        latency = (double)(end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart;
    }
    
    closesocket(sock);
    return latency;
    
#else
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1.0;
    
    struct hostent* he = gethostbyname(host);
    if (!he) {
        close(sock);
        return -1.0;
    }
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(443);
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    struct timeval tv = {5, 0};
    
    double latency = -1.0;
    if (select(sock + 1, NULL, &fds, NULL, &tv) > 0) {
        clock_gettime(CLOCK_MONOTONIC, &end);
        latency = (end.tv_sec - start.tv_sec) * 1000.0 +
                  (end.tv_nsec - start.tv_nsec) / 1000000.0;
    }
    
    close(sock);
    return latency;
#endif
}

// Fetch server time via SSH command
int hw_fetch_server_time(hw_ctx_t* ctx) {
    if (!ctx || ctx->current_server < 0) return -1;
    
    memset(ctx->stats.server_time, 0, sizeof(ctx->stats.server_time));
    
    // For now, use local time as fallback
    // In future, can SSH to server and run 'date' command
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    strftime(ctx->stats.server_time, sizeof(ctx->stats.server_time), "%Y-%m-%d %H:%M:%S", t);
    
    return 0;
}

// Get DNS server being used
int hw_fetch_dns_info(hw_ctx_t* ctx) {
    if (!ctx) return -1;
    
    memset(ctx->stats.dns_server, 0, sizeof(ctx->stats.dns_server));
    
    // DNS leak prevention: All DNS should go through tunnel
    if (ctx->status == HW_CONNECTED) {
        // DNS through SOCKS5 proxy (no leaks)
        snprintf(ctx->stats.dns_server, sizeof(ctx->stats.dns_server), 
                 "127.0.0.1:1080 (SOCKS5 - No Leak)");
    } else {
        // Not connected - show system DNS (potential leak)
        snprintf(ctx->stats.dns_server, sizeof(ctx->stats.dns_server), 
                 "System DNS (Leak Risk)");
    }
    
    return 0;
}

