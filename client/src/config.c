/*
 * HelloWorld - Configuration Management
 */

#include "helloworld.h"

static int parse_line(char* line, char* key, char* value, size_t size) {
    char* eq = strchr(line, '=');
    if (!eq) return -1;
    
    *eq = '\0';
    char* k = line;
    char* v = eq + 1;
    
    while (*k == ' ' || *k == '\t') k++;
    char* ke = k + strlen(k) - 1;
    while (ke > k && (*ke == ' ' || *ke == '\t' || *ke == '\n' || *ke == '\r')) *ke-- = '\0';
    
    while (*v == ' ' || *v == '\t') v++;
    char* ve = v + strlen(v) - 1;
    while (ve > v && (*ve == ' ' || *ve == '\t' || *ve == '\n' || *ve == '\r')) *ve-- = '\0';
    
    strncpy(key, k, size - 1);
    strncpy(value, v, size - 1);
    return 0;
}

int hw_load_config(hw_ctx_t* ctx) {
    if (!ctx) return -1;
    
    char path[HW_MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s%sconfig.txt", ctx->config_dir, HW_PATH_SEP);
    
    FILE* f = fopen(path, "r");
    if (!f) return -1;
    
    char line[512];
    char key[256], value[256];
    hw_server_t current = {0};
    int in_server = 0;
    
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        
        if (strncmp(line, "[server]", 8) == 0) {
            if (in_server && current.name[0]) {
                hw_add_server(ctx, current.name, current.host, 
                             current.port, current.key_path, current.username);
            }
            memset(&current, 0, sizeof(current));
            in_server = 1;
            continue;
        }
        
        if (parse_line(line, key, value, sizeof(key)) == 0) {
            if (in_server) {
                if (strcmp(key, "name") == 0) strncpy(current.name, value, HW_MAX_NAME_LEN - 1);
                else if (strcmp(key, "host") == 0) strncpy(current.host, value, HW_MAX_HOST_LEN - 1);
                else if (strcmp(key, "port") == 0) current.port = (uint16_t)atoi(value);
                else if (strcmp(key, "key") == 0) strncpy(current.key_path, value, HW_MAX_PATH_LEN - 1);
                else if (strcmp(key, "user") == 0) strncpy(current.username, value, HW_MAX_NAME_LEN - 1);
            } else {
                if (strcmp(key, "mode") == 0) {
                    ctx->mode = strcmp(value, "full") == 0 ? HW_MODE_FULL_TUNNEL : HW_MODE_PROXY;
                } else if (strcmp(key, "killswitch") == 0) {
                    ctx->kill_switch = strcmp(value, "1") == 0 || strcmp(value, "true") == 0;
                } else if (strcmp(key, "selected") == 0) {
                    ctx->current_server = atoi(value);
                }
            }
        }
    }
    
    if (in_server && current.name[0]) {
        hw_add_server(ctx, current.name, current.host,
                     current.port, current.key_path, current.username);
    }
    
    fclose(f);
    
    if (ctx->current_server >= ctx->server_count) {
        ctx->current_server = ctx->server_count > 0 ? 0 : -1;
    }
    
    return 0;
}

int hw_save_config(hw_ctx_t* ctx) {
    if (!ctx) return -1;
    
    char path[HW_MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s%sconfig.txt", ctx->config_dir, HW_PATH_SEP);
    
    FILE* f = fopen(path, "w");
    if (!f) return -1;
    
    fprintf(f, "# HelloWorld Configuration\n\n");
    fprintf(f, "mode = %s\n", ctx->mode == HW_MODE_FULL_TUNNEL ? "full" : "proxy");
    fprintf(f, "killswitch = %d\n", ctx->kill_switch ? 1 : 0);
    fprintf(f, "selected = %d\n", ctx->current_server);
    fprintf(f, "\n");
    
    for (int i = 0; i < ctx->server_count; i++) {
        hw_server_t* srv = &ctx->servers[i];
        fprintf(f, "[server]\n");
        fprintf(f, "name = %s\n", srv->name);
        fprintf(f, "host = %s\n", srv->host);
        fprintf(f, "port = %d\n", srv->port);
        fprintf(f, "key = %s\n", srv->key_path);
        fprintf(f, "user = %s\n", srv->username);
        fprintf(f, "\n");
    }
    
    fclose(f);
    return 0;
}

