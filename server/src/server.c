/*
 * HelloWorld Server - Core Implementation
 */

#include "hwserver.h"

static int run_command(const char* cmd) {
    int ret = system(cmd);
    return WEXITSTATUS(ret);
}

static int run_command_output(const char* cmd, char* output, size_t size) {
    FILE* fp = popen(cmd, "r");
    if (!fp) return -1;
    
    if (output && size > 0) {
        output[0] = '\0';
        if (fgets(output, size, fp)) {
            char* nl = strchr(output, '\n');
            if (nl) *nl = '\0';
        }
    }
    
    return pclose(fp);
}

static pid_t start_daemon(const char* cmd) {
    pid_t pid = fork();
    if (pid == 0) {
        setsid();
        freopen("/dev/null", "r", stdin);
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);
        execl("/bin/sh", "sh", "-c", cmd, NULL);
        _exit(127);
    }
    return pid;
}

static void stop_daemon(pid_t pid) {
    if (pid > 0) {
        kill(pid, SIGTERM);
        usleep(500000);
        kill(pid, SIGKILL);
        waitpid(pid, NULL, WNOHANG);
    }
}

hws_ctx_t* hws_create(void) {
    hws_ctx_t* ctx = (hws_ctx_t*)calloc(1, sizeof(hws_ctx_t));
    if (!ctx) return NULL;
    
    ctx->status = HWS_STOPPED;
    ctx->exit_mode = HWS_MODE_DIRECT;
    ctx->stunnel_pid = -1;
    ctx->tor_pid = -1;
    
    snprintf(ctx->config_dir, sizeof(ctx->config_dir), "/etc/helloworld");
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "mkdir -p %s", ctx->config_dir);
    run_command(cmd);
    
    return ctx;
}

void hws_destroy(hws_ctx_t* ctx) {
    if (!ctx) return;
    
    if (ctx->status == HWS_RUNNING) {
        hws_stop(ctx);
    }
    
    free(ctx);
}

const char* hws_status_str(hws_status_t status) {
    switch (status) {
        case HWS_STOPPED: return "Stopped";
        case HWS_RUNNING: return "Running";
        case HWS_ERROR:   return "Error";
        default:          return "Unknown";
    }
}

const char* hws_mode_str(hws_exit_mode_t mode) {
    switch (mode) {
        case HWS_MODE_DIRECT:    return "Direct";
        case HWS_MODE_VPN_CYCLE: return "VPN Cycle";
        case HWS_MODE_TOR:       return "Tor";
        default:                 return "Unknown";
    }
}

void hws_log(const char* level, const char* fmt, ...) {
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

int hws_fetch_public_ip(hws_ctx_t* ctx) {
    if (!ctx) return -1;
    
    memset(ctx->public_ip, 0, sizeof(ctx->public_ip));
    run_command_output("curl -s http://api.ipify.org 2>/dev/null", 
                       ctx->public_ip, sizeof(ctx->public_ip));
    
    return ctx->public_ip[0] ? 0 : -1;
}

int hws_setup_stunnel(hws_ctx_t* ctx) {
    if (!ctx) return -1;
    
    char conf_path[HWS_MAX_PATH];
    snprintf(conf_path, sizeof(conf_path), "%s/stunnel.conf", ctx->config_dir);
    
    FILE* f = fopen(conf_path, "w");
    if (!f) {
        snprintf(ctx->error_msg, sizeof(ctx->error_msg), "Cannot write stunnel config");
        return -1;
    }
    
    fprintf(f, "pid = /var/run/stunnel.pid\n");
    fprintf(f, "cert = %s/server.pem\n", ctx->config_dir);
    fprintf(f, "key = %s/server.key\n", ctx->config_dir);
    fprintf(f, "\n");
    fprintf(f, "[ssh]\n");
    fprintf(f, "accept = 0.0.0.0:%d\n", HWS_STUNNEL_PORT);
    fprintf(f, "connect = 127.0.0.1:%d\n", HWS_SSH_PORT);
    
    fclose(f);
    
    char cert_path[HWS_MAX_PATH];
    snprintf(cert_path, sizeof(cert_path), "%s/server.pem", ctx->config_dir);
    
    FILE* test = fopen(cert_path, "r");
    if (!test) {
        char cmd[HWS_BUFFER_SIZE];
        snprintf(cmd, sizeof(cmd),
                 "openssl req -new -x509 -days 365 -nodes "
                 "-out %s/server.pem -keyout %s/server.key "
                 "-subj '/CN=localhost' 2>/dev/null",
                 ctx->config_dir, ctx->config_dir);
        run_command(cmd);
    } else {
        fclose(test);
    }
    
    return 0;
}

int hws_setup_ssh(hws_ctx_t* ctx) {
    if (!ctx) return -1;
    
    run_command("systemctl enable ssh 2>/dev/null || systemctl enable sshd 2>/dev/null");
    run_command("systemctl start ssh 2>/dev/null || systemctl start sshd 2>/dev/null");
    
    char conf_path[HWS_MAX_PATH];
    snprintf(conf_path, sizeof(conf_path), "%s/sshd_config_helloworld", ctx->config_dir);
    
    FILE* f = fopen(conf_path, "w");
    if (f) {
        fprintf(f, "Port %d\n", HWS_SSH_PORT);
        fprintf(f, "ListenAddress 127.0.0.1\n");
        fprintf(f, "PermitRootLogin prohibit-password\n");
        fprintf(f, "PasswordAuthentication no\n");
        fprintf(f, "PubkeyAuthentication yes\n");
        fprintf(f, "AllowTcpForwarding yes\n");
        fprintf(f, "GatewayPorts no\n");
        fprintf(f, "PermitTunnel yes\n");
        fclose(f);
    }
    
    return 0;
}

int hws_setup_nat(hws_ctx_t* ctx) {
    if (!ctx) return -1;
    
    run_command("echo 1 > /proc/sys/net/ipv4/ip_forward");
    
    run_command("iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE 2>/dev/null");
    run_command("iptables -A FORWARD -i eth0 -o eth0 -m state --state RELATED,ESTABLISHED -j ACCEPT 2>/dev/null");
    run_command("iptables -A FORWARD -i tun0 -o eth0 -j ACCEPT 2>/dev/null");
    
    run_command("iptables -t nat -A POSTROUTING -o ens3 -j MASQUERADE 2>/dev/null");
    run_command("iptables -A FORWARD -i ens3 -o ens3 -m state --state RELATED,ESTABLISHED -j ACCEPT 2>/dev/null");
    
    ctx->nat_configured = true;
    hws_log("INFO", "NAT configured");
    
    return 0;
}

int hws_clear_nat(hws_ctx_t* ctx) {
    if (!ctx) return -1;
    
    run_command("iptables -t nat -F");
    run_command("iptables -F FORWARD");
    
    ctx->nat_configured = false;
    return 0;
}

int hws_start_tor(hws_ctx_t* ctx) {
    if (!ctx) return -1;
    
    char torrc_path[HWS_MAX_PATH];
    snprintf(torrc_path, sizeof(torrc_path), "%s/torrc", ctx->config_dir);
    
    FILE* f = fopen(torrc_path, "w");
    if (f) {
        fprintf(f, "SocksPort 9050\n");
        fprintf(f, "ControlPort 9051\n");
        fprintf(f, "CookieAuthentication 0\n");
        fprintf(f, "HashedControlPassword \n");
        fprintf(f, "MaxCircuitDirtiness 600\n");
        fclose(f);
    }
    
    char cmd[HWS_BUFFER_SIZE];
    snprintf(cmd, sizeof(cmd), "tor -f %s &", torrc_path);
    ctx->tor_pid = start_daemon(cmd);
    
    sleep(3);
    
    hws_log("INFO", "Tor started");
    return 0;
}

int hws_stop_tor(hws_ctx_t* ctx) {
    if (!ctx) return -1;
    
    stop_daemon(ctx->tor_pid);
    ctx->tor_pid = -1;
    
    run_command("pkill -9 tor 2>/dev/null");
    
    return 0;
}

int hws_rotate_tor_circuit(hws_ctx_t* ctx) {
    if (!ctx || ctx->exit_mode != HWS_MODE_TOR) return -1;
    
    run_command("echo -e 'AUTHENTICATE\r\nSIGNAL NEWNYM\r\nQUIT' | nc 127.0.0.1 9051");
    
    hws_log("INFO", "Tor circuit rotated");
    return 0;
}

int hws_set_exit_mode(hws_ctx_t* ctx, hws_exit_mode_t mode) {
    if (!ctx) return -1;
    
    if (ctx->exit_mode == HWS_MODE_TOR && mode != HWS_MODE_TOR) {
        hws_stop_tor(ctx);
    }
    
    ctx->exit_mode = mode;
    
    if (mode == HWS_MODE_TOR) {
        hws_start_tor(ctx);
    }
    
    hws_log("INFO", "Exit mode set to %s", hws_mode_str(mode));
    return 0;
}

int hws_start(hws_ctx_t* ctx) {
    if (!ctx) return -1;
    if (ctx->status == HWS_RUNNING) return 0;
    
    hws_log("INFO", "Starting HelloWorld server");
    
    if (hws_setup_stunnel(ctx) != 0) {
        ctx->status = HWS_ERROR;
        return -1;
    }
    
    if (hws_setup_ssh(ctx) != 0) {
        ctx->status = HWS_ERROR;
        return -1;
    }
    
    if (hws_setup_nat(ctx) != 0) {
        ctx->status = HWS_ERROR;
        return -1;
    }
    
    char cmd[HWS_BUFFER_SIZE];
    snprintf(cmd, sizeof(cmd), "stunnel %s/stunnel.conf", ctx->config_dir);
    ctx->stunnel_pid = start_daemon(cmd);
    
    sleep(1);
    
    ctx->start_time = time(NULL);
    ctx->status = HWS_RUNNING;
    
    hws_fetch_public_ip(ctx);
    hws_log("INFO", "Server running - Public IP: %s", ctx->public_ip);
    
    return 0;
}

int hws_stop(hws_ctx_t* ctx) {
    if (!ctx) return -1;
    if (ctx->status != HWS_RUNNING) return 0;
    
    hws_log("INFO", "Stopping HelloWorld server");
    
    if (ctx->exit_mode == HWS_MODE_TOR) {
        hws_stop_tor(ctx);
    }
    
    stop_daemon(ctx->stunnel_pid);
    ctx->stunnel_pid = -1;
    
    run_command("pkill -9 stunnel 2>/dev/null");
    
    hws_clear_nat(ctx);
    
    ctx->status = HWS_STOPPED;
    return 0;
}

