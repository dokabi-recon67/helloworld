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
    
#ifdef _WIN32
    HINTERNET inet = InternetOpenA("HelloWorld/1.0", INTERNET_OPEN_TYPE_PRECONFIG,
                                   NULL, NULL, 0);
    if (!inet) return -1;
    
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
    FILE* fp = popen("curl -s --socks5 127.0.0.1:1080 http://api.ipify.org 2>/dev/null", "r");
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

