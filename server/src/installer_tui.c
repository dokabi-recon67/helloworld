/*
 * HelloWorld Server - Interactive Installer TUI
 * Terminal-based setup wizard for the server
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"
#define BOLD    "\033[1m"

#define BOX_TL  "+"
#define BOX_TR  "+"
#define BOX_BL  "+"
#define BOX_BR  "+"
#define BOX_H   "-"
#define BOX_V   "|"

static void clear_screen(void) {
    printf("\033[2J\033[H");
}

static void draw_box(int width, int height) {
    printf(CYAN);
    printf("%s", BOX_TL);
    for (int i = 0; i < width - 2; i++) printf("%s", BOX_H);
    printf("%s\n", BOX_TR);
    
    for (int i = 0; i < height - 2; i++) {
        printf("%s", BOX_V);
        for (int j = 0; j < width - 2; j++) printf(" ");
        printf("%s\n", BOX_V);
    }
    
    printf("%s", BOX_BL);
    for (int i = 0; i < width - 2; i++) printf("%s", BOX_H);
    printf("%s\n" RESET, BOX_BR);
}

static void print_header(void) {
    clear_screen();
    printf("\n");
    printf(BOLD CYAN);
    printf("  +------------------------------------------+\n");
    printf("  |                                          |\n");
    printf("  |       HelloWorld Server Setup            |\n");
    printf("  |       DPI-Resistant Tunnel               |\n");
    printf("  |                                          |\n");
    printf("  +------------------------------------------+\n");
    printf(RESET "\n");
}

static void print_menu(const char* title, const char** options, int count, int selected) {
    printf("\n  " BOLD "%s" RESET "\n\n", title);
    
    for (int i = 0; i < count; i++) {
        if (i == selected) {
            printf("  " CYAN "> [*] %s" RESET "\n", options[i]);
        } else {
            printf("    [ ] %s\n", options[i]);
        }
    }
    printf("\n");
}

static int run_cmd(const char* cmd) {
    return system(cmd);
}

static int run_cmd_quiet(const char* cmd) {
    char full_cmd[1024];
    snprintf(full_cmd, sizeof(full_cmd), "%s > /dev/null 2>&1", cmd);
    return system(full_cmd);
}

static void print_progress(const char* task, int done) {
    if (done) {
        printf("  " GREEN "[OK]" RESET " %s\n", task);
    } else {
        printf("  " YELLOW "[..]" RESET " %s", task);
        fflush(stdout);
    }
}

static void finish_progress(int success) {
    if (success) {
        printf("\r  " GREEN "[OK]" RESET "\n");
    } else {
        printf("\r  " RED "[FAIL]" RESET "\n");
    }
}

static int detect_os(char* os_name, size_t size) {
    if (access("/etc/debian_version", F_OK) == 0) {
        strncpy(os_name, "debian", size);
        return 0;
    } else if (access("/etc/redhat-release", F_OK) == 0) {
        strncpy(os_name, "redhat", size);
        return 0;
    } else if (access("/etc/arch-release", F_OK) == 0) {
        strncpy(os_name, "arch", size);
        return 0;
    }
    return -1;
}

static int install_dependencies(const char* os) {
    print_progress("Installing stunnel...", 0);
    int ret;
    if (strcmp(os, "debian") == 0) {
        ret = run_cmd_quiet("apt-get update && apt-get install -y stunnel4 openssh-server curl openssl");
    } else if (strcmp(os, "redhat") == 0) {
        ret = run_cmd_quiet("yum install -y stunnel openssh-server curl openssl");
    } else if (strcmp(os, "arch") == 0) {
        ret = run_cmd_quiet("pacman -Sy --noconfirm stunnel openssh curl openssl");
    } else {
        return -1;
    }
    finish_progress(ret == 0);
    return ret;
}

static int generate_certificates(void) {
    print_progress("Generating TLS certificate...", 0);
    
    run_cmd_quiet("mkdir -p /etc/helloworld");
    
    int ret = run_cmd_quiet(
        "openssl req -new -x509 -days 3650 -nodes "
        "-out /etc/helloworld/server.pem "
        "-keyout /etc/helloworld/server.key "
        "-subj '/CN=localhost'"
    );
    
    run_cmd_quiet("chmod 600 /etc/helloworld/server.key");
    finish_progress(ret == 0);
    return ret;
}

static int configure_stunnel(void) {
    print_progress("Configuring stunnel...", 0);
    
    FILE* f = fopen("/etc/helloworld/stunnel.conf", "w");
    if (!f) {
        finish_progress(0);
        return -1;
    }
    
    fprintf(f, "pid = /var/run/stunnel-hw.pid\n");
    fprintf(f, "cert = /etc/helloworld/server.pem\n");
    fprintf(f, "key = /etc/helloworld/server.key\n");
    fprintf(f, "\n");
    fprintf(f, "[ssh]\n");
    fprintf(f, "accept = 0.0.0.0:443\n");
    fprintf(f, "connect = 127.0.0.1:22\n");
    
    fclose(f);
    finish_progress(1);
    return 0;
}

static int configure_ssh(void) {
    print_progress("Configuring SSH...", 0);
    
    run_cmd_quiet("systemctl enable ssh 2>/dev/null || systemctl enable sshd");
    int ret = run_cmd_quiet("systemctl start ssh 2>/dev/null || systemctl start sshd");
    
    finish_progress(ret == 0);
    return ret;
}

static int configure_firewall(void) {
    print_progress("Opening port 443...", 0);
    
    int ret = 0;
    if (run_cmd_quiet("which ufw") == 0) {
        ret = run_cmd_quiet("ufw allow 443/tcp");
    } else if (run_cmd_quiet("which firewall-cmd") == 0) {
        run_cmd_quiet("firewall-cmd --permanent --add-port=443/tcp");
        ret = run_cmd_quiet("firewall-cmd --reload");
    }
    
    finish_progress(1);
    return ret;
}

static int enable_forwarding(void) {
    print_progress("Enabling IP forwarding...", 0);
    
    run_cmd_quiet("echo 1 > /proc/sys/net/ipv4/ip_forward");
    
    FILE* f = fopen("/etc/sysctl.d/99-helloworld.conf", "w");
    if (f) {
        fprintf(f, "net.ipv4.ip_forward = 1\n");
        fclose(f);
    }
    
    finish_progress(1);
    return 0;
}

static int start_server(void) {
    print_progress("Starting server...", 0);
    
    int ret = run_cmd_quiet("stunnel /etc/helloworld/stunnel.conf");
    
    finish_progress(ret == 0);
    return ret;
}

static void show_completion(void) {
    char ip[64] = {0};
    FILE* fp = popen("curl -s http://api.ipify.org 2>/dev/null", "r");
    if (fp) {
        if (fgets(ip, sizeof(ip), fp)) {
            char* nl = strchr(ip, '\n');
            if (nl) *nl = '\0';
        }
        pclose(fp);
    }
    
    printf("\n");
    printf(GREEN);
    printf("  +------------------------------------------+\n");
    printf("  |                                          |\n");
    printf("  |       Setup Complete!                    |\n");
    printf("  |                                          |\n");
    printf("  +------------------------------------------+\n");
    printf(RESET);
    printf("\n");
    printf("  Your server is running.\n\n");
    printf("  Server IP: " CYAN "%s" RESET "\n\n", ip[0] ? ip : "(unknown)");
    printf("  Next steps:\n");
    printf("  1. Add your SSH public key:\n");
    printf("     " CYAN "echo 'your-key' >> ~/.ssh/authorized_keys" RESET "\n\n");
    printf("  2. Download HelloWorld client on your PC/Mac\n\n");
    printf("  3. Connect using:\n");
    printf("     Host: " CYAN "%s" RESET "\n", ip[0] ? ip : "YOUR_IP");
    printf("     Port: " CYAN "443" RESET "\n");
    printf("     User: " CYAN "root" RESET "\n\n");
}

static int run_full_setup(void) {
    char os[32];
    
    printf("\n  " BOLD "Starting installation..." RESET "\n\n");
    
    if (detect_os(os, sizeof(os)) != 0) {
        printf(RED "  Unsupported operating system.\n" RESET);
        return -1;
    }
    
    if (install_dependencies(os) != 0) {
        printf(RED "  Failed to install dependencies.\n" RESET);
    }
    
    if (generate_certificates() != 0) {
        printf(RED "  Failed to generate certificates.\n" RESET);
        return -1;
    }
    
    if (configure_stunnel() != 0) {
        printf(RED "  Failed to configure stunnel.\n" RESET);
        return -1;
    }
    
    configure_ssh();
    configure_firewall();
    enable_forwarding();
    
    if (start_server() != 0) {
        printf(YELLOW "  Warning: Could not start server automatically.\n" RESET);
        printf("  Run: " CYAN "stunnel /etc/helloworld/stunnel.conf" RESET "\n");
    }
    
    show_completion();
    return 0;
}

int main(int argc, char* argv[]) {
    if (geteuid() != 0) {
        printf(RED "This installer must be run as root.\n" RESET);
        printf("Usage: sudo %s\n", argv[0]);
        return 1;
    }
    
    print_header();
    
    printf("  This wizard will set up HelloWorld server on your VM.\n\n");
    printf("  The following will be installed/configured:\n");
    printf("    - stunnel (TLS wrapper)\n");
    printf("    - SSH server\n");
    printf("    - Firewall rules (port 443)\n");
    printf("    - IP forwarding\n\n");
    
    printf("  Press " CYAN "ENTER" RESET " to continue or " CYAN "Ctrl+C" RESET " to cancel...");
    getchar();
    
    int result = run_full_setup();
    
    if (result == 0) {
        printf("\n  Press ENTER to exit...");
        getchar();
    }
    
    return result;
}

