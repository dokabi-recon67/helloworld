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

static RECT g_rect_title;
static RECT g_rect_server_label;
static RECT g_rect_server_box;
static RECT g_rect_server_dropdown;
static RECT g_rect_btn_add;
static RECT g_rect_btn_connect;
static RECT g_rect_toggle_full;
static RECT g_rect_toggle_kill;
static RECT g_rect_info_box;

static int g_hover_connect = 0;
static int g_hover_add = 0;
static int g_hover_dropdown = 0;
static int g_hover_toggle_full = 0;
static int g_hover_toggle_kill = 0;
static int g_dropdown_open = 0;

static char g_add_name[64] = {0};
static char g_add_host[256] = {0};
static char g_add_port[16] = "443";
static char g_add_key[512] = {0};
static char g_add_user[64] = "root";

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
    HBITMAP hbmMask = CreateBitmap(size, size, 1, 1, NULL);
    
    SelectObject(memDC, hbmColor);
    
    Graphics g(memDC);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.Clear(Color(0, 0, 0, 0));
    
    int cx = size / 2;
    int cy = size / 2;
    int ew = size * 4 / 5;
    int eh = size * 2 / 5;
    
    SolidBrush accentBrush(Color(255, 0, 255, 136));
    SolidBrush whiteBrush(Color(255, 255, 255, 255));
    SolidBrush blackBrush(Color(255, 13, 17, 23));
    Pen outlinePen(Color(255, 0, 200, 100), 2.0f);
    
    g.FillEllipse(&accentBrush, cx - ew/2, cy - eh/2, ew, eh);
    
    int irisSize = size / 3;
    g.FillEllipse(&whiteBrush, cx - irisSize/2, cy - irisSize/2, irisSize, irisSize);
    
    int pupilSize = size / 6;
    g.FillEllipse(&blackBrush, cx - pupilSize/2, cy - pupilSize/2, pupilSize, pupilSize);
    
    int glintSize = size / 12;
    g.FillEllipse(&whiteBrush, cx - pupilSize/4, cy - pupilSize/4, glintSize, glintSize);
    
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
        SetRect(&r, rc->left + pad, y, rc->right - pad, y + line_h);
        draw_text_left(hdc, "STATUS", &r, g_font_small, CLR_TEXT_DIM);
        y += 18;
        
        SetRect(&r, rc->left + pad, y, rc->right - pad, y + line_h);
        draw_text_left(hdc, "Connected", &r, g_font_normal, CLR_ACCENT);
        y += line_h + 10;
        
        SetRect(&r, rc->left + pad, y, rc->right - pad, y + line_h);
        draw_text_left(hdc, "MODE", &r, g_font_small, CLR_TEXT_DIM);
        y += 18;
        
        SetRect(&r, rc->left + pad, y, rc->right - pad, y + line_h);
        const char* mode = g_ctx->mode == HW_MODE_FULL_TUNNEL ? "Full Tunnel (All Traffic)" : "Proxy Mode (SOCKS5)";
        draw_text_left(hdc, mode, &r, g_font_normal, CLR_TEXT);
        y += line_h + 10;
        
        SetRect(&r, rc->left + pad, y, rc->right - pad, y + line_h);
        draw_text_left(hdc, "YOUR IP", &r, g_font_small, CLR_TEXT_DIM);
        y += 18;
        
        SetRect(&r, rc->left + pad, y, rc->right - pad, y + line_h);
        char ip_display[128];
        snprintf(ip_display, sizeof(ip_display), "%s", 
                 g_ctx->stats.public_ip[0] ? g_ctx->stats.public_ip : "Fetching...");
        draw_text_left(hdc, ip_display, &r, g_font_button, CLR_ACCENT);
        y += line_h + 10;
        
        SetRect(&r, rc->left + pad, y, rc->right - pad, y + line_h);
        draw_text_left(hdc, "SERVER", &r, g_font_small, CLR_TEXT_DIM);
        y += 18;
        
        SetRect(&r, rc->left + pad, y, rc->right - pad, y + line_h);
        if (g_ctx->current_server >= 0 && g_ctx->current_server < g_ctx->server_count) {
            draw_text_left(hdc, g_ctx->servers[g_ctx->current_server].name, &r, g_font_normal, CLR_TEXT);
        }
        y += line_h + 10;
        
        SetRect(&r, rc->left + pad, y, rc->right - pad, y + line_h);
        draw_text_left(hdc, "DURATION", &r, g_font_small, CLR_TEXT_DIM);
        y += 18;
        
        time_t el = time(NULL) - g_ctx->stats.start_time;
        char dur[64];
        snprintf(dur, sizeof(dur), "%02d:%02d:%02d", (int)(el/3600), (int)((el%3600)/60), (int)(el%60));
        SetRect(&r, rc->left + pad, y, rc->right - pad, y + line_h);
        draw_text_left(hdc, dur, &r, g_font_normal, CLR_TEXT);
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
        const char* hint = g_ctx && g_ctx->server_count == 0 
            ? "No servers configured.\nClick + ADD to add your server."
            : "Select a server and click\nCONNECT to start.";
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
                20, 40, 260, 24, hwnd, NULL, NULL, NULL);
            
            CreateWindowA("STATIC", "Host / IP Address:", WS_CHILD | WS_VISIBLE, 20, 75, 150, 20, hwnd, NULL, NULL, NULL);
            g_add_edit_host = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                20, 95, 260, 24, hwnd, NULL, NULL, NULL);
            
            CreateWindowA("STATIC", "Port:", WS_CHILD | WS_VISIBLE, 20, 130, 50, 20, hwnd, NULL, NULL, NULL);
            g_add_edit_port = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "443", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER,
                70, 128, 60, 24, hwnd, NULL, NULL, NULL);
            
            CreateWindowA("STATIC", "Username:", WS_CHILD | WS_VISIBLE, 150, 130, 70, 20, hwnd, NULL, NULL, NULL);
            g_add_edit_user = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "root", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                220, 128, 60, 24, hwnd, NULL, NULL, NULL);
            
            CreateWindowA("STATIC", "SSH Key File:", WS_CHILD | WS_VISIBLE, 20, 165, 100, 20, hwnd, NULL, NULL, NULL);
            g_add_edit_key = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                20, 185, 200, 24, hwnd, NULL, NULL, NULL);
            CreateWindowA("BUTTON", "Browse", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                225, 185, 55, 24, hwnd, (HMENU)106, NULL, NULL);
            
            CreateWindowA("BUTTON", "Add Server", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                70, 230, 90, 30, hwnd, (HMENU)IDOK, NULL, NULL);
            CreateWindowA("BUTTON", "Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                170, 230, 60, 30, hwnd, (HMENU)IDCANCEL, NULL, NULL);
            
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
                    MessageBoxA(hwnd, "Please enter Name and Host.", "Missing Info", MB_OK);
                    return 0;
                }
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
    int w = 320, h = 300;
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
            }
            
            g_show_tutorial = check_first_run();
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
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        
        case WM_MOUSEMOVE: {
            if (g_show_tutorial) return 0;
            int x = GET_X_LPARAM(lp), y = GET_Y_LPARAM(lp);
            int h1 = g_hover_connect, h2 = g_hover_add, h3 = g_hover_dropdown;
            int h4 = g_hover_toggle_full, h5 = g_hover_toggle_kill;
            
            g_hover_connect = PtInRect(&g_rect_btn_connect, (POINT){x, y});
            g_hover_add = PtInRect(&g_rect_btn_add, (POINT){x, y});
            g_hover_dropdown = PtInRect(&g_rect_server_box, (POINT){x, y});
            g_hover_toggle_full = PtInRect(&g_rect_toggle_full, (POINT){x, y});
            g_hover_toggle_kill = PtInRect(&g_rect_toggle_kill, (POINT){x, y});
            
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
            
            if (g_show_tutorial) {
                RECT next_btn = {rc.right - 140, rc.bottom - 70, rc.right - 32, rc.bottom - 36};
                RECT back_btn = {rc.right - 230, rc.bottom - 70, rc.right - 155, rc.bottom - 36};
                if (PtInRect(&next_btn, (POINT){x, y})) {
                    if (g_tutorial_step < 4) g_tutorial_step++; else g_show_tutorial = 0;
                } else if (g_tutorial_step > 0 && PtInRect(&back_btn, (POINT){x, y})) {
                    g_tutorial_step--;
                }
                InvalidateRect(hwnd, NULL, TRUE);
                return 0;
            }
            
            if (PtInRect(&g_rect_btn_connect, (POINT){x, y}) && g_ctx) {
                if (g_ctx->status == HW_CONNECTED) hw_disconnect(g_ctx);
                else if (g_ctx->status == HW_DISCONNECTED) {
                    if (g_ctx->server_count == 0) MessageBoxA(hwnd, "Please add a server first.", "No Server", MB_OK);
                    else {
                        g_ctx->current_server = g_selected_server;
                        g_ctx->mode = g_full_tunnel ? HW_MODE_FULL_TUNNEL : HW_MODE_PROXY;
                        g_ctx->kill_switch = g_kill_switch;
                        hw_connect(g_ctx);
                    }
                }
            } else if (PtInRect(&g_rect_btn_add, (POINT){x, y})) {
                show_add_server_dialog(hwnd);
            } else if (PtInRect(&g_rect_server_box, (POINT){x, y})) {
                g_dropdown_open = !g_dropdown_open;
            } else if (g_dropdown_open && PtInRect(&g_rect_server_dropdown, (POINT){x, y})) {
                int idx = (y - g_rect_server_dropdown.top - 10) / 36;
                if (idx >= 0 && g_ctx && idx < g_ctx->server_count) g_selected_server = idx;
                g_dropdown_open = 0;
            } else if (PtInRect(&g_rect_toggle_full, (POINT){x, y})) {
                g_full_tunnel = !g_full_tunnel;
            } else if (PtInRect(&g_rect_toggle_kill, (POINT){x, y})) {
                g_kill_switch = !g_kill_switch;
            } else {
                g_dropdown_open = 0;
            }
            InvalidateRect(hwnd, NULL, FALSE);
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
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        (GetSystemMetrics(SM_CXSCREEN)-w)/2, (GetSystemMetrics(SM_CYSCREEN)-h)/2,
        w, h, NULL, NULL, inst, NULL);
    
    SendMessage(g_hwnd, WM_SETICON, ICON_BIG, (LPARAM)g_icon);
    SendMessage(g_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)iconSmall);
    
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
