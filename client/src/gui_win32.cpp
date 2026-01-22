/*
 * HelloWorld - Windows GUI
 * Modern UI with Smooth Graphics (GDI+), Toggle Switches, Eye Icon
 */

#ifdef _WIN32

#include "helloworld.h"
#include <commctrl.h>
#include <commdlg.h>
#include <windowsx.h>
#include <shlobj.h>
#include <wchar.h>
#include <objidl.h>
#include <gdiplus.h>
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

#define ID_TIMER_UPDATE  2001

#define CLR_BG_DARK      RGB(13, 17, 23)
#define CLR_BG_CARD      RGB(22, 27, 34)
#define CLR_BG_INPUT     RGB(33, 38, 45)
#define CLR_BORDER       RGB(48, 54, 61)
#define CLR_TEXT         RGB(230, 237, 243)
#define CLR_TEXT_DIM     RGB(139, 148, 158)
#define CLR_ACCENT       RGB(0, 255, 136)
#define CLR_ACCENT_DARK  RGB(0, 200, 100)
#define CLR_ERROR        RGB(255, 85, 85)
#define CLR_TOGGLE_OFF   RGB(60, 65, 75)
#define CLR_TOGGLE_ON    RGB(0, 200, 100)

static inline POINT MakePt(int x, int y) { POINT p = {x, y}; return p; }

static ULONG_PTR g_gdiplusToken = 0;
static hw_ctx_t* g_ctx = NULL;
static HWND g_hwnd = NULL;
static HICON g_icon = NULL;
static HFONT g_font_title = NULL;
static HFONT g_font_normal = NULL;
static HFONT g_font_button = NULL;
static HFONT g_font_small = NULL;
static HBRUSH g_brush_bg = NULL;
static HBRUSH g_brush_card = NULL;
static HBRUSH g_brush_input = NULL;
static HBRUSH g_brush_accent = NULL;

static int g_selected_server = 0;
static int g_full_tunnel = 0;
static int g_kill_switch = 0;
static int g_show_tutorial = 0;
static int g_tutorial_step = 0;
static int g_selected_user_agent = 0;
static int g_user_agent_dropdown_open = 0;

static RECT g_rect_title;
static RECT g_rect_server_label;
static RECT g_rect_server_box;
static RECT g_rect_server_dropdown;
static RECT g_rect_btn_add;
static RECT g_rect_btn_connect;
static RECT g_rect_toggle_full;
static RECT g_rect_toggle_kill;
static RECT g_rect_info_box;
static RECT g_rect_user_agent_label;
static RECT g_rect_user_agent_box;
static RECT g_rect_user_agent_dropdown;

static int g_hover_connect = 0;
static int g_hover_add = 0;
static int g_hover_dropdown = 0;
static int g_hover_toggle_full = 0;
static int g_hover_toggle_kill = 0;
static int g_dropdown_open = 0;
static int g_hover_user_agent = 0;

static char g_add_name[64] = {0};
static char g_add_host[256] = {0};
static char g_add_port[16] = "443";
static char g_add_key[512] = {0};
static char g_add_user[64] = "root";

// User agent list
#define MAX_USER_AGENTS 5000
static char g_user_agents[MAX_USER_AGENTS][512];
static int g_user_agent_count = 0;
static int g_randomize_user_agent = 0;  // 0 = use selected, 1 = randomize

// Load user agents from file
static void load_user_agents(void) {
    if (g_user_agent_count > 0) return;  // Already loaded
    
    char ua_path[512];
    // Try multiple locations
    snprintf(ua_path, sizeof(ua_path), "%s\\user_agents.txt", g_ctx ? g_ctx->config_dir : "");
    FILE* f = fopen(ua_path, "r");
    if (!f) {
        // Try resources directory
        char exe_path[MAX_PATH];
        GetModuleFileNameA(NULL, exe_path, MAX_PATH);
        char* last_slash = strrchr(exe_path, '\\');
        if (last_slash) {
            *last_slash = '\0';
            snprintf(ua_path, sizeof(ua_path), "%s\\resources\\user_agents.txt", exe_path);
            f = fopen(ua_path, "r");
        }
    }
    if (!f) {
        // Try current directory
        f = fopen("user_agents.txt", "r");
    }
    
    if (f) {
        char line[512];
        while (fgets(line, sizeof(line), f) && g_user_agent_count < MAX_USER_AGENTS) {
            // Remove newline
            char* nl = strchr(line, '\r');
            if (nl) *nl = '\0';
            nl = strchr(line, '\n');
            if (nl) *nl = '\0';
            
            // Skip empty lines
            if (line[0] == '\0') continue;
            
            strncpy(g_user_agents[g_user_agent_count], line, sizeof(g_user_agents[0]) - 1);
            g_user_agents[g_user_agent_count][sizeof(g_user_agents[0]) - 1] = '\0';
            g_user_agent_count++;
        }
        fclose(f);
    }
    
    // If no file found, add some defaults
    if (g_user_agent_count == 0) {
        strcpy(g_user_agents[0], "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
        strcpy(g_user_agents[1], "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36");
        strcpy(g_user_agents[2], "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36");
        g_user_agent_count = 3;
    }
}

// Get current user agent (selected or random)
static const char* get_current_user_agent(void) {
    if (g_user_agent_count == 0) load_user_agents();
    if (g_user_agent_count == 0) return "Mozilla/5.0";
    
    if (g_randomize_user_agent) {
        int idx = rand() % g_user_agent_count;
        return g_user_agents[idx];
    }
    
    if (g_selected_user_agent >= 0 && g_selected_user_agent < g_user_agent_count) {
        return g_user_agents[g_selected_user_agent];
    }
    
    return g_user_agents[0];
}

static const char* TUTORIAL_TITLES[] = {
    "Welcome to HelloWorld!",
    "Step 1: Get a Cloud VM",
    "Step 2: Set Up the Server",
    "Step 3: Add Your Server",
    "You're Ready!"
};

static const char* TUTORIAL_TEXTS[] = {
    "HelloWorld creates a secure, invisible tunnel between your PC and a cloud server you control.\n\n"
    "Your internet traffic will look like normal HTTPS - undetectable.\n\n"
    "Let's get you set up in 3 easy steps!",
    
    "First, you need a cloud VM (Virtual Machine).\n\n"
    "We recommend Oracle Cloud Free Tier - it's completely free!\n\n"
    "1. Go to cloud.oracle.com\n"
    "2. Create a free account\n"
    "3. Create a small Linux VM (Ubuntu)\n"
    "4. Save the SSH key file they give you\n"
    "5. Note your VM's IP address",
    
    "Now install HelloWorld on your VM.\n\n"
    "Connect to your VM and run this command:\n\n"
    "curl -sSL https://raw.githubusercontent.com/dokabi-recon67/helloworld/main/scripts/install_server.sh | sudo bash\n\n"
    "This sets up everything automatically!",
    
    "Click the + ADD SERVER button and enter:\n\n"
    "- Name: Anything you want\n"
    "- Host: Your VM's IP address\n"
    "- Port: 443\n"
    "- SSH Key: Browse to your key file\n"
    "- Username: root (or ubuntu)",
    
    "That's it! You're all set.\n\n"
    "Select your server and click CONNECT.\n\n"
    "To verify: visit api.ipify.org\n"
    "You should see your VM's IP!\n\n"
    "Enjoy your private, secure tunnel!"
};

static HICON create_eye_icon(int size) {
    HDC screenDC = GetDC(NULL);
    HDC memDC = CreateCompatibleDC(screenDC);
    
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = size;
    bmi.bmiHeader.biHeight = size;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    
    void* bits = NULL;
    HBITMAP hbmColor = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    
    // Create proper mask bitmap (1 bit per pixel, white = transparent)
    BITMAPINFO maskBmi = {0};
    maskBmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    maskBmi.bmiHeader.biWidth = size;
    maskBmi.bmiHeader.biHeight = size;
    maskBmi.bmiHeader.biPlanes = 1;
    maskBmi.bmiHeader.biBitCount = 1;
    maskBmi.bmiHeader.biCompression = BI_RGB;
    void* maskBits = NULL;
    HBITMAP hbmMask = CreateDIBSection(memDC, &maskBmi, DIB_RGB_COLORS, &maskBits, NULL, 0);
    
    // Fill mask with white (transparent)
    HDC maskDC = CreateCompatibleDC(memDC);
    SelectObject(maskDC, hbmMask);
    RECT maskRect = {0, 0, size, size};
    FillRect(maskDC, &maskRect, (HBRUSH)GetStockObject(WHITE_BRUSH));
    DeleteDC(maskDC);
    
    SelectObject(memDC, hbmColor);
    
    Graphics g(memDC);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.Clear(Color(0, 0, 0, 0));
    
    int cx = size / 2;
    int cy = size / 2;
    int ew = size * 4 / 5;
    int eh = size * 2 / 5;
    
    // Black eye icon (as requested)
    SolidBrush eyeBrush(Color(255, 0, 0, 0));
    SolidBrush whiteBrush(Color(255, 240, 240, 245));
    SolidBrush pupilBrush(Color(255, 0, 0, 0));
    Pen outlinePen(Color(255, 40, 40, 50), 1.5f);
    
    g.FillEllipse(&eyeBrush, cx - ew/2, cy - eh/2, ew, eh);
    g.DrawEllipse(&outlinePen, cx - ew/2, cy - eh/2, ew, eh);
    
    int irisSize = size / 3;
    g.FillEllipse(&whiteBrush, cx - irisSize/2, cy - irisSize/2, irisSize, irisSize);
    
    int pupilSize = size / 5;
    g.FillEllipse(&pupilBrush, cx - pupilSize/2, cy - pupilSize/2, pupilSize, pupilSize);
    
    int glintSize = size / 10;
    SolidBrush glintBrush(Color(200, 255, 255, 255));
    g.FillEllipse(&glintBrush, cx - pupilSize/3, cy - pupilSize/3, glintSize, glintSize);
    
    ICONINFO ii = {0};
    ii.fIcon = TRUE;
    ii.hbmMask = hbmMask;
    ii.hbmColor = hbmColor;
    
    HICON icon = CreateIconIndirect(&ii);
    
    DeleteObject(hbmColor);
    DeleteObject(hbmMask);
    DeleteDC(memDC);
    ReleaseDC(NULL, screenDC);
    
    return icon;
}

static void init_layout(int w, int h) {
    int margin = 20;
    int card_x = margin;
    int card_w = w - margin * 2;
    int y = 24;
    
    SetRect(&g_rect_title, card_x, y, card_x + card_w, y + 44);
    y += 56;
    
    SetRect(&g_rect_server_label, card_x, y, card_x + card_w, y + 16);
    y += 20;
    
    int add_btn_w = 70;
    SetRect(&g_rect_server_box, card_x, y, card_x + card_w - add_btn_w - 8, y + 40);
    SetRect(&g_rect_btn_add, card_x + card_w - add_btn_w, y, card_x + card_w, y + 40);
    SetRect(&g_rect_server_dropdown, card_x, y + 40, card_x + card_w - add_btn_w - 8, y + 40 + 130);
    y += 52;
    
    SetRect(&g_rect_btn_connect, card_x, y, card_x + card_w, y + 48);
    y += 64;
    
    SetRect(&g_rect_toggle_full, card_x, y, card_x + card_w, y + 36);
    y += 44;
    
    SetRect(&g_rect_toggle_kill, card_x, y, card_x + card_w, y + 36);
    y += 44;
    
    // User agent selector
    SetRect(&g_rect_user_agent_label, card_x, y, card_x + card_w, y + 16);
    y += 20;
    SetRect(&g_rect_user_agent_box, card_x, y, card_x + card_w, y + 40);
    SetRect(&g_rect_user_agent_dropdown, card_x, y + 40, card_x + card_w, y + 40 + 200);
    y += 52;
    
    SetRect(&g_rect_info_box, card_x, y, card_x + card_w, h - margin);
}

static void draw_rounded_rect_gdi(HDC hdc, RECT* rc, int radius, COLORREF fill, COLORREF border) {
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    
    int r = GetRValue(fill), gr = GetGValue(fill), b = GetBValue(fill);
    SolidBrush brush(Color(255, r, gr, b));
    
    int br = GetRValue(border), bg = GetGValue(border), bb = GetBValue(border);
    Pen pen(Color(255, br, bg, bb), 1.0f);
    
    GraphicsPath path;
    int x = rc->left, y = rc->top, w = rc->right - rc->left, h = rc->bottom - rc->top;
    int d = radius * 2;
    
    path.AddArc(x, y, d, d, 180, 90);
    path.AddArc(x + w - d, y, d, d, 270, 90);
    path.AddArc(x + w - d, y + h - d, d, d, 0, 90);
    path.AddArc(x, y + h - d, d, d, 90, 90);
    path.CloseFigure();
    
    g.FillPath(&brush, &path);
    g.DrawPath(&pen, &path);
}

static void draw_toggle_gdi(HDC hdc, RECT* rc, const char* label, int on, int hover) {
    int toggle_w = 50;
    int toggle_h = 26;
    int toggle_x = rc->right - toggle_w;
    int toggle_y = rc->top + (rc->bottom - rc->top - toggle_h) / 2;
    
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    
    COLORREF bg_clr = on ? CLR_TOGGLE_ON : CLR_TOGGLE_OFF;
    if (hover) bg_clr = on ? CLR_ACCENT : RGB(80, 85, 95);
    
    int r = GetRValue(bg_clr), gr = GetGValue(bg_clr), b = GetBValue(bg_clr);
    SolidBrush trackBrush(Color(255, r, gr, b));
    
    GraphicsPath track;
    int radius = toggle_h / 2;
    track.AddArc(toggle_x, toggle_y, toggle_h, toggle_h, 90, 180);
    track.AddArc(toggle_x + toggle_w - toggle_h, toggle_y, toggle_h, toggle_h, 270, 180);
    track.CloseFigure();
    g.FillPath(&trackBrush, &track);
    
    int knob_size = toggle_h - 6;
    int knob_x = on ? (toggle_x + toggle_w - knob_size - 3) : (toggle_x + 3);
    int knob_y = toggle_y + 3;
    
    SolidBrush knobBrush(Color(255, 255, 255, 255));
    g.FillEllipse(&knobBrush, knob_x, knob_y, knob_size, knob_size);
    
    Pen shadow(Color(40, 0, 0, 0), 1.0f);
    g.DrawEllipse(&shadow, knob_x, knob_y, knob_size, knob_size);
    
    RECT label_rc = {rc->left, rc->top, toggle_x - 10, rc->bottom};
    SetTextColor(hdc, on ? CLR_TEXT : CLR_TEXT_DIM);
    SetBkMode(hdc, TRANSPARENT);
    HFONT old = (HFONT)SelectObject(hdc, g_font_normal);
    DrawTextA(hdc, label, -1, &label_rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, old);
}

static void draw_text_centered(HDC hdc, const char* text, RECT* rc, HFONT font, COLORREF color) {
    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);
    HFONT old = (HFONT)SelectObject(hdc, font);
    DrawTextA(hdc, text, -1, rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, old);
}

static void draw_text_left(HDC hdc, const char* text, RECT* rc, HFONT font, COLORREF color) {
    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);
    HFONT old = (HFONT)SelectObject(hdc, font);
    DrawTextA(hdc, text, -1, rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, old);
}

static void draw_info_box(HDC hdc, RECT* rc) {
    draw_rounded_rect_gdi(hdc, rc, 12, CLR_BG_CARD, CLR_BORDER);
    
    int pad = 16;
    int line_h = 22;
    int y = rc->top + pad;
    RECT r;
    
    if (g_ctx && g_ctx->status == HW_CONNECTED) {
        // Server Time
        SetRect(&r, rc->left + pad, y, rc->right - pad, y + line_h);
        draw_text_left(hdc, "SERVER TIME", &r, g_font_small, CLR_TEXT_DIM);
        y += 18;
        
        SetRect(&r, rc->left + pad, y, rc->right - pad, y + line_h);
        char time_str[64];
        if (g_ctx->stats.server_time[0]) {
            snprintf(time_str, sizeof(time_str), "%s", g_ctx->stats.server_time);
        } else {
            // Fallback to local time if server time not available
            time_t now = time(NULL);
            struct tm* t = localtime(&now);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);
        }
        draw_text_left(hdc, time_str, &r, g_font_normal, CLR_ACCENT);
        y += line_h + 10;
        
        // DNS Server
        SetRect(&r, rc->left + pad, y, rc->right - pad, y + line_h);
        draw_text_left(hdc, "DNS", &r, g_font_small, CLR_TEXT_DIM);
        y += 18;
        
        SetRect(&r, rc->left + pad, y, rc->right - pad, y + line_h);
        char dns_display[128];
        if (g_ctx->stats.dns_server[0]) {
            snprintf(dns_display, sizeof(dns_display), "%s", g_ctx->stats.dns_server);
        } else {
            snprintf(dns_display, sizeof(dns_display), "127.0.0.1:1080 (SOCKS5)");
        }
        draw_text_left(hdc, dns_display, &r, g_font_normal, CLR_TEXT);
        y += line_h + 10;
        
        // User Agent
        SetRect(&r, rc->left + pad, y, rc->right - pad, y + line_h);
        draw_text_left(hdc, "USER AGENT", &r, g_font_small, CLR_TEXT_DIM);
        y += 18;
        
        SetRect(&r, rc->left + pad, y, rc->right - pad, y + line_h);
        const char* ua = get_current_user_agent();
        char ua_display[256];
        if (g_randomize_user_agent) {
            snprintf(ua_display, sizeof(ua_display), "[RANDOM] %s", ua);
        } else {
            snprintf(ua_display, sizeof(ua_display), "%s", ua);
        }
        // Truncate if too long
        if (strlen(ua_display) > 50) {
            ua_display[47] = '.';
            ua_display[48] = '.';
            ua_display[49] = '.';
            ua_display[50] = '\0';
        }
        draw_text_left(hdc, ua_display, &r, g_font_small, CLR_TEXT);
    } else {
        SetRect(&r, rc->left + pad, y, rc->right - pad, y + line_h);
        draw_text_left(hdc, "STATUS", &r, g_font_small, CLR_TEXT_DIM);
        y += 18;
        
        const char* status = "Disconnected";
        COLORREF status_clr = CLR_TEXT_DIM;
        if (g_ctx && g_ctx->status == HW_CONNECTING) {
            status = "Connecting...";
            status_clr = CLR_TEXT;
        } else if (g_ctx && g_ctx->status == HW_ERROR) {
            status = "Error";
            status_clr = CLR_ERROR;
        }
        SetRect(&r, rc->left + pad, y, rc->right - pad, y + line_h);
        draw_text_left(hdc, status, &r, g_font_normal, status_clr);
        y += line_h + 16;
        
        SetRect(&r, rc->left + pad, y, rc->right - pad, rc->bottom - pad);
        SetTextColor(hdc, CLR_TEXT_DIM);
        SetBkMode(hdc, TRANSPARENT);
        HFONT old = (HFONT)SelectObject(hdc, g_font_small);
        const char* hint;
        if (g_ctx && g_ctx->status == HW_CONNECTING) {
            hint = "Connecting...\nPlease wait.";
        } else if (g_ctx && g_ctx->status == HW_ERROR && g_ctx->error_msg[0]) {
            // Show error message in status box
            char error_display[256];
            const char* err = g_ctx->error_msg;
            if (strlen(err) > 200) {
                strncpy(error_display, err, 200);
                error_display[200] = '\0';
                strcat(error_display, "...");
            } else {
                strncpy(error_display, err, sizeof(error_display) - 1);
                error_display[sizeof(error_display) - 1] = '\0';
            }
            SetTextColor(hdc, CLR_ERROR);
            DrawTextA(hdc, error_display, -1, &r, DT_LEFT | DT_WORDBREAK);
            SelectObject(hdc, old);
            return;
        } else if (g_ctx && g_ctx->server_count == 0) {
            hint = "No servers configured.\nClick + ADD to add your server.";
        } else {
            hint = "Select a server and click\nCONNECT to start.";
        }
        DrawTextA(hdc, hint, -1, &r, DT_LEFT | DT_WORDBREAK);
        SelectObject(hdc, old);
    }
}

static void paint_main(HWND hwnd, HDC hdc) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    
    Graphics g(hdc);
    g.Clear(Color(255, 13, 17, 23));
    
    draw_text_centered(hdc, "HelloWorld", &g_rect_title, g_font_title, CLR_ACCENT);
    draw_text_left(hdc, "SERVER", &g_rect_server_label, g_font_small, CLR_TEXT_DIM);
    
    draw_rounded_rect_gdi(hdc, &g_rect_server_box, 8, CLR_BG_INPUT, 
                          g_hover_dropdown ? CLR_ACCENT : CLR_BORDER);
    
    const char* server_name = "No servers - click + ADD";
    if (g_ctx && g_ctx->server_count > 0 && g_selected_server >= 0) {
        server_name = g_ctx->servers[g_selected_server].name;
    }
    RECT server_text_rc = g_rect_server_box;
    server_text_rc.left += 14;
    server_text_rc.right -= 28;
    draw_text_left(hdc, server_name, &server_text_rc, g_font_normal, CLR_TEXT);
    
    RECT arrow_rc = g_rect_server_box;
    arrow_rc.left = arrow_rc.right - 28;
    draw_text_centered(hdc, g_dropdown_open ? "^" : "v", &arrow_rc, g_font_small, CLR_TEXT_DIM);
    
    draw_rounded_rect_gdi(hdc, &g_rect_btn_add, 8, 
                          g_hover_add ? CLR_ACCENT : CLR_BG_CARD,
                          g_hover_add ? CLR_ACCENT : CLR_BORDER);
    draw_text_centered(hdc, "+ ADD", &g_rect_btn_add, g_font_small,
                       g_hover_add ? CLR_BG_DARK : CLR_ACCENT);
    
    if (g_dropdown_open && g_ctx && g_ctx->server_count > 0) {
        draw_rounded_rect_gdi(hdc, &g_rect_server_dropdown, 8, CLR_BG_CARD, CLR_BORDER);
        int item_h = 36;
        for (int i = 0; i < g_ctx->server_count && i < 3; i++) {
            RECT item_rc = {
                g_rect_server_dropdown.left + 14,
                g_rect_server_dropdown.top + 10 + i * item_h,
                g_rect_server_dropdown.right - 14,
                g_rect_server_dropdown.top + 10 + (i + 1) * item_h
            };
            COLORREF clr = (i == g_selected_server) ? CLR_ACCENT : CLR_TEXT;
            draw_text_left(hdc, g_ctx->servers[i].name, &item_rc, g_font_normal, clr);
            
            RECT host_rc = item_rc;
            host_rc.top += 16;
            draw_text_left(hdc, g_ctx->servers[i].host, &host_rc, g_font_small, CLR_TEXT_DIM);
        }
    }
    
    COLORREF btn_bg, btn_border, btn_text_clr;
    const char* btn_label;
    
    if (g_ctx && g_ctx->status == HW_CONNECTED) {
        btn_bg = CLR_ERROR; btn_border = CLR_ERROR; btn_text_clr = CLR_TEXT;
        btn_label = "DISCONNECT";
    } else if (g_ctx && g_ctx->status == HW_CONNECTING) {
        btn_bg = CLR_BG_INPUT; btn_border = CLR_BORDER; btn_text_clr = CLR_TEXT_DIM;
        btn_label = "CONNECTING...";
    } else {
        btn_bg = g_hover_connect ? CLR_ACCENT_DARK : CLR_ACCENT;
        btn_border = CLR_ACCENT; btn_text_clr = CLR_BG_DARK;
        btn_label = "CONNECT";
    }
    draw_rounded_rect_gdi(hdc, &g_rect_btn_connect, 8, btn_bg, btn_border);
    draw_text_centered(hdc, btn_label, &g_rect_btn_connect, g_font_button, btn_text_clr);
    
    draw_toggle_gdi(hdc, &g_rect_toggle_full, "Full Tunnel Mode", g_full_tunnel, g_hover_toggle_full);
    draw_toggle_gdi(hdc, &g_rect_toggle_kill, "Kill Switch", g_kill_switch, g_hover_toggle_kill);
    
    draw_info_box(hdc, &g_rect_info_box);
}

static void paint_tutorial(HWND hwnd, HDC hdc) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    
    Graphics g(hdc);
    g.Clear(Color(255, 13, 17, 23));
    
    RECT overlay = {16, 16, rc.right - 16, rc.bottom - 16};
    draw_rounded_rect_gdi(hdc, &overlay, 16, CLR_BG_CARD, CLR_ACCENT);
    
    RECT title_rc = {32, 32, rc.right - 32, 76};
    draw_text_centered(hdc, TUTORIAL_TITLES[g_tutorial_step], &title_rc, g_font_title, CLR_ACCENT);
    
    RECT text_rc = {40, 90, rc.right - 40, rc.bottom - 90};
    SetTextColor(hdc, CLR_TEXT);
    SetBkMode(hdc, TRANSPARENT);
    HFONT old = (HFONT)SelectObject(hdc, g_font_normal);
    DrawTextA(hdc, TUTORIAL_TEXTS[g_tutorial_step], -1, &text_rc, DT_LEFT | DT_WORDBREAK);
    SelectObject(hdc, old);
    
    char progress[32];
    snprintf(progress, sizeof(progress), "%d / %d", g_tutorial_step + 1, 5);
    RECT prog_rc = {40, rc.bottom - 70, 100, rc.bottom - 40};
    draw_text_left(hdc, progress, &prog_rc, g_font_small, CLR_TEXT_DIM);
    
    RECT next_btn = {rc.right - 140, rc.bottom - 70, rc.right - 32, rc.bottom - 36};
    draw_rounded_rect_gdi(hdc, &next_btn, 8, CLR_ACCENT, CLR_ACCENT);
    draw_text_centered(hdc, g_tutorial_step < 4 ? "NEXT" : "START", &next_btn, g_font_button, CLR_BG_DARK);
    
    if (g_tutorial_step > 0) {
        RECT back_btn = {rc.right - 230, rc.bottom - 70, rc.right - 155, rc.bottom - 36};
        draw_rounded_rect_gdi(hdc, &back_btn, 8, CLR_BG_INPUT, CLR_BORDER);
        draw_text_centered(hdc, "BACK", &back_btn, g_font_normal, CLR_TEXT_DIM);
    }
}

static HWND g_add_dlg = NULL;
static HWND g_add_edit_name = NULL;
static HWND g_add_edit_host = NULL;
static HWND g_add_edit_port = NULL;
static HWND g_add_edit_key = NULL;
static HWND g_add_edit_user = NULL;

static LRESULT CALLBACK add_dlg_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE: {
            HFONT font = CreateFontA(14, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
            
            CreateWindowA("STATIC", "Server Name:", WS_CHILD | WS_VISIBLE, 20, 20, 100, 20, hwnd, NULL, NULL, NULL);
            g_add_edit_name = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                20, 40, 420, 24, hwnd, NULL, NULL, NULL);
            
            CreateWindowA("STATIC", "Host / IP Address:", WS_CHILD | WS_VISIBLE, 20, 75, 150, 20, hwnd, NULL, NULL, NULL);
            g_add_edit_host = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                20, 95, 420, 24, hwnd, NULL, NULL, NULL);
            
            CreateWindowA("STATIC", "Port:", WS_CHILD | WS_VISIBLE, 20, 130, 50, 20, hwnd, NULL, NULL, NULL);
            g_add_edit_port = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "443", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER,
                70, 128, 60, 24, hwnd, NULL, NULL, NULL);
            
            CreateWindowA("STATIC", "Username:", WS_CHILD | WS_VISIBLE, 20, 165, 100, 20, hwnd, NULL, NULL, NULL);
            g_add_edit_user = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                20, 185, 420, 24, hwnd, NULL, NULL, NULL);
            CreateWindowA("STATIC", "Oracle: opc | Google: your-username", WS_CHILD | WS_VISIBLE | SS_LEFT,
                20, 212, 420, 16, hwnd, NULL, NULL, NULL);
            
            CreateWindowA("STATIC", "SSH Private Key (file path):", WS_CHILD | WS_VISIBLE, 20, 240, 200, 20, hwnd, NULL, NULL, NULL);
            g_add_edit_key = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                20, 260, 340, 24, hwnd, NULL, NULL, NULL);
            CreateWindowA("BUTTON", "Browse", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                370, 260, 70, 24, hwnd, (HMENU)106, NULL, NULL);
            CreateWindowA("STATIC", "e.g. C:\\Users\\You\\.ssh\\helloworld_key", WS_CHILD | WS_VISIBLE | SS_LEFT,
                20, 287, 420, 16, hwnd, NULL, NULL, NULL);
            
            HWND btnAdd = CreateWindowA("BUTTON", "Add Server", 
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
                160, 315, 90, 30, hwnd, (HMENU)IDOK, NULL, NULL);
            HWND btnCancel = CreateWindowA("BUTTON", "Cancel", 
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                270, 315, 60, 30, hwnd, (HMENU)IDCANCEL, NULL, NULL);
            
            // Ensure buttons are visible with proper colors
            SendMessageA(btnAdd, WM_SETFONT, (WPARAM)font, TRUE);
            SendMessageA(btnCancel, WM_SETFONT, (WPARAM)font, TRUE);
            
            for (HWND child = GetWindow(hwnd, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT))
                SendMessageA(child, WM_SETFONT, (WPARAM)font, TRUE);
            return 0;
        }
        
        case WM_COMMAND:
            if (LOWORD(wp) == 106) {
                OPENFILENAMEA ofn = {0};
                char file[512] = {0};
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hwnd;
                ofn.lpstrFilter = "All Files\0*.*\0";
                ofn.lpstrFile = file;
                ofn.nMaxFile = sizeof(file);
                ofn.Flags = OFN_FILEMUSTEXIST;
                if (GetOpenFileNameA(&ofn)) SetWindowTextA(g_add_edit_key, file);
            } else if (LOWORD(wp) == IDOK) {
                GetWindowTextA(g_add_edit_name, g_add_name, sizeof(g_add_name));
                GetWindowTextA(g_add_edit_host, g_add_host, sizeof(g_add_host));
                GetWindowTextA(g_add_edit_port, g_add_port, sizeof(g_add_port));
                GetWindowTextA(g_add_edit_key, g_add_key, sizeof(g_add_key));
                GetWindowTextA(g_add_edit_user, g_add_user, sizeof(g_add_user));
                
                if (!g_add_name[0] || !g_add_host[0]) {
                    MessageBoxA(hwnd, "Please enter Name and Host.", "Missing Info", MB_OK | MB_ICONWARNING);
                    return 0;
                }
                if (!g_add_user[0]) {
                    MessageBoxA(hwnd, "Please enter your username.\n\nOracle Cloud: 'opc'\nGoogle Cloud: Your Google account username\n\nTo find it: SSH into your VM and run 'whoami'", "Username Required", MB_OK | MB_ICONWARNING);
                    return 0;
                }
                if (!g_add_key[0]) {
                    MessageBoxA(hwnd, "Please select your SSH private key file.\n\nClick Browse and navigate to your .ssh folder.\nSelect the private key (NOT the .pub file).", "SSH Key Required", MB_OK | MB_ICONWARNING);
                    return 0;
                }
                FILE* keytest = fopen(g_add_key, "r");
                if (!keytest) {
                    MessageBoxA(hwnd, "SSH key file not found!\n\nMake sure the path is correct.\nExample: C:\\Users\\You\\.ssh\\helloworld_key", "File Not Found", MB_OK | MB_ICONERROR);
                    return 0;
                }
                fclose(keytest);
                if (g_ctx) {
                    int port = atoi(g_add_port);
                    hw_add_server(g_ctx, g_add_name, g_add_host, port > 0 ? port : 443, g_add_key, g_add_user);
                    g_selected_server = g_ctx->server_count - 1;
                    hw_save_config(g_ctx);
                }
                DestroyWindow(hwnd);
            } else if (LOWORD(wp) == IDCANCEL) {
                DestroyWindow(hwnd);
            }
            return 0;
        
        case WM_DESTROY:
            g_add_dlg = NULL;
            InvalidateRect(g_hwnd, NULL, TRUE);
            return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

static void show_add_server_dialog(HWND parent) {
    if (g_add_dlg) { SetForegroundWindow(g_add_dlg); return; }
    
    WNDCLASSEXA wc = {sizeof(wc), 0, add_dlg_proc, 0, 0, GetModuleHandle(NULL),
        NULL, LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_BTNFACE + 1), NULL, "AddServerDlg", NULL};
    RegisterClassExA(&wc);
    
    RECT prc; GetWindowRect(parent, &prc);
    int w = 480, h = 380;
    g_add_dlg = CreateWindowExA(WS_EX_DLGMODALFRAME, "AddServerDlg", "Add Server",
        WS_POPUP | WS_CAPTION | WS_SYSMENU, 
        prc.left + (prc.right-prc.left-w)/2, prc.top + (prc.bottom-prc.top-h)/2,
        w, h, parent, NULL, GetModuleHandle(NULL), NULL);
    ShowWindow(g_add_dlg, SW_SHOW);
}

static int check_first_run(void) {
    char path[512];
    if (g_ctx) {
        snprintf(path, sizeof(path), "%s\\first_run_done", g_ctx->config_dir);
        FILE* f = fopen(path, "r");
        if (f) { fclose(f); return 0; }
        f = fopen(path, "w");
        if (f) { fprintf(f, "1"); fclose(f); }
    }
    return 1;
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE: {
            g_font_title = CreateFontA(28, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
            g_font_normal = CreateFontA(14, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
            g_font_button = CreateFontA(15, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
            g_font_small = CreateFontA(11, 0, 0, 0, FW_MEDIUM, 0, 0, 0, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
            
            g_brush_bg = CreateSolidBrush(CLR_BG_DARK);
            g_brush_card = CreateSolidBrush(CLR_BG_CARD);
            g_brush_input = CreateSolidBrush(CLR_BG_INPUT);
            g_brush_accent = CreateSolidBrush(CLR_ACCENT);
            
            RECT rc; GetClientRect(hwnd, &rc);
            init_layout(rc.right, rc.bottom);
            
            if (g_ctx) {
                g_selected_server = g_ctx->current_server >= 0 ? g_ctx->current_server : 0;
                g_full_tunnel = g_ctx->mode == HW_MODE_FULL_TUNNEL;
                g_kill_switch = g_ctx->kill_switch;
                // Set initial user agent
                const char* ua = get_current_user_agent();
                if (ua) {
                    strncpy(g_ctx->stats.user_agent, ua, sizeof(g_ctx->stats.user_agent) - 1);
                    g_ctx->stats.user_agent[sizeof(g_ctx->stats.user_agent) - 1] = '\0';
                }
            }
            
            g_show_tutorial = check_first_run();
            load_user_agents();  // Load user agents on startup
            srand((unsigned int)time(NULL));  // Seed random for user agent randomization
            SetTimer(hwnd, ID_TIMER_UPDATE, 1000, NULL);
            return 0;
        }
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc; GetClientRect(hwnd, &rc);
            HDC mem = CreateCompatibleDC(hdc);
            HBITMAP bmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            HBITMAP old = (HBITMAP)SelectObject(mem, bmp);
            
            if (g_show_tutorial) paint_tutorial(hwnd, mem);
            else paint_main(hwnd, mem);
            
            BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
            SelectObject(mem, old);
            DeleteObject(bmp);
            DeleteDC(mem);
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_TIMER:
            if (wp == ID_TIMER_UPDATE) {
                // Update server time and DNS periodically
                if (g_ctx && g_ctx->status == HW_CONNECTED) {
                    hw_fetch_server_time(g_ctx);
                    hw_fetch_dns_info(g_ctx);
                    // Update user agent if randomizing
                    if (g_randomize_user_agent) {
                        const char* ua = get_current_user_agent();
                        if (ua) {
                            strncpy(g_ctx->stats.user_agent, ua, sizeof(g_ctx->stats.user_agent) - 1);
                            g_ctx->stats.user_agent[sizeof(g_ctx->stats.user_agent) - 1] = '\0';
                        }
                    }
                }
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        
        case WM_MOUSEMOVE: {
            if (g_show_tutorial) return 0;
            int x = GET_X_LPARAM(lp), y = GET_Y_LPARAM(lp);
            int h1 = g_hover_connect, h2 = g_hover_add, h3 = g_hover_dropdown;
            int h4 = g_hover_toggle_full, h5 = g_hover_toggle_kill;
            
            POINT pt = MakePt(x, y);
            g_hover_connect = PtInRect(&g_rect_btn_connect, pt);
            g_hover_add = PtInRect(&g_rect_btn_add, pt);
            g_hover_dropdown = PtInRect(&g_rect_server_box, pt);
            g_hover_toggle_full = PtInRect(&g_rect_toggle_full, pt);
            g_hover_toggle_kill = PtInRect(&g_rect_toggle_kill, pt);
            
            if (h1!=g_hover_connect || h2!=g_hover_add || h3!=g_hover_dropdown || h4!=g_hover_toggle_full || h5!=g_hover_toggle_kill)
                InvalidateRect(hwnd, NULL, FALSE);
            
            TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, hwnd, 0};
            TrackMouseEvent(&tme);
            return 0;
        }
        
        case WM_MOUSELEAVE:
            g_hover_connect = g_hover_add = g_hover_dropdown = g_hover_toggle_full = g_hover_toggle_kill = 0;
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        
        case WM_LBUTTONDOWN: {
            int x = GET_X_LPARAM(lp), y = GET_Y_LPARAM(lp);
            RECT rc; GetClientRect(hwnd, &rc);
            
            POINT pt = MakePt(x, y);
            if (g_show_tutorial) {
                RECT next_btn = {rc.right - 140, rc.bottom - 70, rc.right - 32, rc.bottom - 36};
                RECT back_btn = {rc.right - 230, rc.bottom - 70, rc.right - 155, rc.bottom - 36};
                if (PtInRect(&next_btn, pt)) {
                    if (g_tutorial_step < 4) g_tutorial_step++; else g_show_tutorial = 0;
                } else if (g_tutorial_step > 0 && PtInRect(&back_btn, pt)) {
                    g_tutorial_step--;
                }
                InvalidateRect(hwnd, NULL, TRUE);
                return 0;
            }
            
            if (PtInRect(&g_rect_btn_connect, pt) && g_ctx) {
                if (g_ctx->status == HW_CONNECTED) hw_disconnect(g_ctx);
                else if (g_ctx->status == HW_DISCONNECTED) {
                    if (g_ctx->server_count == 0) {
                        MessageBoxA(hwnd, "Please add a server first.\n\nClick the + ADD button to add your server.", "No Server", MB_OK | MB_ICONINFORMATION);
                    } else {
                        g_ctx->current_server = g_selected_server;
                        g_ctx->mode = g_full_tunnel ? HW_MODE_FULL_TUNNEL : HW_MODE_PROXY;
                        g_ctx->kill_switch = g_kill_switch;
                        int result = hw_connect(g_ctx);
                        if (result != 0) {
                            char errmsg[1024];
                            const char* error_detail = g_ctx->error_msg[0] ? g_ctx->error_msg : "Unknown error";
                            
                            if (strstr(error_detail, "stunnel not found")) {
                                snprintf(errmsg, sizeof(errmsg), 
                                    "STUNNEL NOT INSTALLED!\n\n"
                                    "HelloWorld requires stunnel to work.\n\n"
                                    "To install stunnel:\n"
                                    "1. Open PowerShell as Administrator\n"
                                    "2. Run: winget install stunnel\n"
                                    "   OR download from: https://www.stunnel.org/downloads.html\n"
                                    "3. Restart HelloWorld after installing\n\n"
                                    "Original error: %s", error_detail);
                            } else if (strstr(error_detail, "stunnel exited") || strstr(error_detail, "Failed to start stunnel")) {
                                snprintf(errmsg, sizeof(errmsg), 
                                    "STUNNEL FAILED TO START!\n\n"
                                    "Error: %s\n\n"
                                    "Checklist:\n"
                                    "1. Make sure port %d is free on your PC\n"
                                    "2. Restart HelloWorld and try again\n"
                                    "3. Reinstall stunnel if needed\n"
                                    "4. Run stunnel manually to see errors\n",
                                    error_detail, HW_LOCAL_PORT);
                            } else if (strstr(error_detail, "SSH") || strstr(error_detail, "ssh")) {
                                snprintf(errmsg, sizeof(errmsg), 
                                    "SSH CONNECTION FAILED!\n\n"
                                    "Error: %s\n\n"
                                    "Troubleshooting:\n"
                                    "1. Verify server IP is correct\n"
                                    "2. Check username (Oracle: 'opc', Google: 'your-username')\n"
                                    "3. Verify SSH key file path is correct\n"
                                    "4. Make sure key is authorized on server\n"
                                    "5. Test connection: ssh -i \"key\" user@server-ip\n"
                                    "6. Check server status: ssh into server and run 'helloworld-status'",
                                    error_detail);
                            } else {
                                snprintf(errmsg, sizeof(errmsg), 
                                    "CONNECTION FAILED!\n\n"
                                    "Error: %s\n\n"
                                    "Step-by-step checklist:\n"
                                    "1. Server running? SSH to server and run: helloworld-status\n"
                                    "2. Port 443 open? Check firewall rules on cloud provider\n"
                                    "3. SSH key correct? Verify file path and permissions\n"
                                    "4. Username correct? Oracle: 'opc', Google: check your VM user\n"
                                    "5. Server IP correct? Get it from cloud console\n"
                                    "6. Stunnel installed? Run: winget install stunnel\n\n"
                                    "For detailed setup: https://github.com/dokabi-recon67/helloworld",
                                    error_detail);
                            }
                            MessageBoxA(hwnd, errmsg, "Connection Failed", MB_OK | MB_ICONERROR);
                        }
                    }
                }
            } else if (PtInRect(&g_rect_btn_add, pt)) {
                show_add_server_dialog(hwnd);
            } else if (PtInRect(&g_rect_server_box, pt)) {
                g_dropdown_open = !g_dropdown_open;
            } else if (g_dropdown_open && PtInRect(&g_rect_server_dropdown, pt)) {
                int idx = (y - g_rect_server_dropdown.top - 10) / 36;
                if (idx >= 0 && g_ctx && idx < g_ctx->server_count) g_selected_server = idx;
                g_dropdown_open = 0;
            } else if (PtInRect(&g_rect_toggle_full, pt)) {
                g_full_tunnel = !g_full_tunnel;
                InvalidateRect(hwnd, NULL, FALSE);
            } else if (PtInRect(&g_rect_toggle_kill, pt)) {
                g_kill_switch = !g_kill_switch;
                InvalidateRect(hwnd, NULL, FALSE);
            } else if (PtInRect(&g_rect_user_agent_box, pt)) {
                g_user_agent_dropdown_open = !g_user_agent_dropdown_open;
                g_dropdown_open = 0;  // Close server dropdown if open
            } else if (g_user_agent_dropdown_open && PtInRect(&g_rect_user_agent_dropdown, pt)) {
                int idx = (y - g_rect_user_agent_dropdown.top - 10) / 32;
                if (idx == 0) {
                    // Randomize option
                    g_randomize_user_agent = !g_randomize_user_agent;
                } else if (idx > 0 && idx <= g_user_agent_count) {
                    g_selected_user_agent = idx - 1;
                    g_randomize_user_agent = 0;
                }
                g_user_agent_dropdown_open = 0;
                if (g_ctx) {
                    strncpy(g_ctx->stats.user_agent, get_current_user_agent(), sizeof(g_ctx->stats.user_agent) - 1);
                    g_ctx->stats.user_agent[sizeof(g_ctx->stats.user_agent) - 1] = '\0';
                }
                InvalidateRect(hwnd, NULL, FALSE);
            } else {
                g_dropdown_open = 0;
                g_user_agent_dropdown_open = 0;
            }
            return 0;
        }
        
        case WM_ERASEBKGND: return 1;
        
        case WM_DESTROY:
            KillTimer(hwnd, ID_TIMER_UPDATE);
            DeleteObject(g_font_title); DeleteObject(g_font_normal);
            DeleteObject(g_font_button); DeleteObject(g_font_small);
            DeleteObject(g_brush_bg); DeleteObject(g_brush_card);
            DeleteObject(g_brush_input); DeleteObject(g_brush_accent);
            if (g_icon) DestroyIcon(g_icon);
            PostQuitMessage(0);
            return 0;
        
        case WM_CLOSE:
            if (g_ctx && g_ctx->status == HW_CONNECTED) {
                if (MessageBoxA(hwnd, "Still connected. Disconnect?", "HelloWorld", MB_YESNO) != IDYES) return 0;
                hw_disconnect(g_ctx);
            }
            DestroyWindow(hwnd);
            return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show) {
    (void)prev; (void)cmd;
    
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);
    
    SetProcessDPIAware();
    g_ctx = hw_create();
    
    g_icon = create_eye_icon(32);
    HICON iconSmall = create_eye_icon(16);
    
    WNDCLASSEXA wc = {sizeof(wc), CS_HREDRAW | CS_VREDRAW, window_proc, 0, 0, inst,
        g_icon, LoadCursor(NULL, IDC_ARROW), NULL, NULL, "HelloWorldClass", iconSmall};
    RegisterClassExA(&wc);
    
    int w = 360, h = 580;
    g_hwnd = CreateWindowExA(WS_EX_APPWINDOW, "HelloWorldClass", "HelloWorld",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,
        (GetSystemMetrics(SM_CXSCREEN)-w)/2, (GetSystemMetrics(SM_CYSCREEN)-h)/2,
        w, h, NULL, NULL, inst, NULL);
    
    // Set window icons (for title bar and taskbar)
    SendMessage(g_hwnd, WM_SETICON, ICON_BIG, (LPARAM)g_icon);
    SendMessage(g_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)iconSmall);
    
    // Also set class icon for better compatibility
    SetClassLongPtrA(g_hwnd, GCLP_HICON, (LONG_PTR)g_icon);
    SetClassLongPtrA(g_hwnd, GCLP_HICONSM, (LONG_PTR)iconSmall);
    
    ShowWindow(g_hwnd, show);
    UpdateWindow(g_hwnd);
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    hw_destroy(g_ctx);
    GdiplusShutdown(g_gdiplusToken);
    return (int)msg.wParam;
}

#endif
