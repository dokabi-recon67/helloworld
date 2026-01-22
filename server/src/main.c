/*
 * HelloWorld Server - Main Entry Point
 * Command line interface for server management
 */

#include "hwserver.h"
#include <getopt.h>

static void print_usage(const char* prog) {
    printf("HelloWorld Server v%s\n\n", HWS_VERSION);
    printf("Usage: %s [options] <command>\n\n", prog);
    printf("Commands:\n");
    printf("  start          Start the server\n");
    printf("  stop           Stop the server\n");
    printf("  status         Show server status\n");
    printf("  setup          Initial setup\n");
    printf("  tor            Switch to Tor exit mode\n");
    printf("  direct         Switch to direct exit mode\n");
    printf("  rotate         Rotate Tor circuit\n");
    printf("  ip             Show current public IP\n");
    printf("\n");
    printf("Options:\n");
    printf("  -h, --help     Show this help\n");
    printf("  -d, --daemon   Run as daemon\n");
    printf("\n");
}

static int cmd_start(hws_ctx_t* ctx) {
    printf("Starting HelloWorld server...\n");
    
    if (hws_start(ctx) != 0) {
        printf("Error: %s\n", ctx->error_msg);
        return 1;
    }
    
    printf("Server started successfully\n");
    printf("Public IP: %s\n", ctx->public_ip);
    return 0;
}

static int cmd_stop(hws_ctx_t* ctx) {
    printf("Stopping HelloWorld server...\n");
    hws_stop(ctx);
    printf("Server stopped\n");
    return 0;
}

static int cmd_status(hws_ctx_t* ctx) {
    hws_fetch_public_ip(ctx);
    
    printf("HelloWorld Server Status\n");
    printf("========================\n");
    printf("Status:    %s\n", hws_status_str(ctx->status));
    printf("Exit Mode: %s\n", hws_mode_str(ctx->exit_mode));
    printf("Public IP: %s\n", ctx->public_ip[0] ? ctx->public_ip : "N/A");
    
    if (ctx->status == HWS_RUNNING) {
        time_t elapsed = time(NULL) - ctx->start_time;
        int hours = (int)(elapsed / 3600);
        int mins = (int)((elapsed % 3600) / 60);
        printf("Uptime:    %02d:%02d\n", hours, mins);
    }
    
    return 0;
}

static int cmd_setup(hws_ctx_t* ctx) {
    printf("Running initial setup...\n");
    
    printf("Installing dependencies...\n");
    int ret = system("apt-get update && apt-get install -y stunnel4 openssh-server tor curl 2>/dev/null || "
                     "yum install -y stunnel openssh-server tor curl 2>/dev/null");
    (void)ret;
    
    printf("Configuring stunnel...\n");
    hws_setup_stunnel(ctx);
    
    printf("Configuring SSH...\n");
    hws_setup_ssh(ctx);
    
    printf("Opening firewall port 443...\n");
    system("ufw allow 443/tcp 2>/dev/null");
    system("firewall-cmd --permanent --add-port=443/tcp 2>/dev/null");
    system("firewall-cmd --reload 2>/dev/null");
    
    printf("\nSetup complete!\n");
    printf("Add your SSH public key to ~/.ssh/authorized_keys\n");
    printf("Then run: helloworld-server start\n");
    
    return 0;
}

static int cmd_tor(hws_ctx_t* ctx) {
    printf("Switching to Tor exit mode...\n");
    hws_set_exit_mode(ctx, HWS_MODE_TOR);
    printf("Exit mode: Tor\n");
    return 0;
}

static int cmd_direct(hws_ctx_t* ctx) {
    printf("Switching to direct exit mode...\n");
    hws_set_exit_mode(ctx, HWS_MODE_DIRECT);
    printf("Exit mode: Direct\n");
    return 0;
}

static int cmd_rotate(hws_ctx_t* ctx) {
    if (ctx->exit_mode != HWS_MODE_TOR) {
        printf("Error: Not in Tor mode\n");
        return 1;
    }
    
    printf("Rotating Tor circuit...\n");
    hws_rotate_tor_circuit(ctx);
    
    sleep(2);
    hws_fetch_public_ip(ctx);
    printf("New IP: %s\n", ctx->public_ip);
    
    return 0;
}

static int cmd_ip(hws_ctx_t* ctx) {
    hws_fetch_public_ip(ctx);
    printf("%s\n", ctx->public_ip);
    return 0;
}

int main(int argc, char* argv[]) {
    static struct option long_opts[] = {
        {"help",   no_argument, 0, 'h'},
        {"daemon", no_argument, 0, 'd'},
        {0, 0, 0, 0}
    };
    
    int daemon_mode = 0;
    int opt;
    
    while ((opt = getopt_long(argc, argv, "hd", long_opts, NULL)) != -1) {
        switch (opt) {
            case 'h':
                print_usage(argv[0]);
                return 0;
            case 'd':
                daemon_mode = 1;
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    if (optind >= argc) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char* command = argv[optind];
    
    if (geteuid() != 0) {
        fprintf(stderr, "Error: This program must be run as root\n");
        return 1;
    }
    
    hws_ctx_t* ctx = hws_create();
    if (!ctx) {
        fprintf(stderr, "Error: Failed to initialize\n");
        return 1;
    }
    
    int result = 0;
    
    if (strcmp(command, "start") == 0) {
        result = cmd_start(ctx);
        if (result == 0 && daemon_mode) {
            printf("Running in background...\n");
            while (ctx->status == HWS_RUNNING) {
                sleep(10);
            }
        }
    } else if (strcmp(command, "stop") == 0) {
        result = cmd_stop(ctx);
    } else if (strcmp(command, "status") == 0) {
        result = cmd_status(ctx);
    } else if (strcmp(command, "setup") == 0) {
        result = cmd_setup(ctx);
    } else if (strcmp(command, "tor") == 0) {
        result = cmd_tor(ctx);
    } else if (strcmp(command, "direct") == 0) {
        result = cmd_direct(ctx);
    } else if (strcmp(command, "rotate") == 0) {
        result = cmd_rotate(ctx);
    } else if (strcmp(command, "ip") == 0) {
        result = cmd_ip(ctx);
    } else {
        fprintf(stderr, "Unknown command: %s\n", command);
        print_usage(argv[0]);
        result = 1;
    }
    
    hws_destroy(ctx);
    return result;
}

