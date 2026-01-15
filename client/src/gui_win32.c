/*
 * HelloWorld - Windows GUI
 * Modern Dark Theme with Add Server Dialog & Tutorial
 */

#ifdef _WIN32

#include "helloworld.h"
#include <commctrl.h>
#include <commdlg.h>
#include <windowsx.h>
#include <shlobj.h>
#include <wchar.h>
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

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

static hw_ctx_t* g_ctx = NULL;
static HWND g_hwnd = NULL;
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
static RECT g_rect_chk_full;
static RECT g_rect_chk_kill;
static RECT g_rect_status;
static RECT g_rect_ip;
static RECT g_rect_time;

static int g_hover_connect = 0;
static int g_hover_add = 0;
static int g_hover_dropdown = 0;
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

static void init_layout(int w, int h) {
    int margin = 24;
    int card_x = margin;
    int card_w = w - margin * 2;
    int y = 30;
    
    SetRect(&g_rect_title, card_x, y, card_x + card_w, y + 50);
    y += 70;
    
    SetRect(&g_rect_server_label, card_x, y, card_x + card_w, y + 20);
    y += 24;
    
    int add_btn_w = 100;
    SetRect(&g_rect_server_box, card_x, y, card_x + card_w - add_btn_w - 8, y + 44);
    SetRect(&g_rect_btn_add, card_x + card_w - add_btn_w, y, card_x + card_w, y + 44);
    SetRect(&g_rect_server_dropdown, card_x, y + 44, card_x + card_w - add_btn_w - 8, y + 44 + 150);
    y += 60;
    
    SetRect(&g_rect_btn_connect, card_x, y, card_x + card_w, y + 52);
    y += 70;
    
    SetRect(&g_rect_chk_full, card_x, y, card_x + card_w, y + 28);
    y += 36;
    
    SetRect(&g_rect_chk_kill, card_x, y, card_x + card_w, y + 28);
    y += 50;
    
    SetRect(&g_rect_status, card_x, y, card_x + card_w, y + 28);
    y += 32;
    
    SetRect(&g_rect_ip, card_x, y, card_x + card_w, y + 24);
    y += 28;
    
    SetRect(&g_rect_time, card_x, y, card_x + card_w, y + 24);
}

static void draw_rounded_rect(HDC hdc, RECT* rc, int radius, HBRUSH fill, COLORREF border) {
    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HPEN old_pen = SelectObject(hdc, pen);
    HBRUSH old_brush = SelectObject(hdc, fill);
    RoundRect(hdc, rc->left, rc->top, rc->right, rc->bottom, radius, radius);
    SelectObject(hdc, old_pen);
    SelectObject(hdc, old_brush);
    DeleteObject(pen);
}

static void draw_text_centered(HDC hdc, const char* text, RECT* rc, HFONT font, COLORREF color) {
    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);
    HFONT old_font = SelectObject(hdc, font);
    DrawTextA(hdc, text, -1, rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, old_font);
}

static void draw_text_left(HDC hdc, const char* text, RECT* rc, HFONT font, COLORREF color) {
    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);
    HFONT old_font = SelectObject(hdc, font);
    DrawTextA(hdc, text, -1, rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, old_font);
}

static void draw_checkbox(HDC hdc, RECT* rc, const char* text, int checked) {
    int box_size = 20;
    RECT box_rc = {rc->left, rc->top + 4, rc->left + box_size, rc->top + 4 + box_size};
    HBRUSH fill = checked ? g_brush_accent : g_brush_input;
    draw_rounded_rect(hdc, &box_rc, 4, fill, checked ? CLR_ACCENT : CLR_BORDER);
    if (checked) {
        SetTextColor(hdc, CLR_BG_DARK);
        SetBkMode(hdc, TRANSPARENT);
        HFONT old = SelectObject(hdc, g_font_normal);
        DrawTextA(hdc, "X", -1, &box_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, old);
    }
    RECT text_rc = {rc->left + box_size + 12, rc->top, rc->right, rc->bottom};
    draw_text_left(hdc, text, &text_rc, g_font_normal, CLR_TEXT_DIM);
}

static void paint_main(HWND hwnd, HDC hdc) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    
    HBRUSH bg = CreateSolidBrush(CLR_BG_DARK);
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);
    
    draw_text_centered(hdc, "HelloWorld", &g_rect_title, g_font_title, CLR_ACCENT);
    draw_text_left(hdc, "SERVER", &g_rect_server_label, g_font_small, CLR_TEXT_DIM);
    
    draw_rounded_rect(hdc, &g_rect_server_box, 8, g_brush_input, 
                      g_hover_dropdown ? CLR_ACCENT : CLR_BORDER);
    
    const char* server_name = "No servers - click + ADD";
    if (g_ctx && g_ctx->server_count > 0 && g_selected_server >= 0) {
        server_name = g_ctx->servers[g_selected_server].name;
    }
    RECT server_text_rc = g_rect_server_box;
    server_text_rc.left += 16;
    server_text_rc.right -= 30;
    draw_text_left(hdc, server_name, &server_text_rc, g_font_normal, CLR_TEXT);
    
    RECT arrow_rc = g_rect_server_box;
    arrow_rc.left = arrow_rc.right - 30;
    draw_text_centered(hdc, g_dropdown_open ? "^" : "v", &arrow_rc, g_font_small, CLR_TEXT_DIM);
    
    draw_rounded_rect(hdc, &g_rect_btn_add, 8, 
                      g_hover_add ? g_brush_accent : g_brush_card,
                      g_hover_add ? CLR_ACCENT : CLR_BORDER);
    draw_text_centered(hdc, "+ ADD", &g_rect_btn_add, g_font_normal,
                       g_hover_add ? CLR_BG_DARK : CLR_ACCENT);
    
    if (g_dropdown_open && g_ctx && g_ctx->server_count > 0) {
        draw_rounded_rect(hdc, &g_rect_server_dropdown, 8, g_brush_card, CLR_BORDER);
        int item_h = 40;
        for (int i = 0; i < g_ctx->server_count && i < 3; i++) {
            RECT item_rc = {
                g_rect_server_dropdown.left + 16,
                g_rect_server_dropdown.top + 12 + i * item_h,
                g_rect_server_dropdown.right - 16,
                g_rect_server_dropdown.top + 12 + (i + 1) * item_h
            };
            COLORREF clr = (i == g_selected_server) ? CLR_ACCENT : CLR_TEXT;
            draw_text_left(hdc, g_ctx->servers[i].name, &item_rc, g_font_normal, clr);
        }
    }
    
    HBRUSH btn_brush;
    COLORREF btn_text;
    const char* btn_label;
    
    if (g_ctx && g_ctx->status == HW_CONNECTED) {
        btn_brush = CreateSolidBrush(CLR_ERROR);
        btn_text = CLR_TEXT;
        btn_label = "DISCONNECT";
    } else if (g_ctx && g_ctx->status == HW_CONNECTING) {
        btn_brush = g_brush_input;
        btn_text = CLR_TEXT_DIM;
        btn_label = "CONNECTING...";
    } else {
        btn_brush = g_hover_connect ? CreateSolidBrush(CLR_ACCENT_DARK) : g_brush_accent;
        btn_text = CLR_BG_DARK;
        btn_label = "CONNECT";
    }
    draw_rounded_rect(hdc, &g_rect_btn_connect, 8, btn_brush, CLR_ACCENT);
    draw_text_centered(hdc, btn_label, &g_rect_btn_connect, g_font_button, btn_text);
    if (btn_brush != g_brush_accent && btn_brush != g_brush_input) DeleteObject(btn_brush);
    
    draw_checkbox(hdc, &g_rect_chk_full, "Full Tunnel (route all traffic)", g_full_tunnel);
    draw_checkbox(hdc, &g_rect_chk_kill, "Kill Switch (block if disconnected)", g_kill_switch);
    
    const char* status_text = "DISCONNECTED";
    COLORREF status_color = CLR_TEXT_DIM;
    if (g_ctx) {
        if (g_ctx->status == HW_CONNECTED) { status_text = "CONNECTED"; status_color = CLR_ACCENT; }
        else if (g_ctx->status == HW_CONNECTING) { status_text = "CONNECTING..."; }
        else if (g_ctx->status == HW_ERROR) { status_text = "ERROR"; status_color = CLR_ERROR; }
    }
    draw_text_centered(hdc, status_text, &g_rect_status, g_font_normal, status_color);
    
    char ip_text[128] = "IP: ---";
    if (g_ctx && g_ctx->status == HW_CONNECTED && g_ctx->stats.public_ip[0])
        snprintf(ip_text, sizeof(ip_text), "IP: %s", g_ctx->stats.public_ip);
    draw_text_centered(hdc, ip_text, &g_rect_ip, g_font_small, CLR_TEXT_DIM);
    
    char time_text[64] = "Duration: --:--:--";
    if (g_ctx && g_ctx->status == HW_CONNECTED) {
        time_t el = time(NULL) - g_ctx->stats.start_time;
        snprintf(time_text, sizeof(time_text), "Duration: %02d:%02d:%02d", 
                 (int)(el/3600), (int)((el%3600)/60), (int)(el%60));
    }
    draw_text_centered(hdc, time_text, &g_rect_time, g_font_small, CLR_TEXT_DIM);
}

static void paint_tutorial(HWND hwnd, HDC hdc) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    
    HBRUSH bg = CreateSolidBrush(CLR_BG_DARK);
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);
    
    RECT overlay = {20, 20, rc.right - 20, rc.bottom - 20};
    draw_rounded_rect(hdc, &overlay, 16, g_brush_card, CLR_ACCENT);
    
    RECT title_rc = {40, 40, rc.right - 40, 90};
    draw_text_centered(hdc, TUTORIAL_TITLES[g_tutorial_step], &title_rc, g_font_title, CLR_ACCENT);
    
    RECT text_rc = {50, 100, rc.right - 50, rc.bottom - 100};
    SetTextColor(hdc, CLR_TEXT);
    SetBkMode(hdc, TRANSPARENT);
    HFONT old = SelectObject(hdc, g_font_normal);
    DrawTextA(hdc, TUTORIAL_TEXTS[g_tutorial_step], -1, &text_rc, DT_LEFT | DT_WORDBREAK);
    SelectObject(hdc, old);
    
    char progress[32];
    snprintf(progress, sizeof(progress), "Step %d of %d", g_tutorial_step + 1, 5);
    RECT prog_rc = {40, rc.bottom - 80, rc.right / 2, rc.bottom - 50};
    draw_text_left(hdc, progress, &prog_rc, g_font_small, CLR_TEXT_DIM);
    
    RECT btn_rc = {rc.right - 160, rc.bottom - 80, rc.right - 40, rc.bottom - 40};
    draw_rounded_rect(hdc, &btn_rc, 8, g_brush_accent, CLR_ACCENT);
    const char* btn_text = (g_tutorial_step < 4) ? "NEXT" : "GET STARTED";
    draw_text_centered(hdc, btn_text, &btn_rc, g_font_button, CLR_BG_DARK);
    
    if (g_tutorial_step > 0) {
        RECT back_rc = {rc.right - 280, rc.bottom - 80, rc.right - 180, rc.bottom - 40};
        draw_rounded_rect(hdc, &back_rc, 8, g_brush_input, CLR_BORDER);
        draw_text_centered(hdc, "BACK", &back_rc, g_font_normal, CLR_TEXT_DIM);
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
            
            EnumChildWindows(hwnd, (WNDENUMPROC)SendMessageA, (LPARAM)font);
            return 0;
        }
        
        case WM_COMMAND:
            if (LOWORD(wp) == 106) {
                OPENFILENAMEA ofn = {0};
                char file[512] = {0};
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hwnd;
                ofn.lpstrFilter = "All Files\0*.*\0SSH Keys\0*.pem\0";
                ofn.lpstrFile = file;
                ofn.nMaxFile = sizeof(file);
                ofn.Flags = OFN_FILEMUSTEXIST;
                if (GetOpenFileNameA(&ofn)) {
                    SetWindowTextA(g_add_edit_key, file);
                }
            } else if (LOWORD(wp) == IDOK) {
                GetWindowTextA(g_add_edit_name, g_add_name, sizeof(g_add_name));
                GetWindowTextA(g_add_edit_host, g_add_host, sizeof(g_add_host));
                GetWindowTextA(g_add_edit_port, g_add_port, sizeof(g_add_port));
                GetWindowTextA(g_add_edit_key, g_add_key, sizeof(g_add_key));
                GetWindowTextA(g_add_edit_user, g_add_user, sizeof(g_add_user));
                
                if (strlen(g_add_name) == 0 || strlen(g_add_host) == 0) {
                    MessageBoxA(hwnd, "Please enter Name and Host.", "Missing Info", MB_OK);
                    return 0;
                }
                
                if (g_ctx) {
                    int port = atoi(g_add_port);
                    if (port <= 0) port = 443;
                    hw_add_server(g_ctx, g_add_name, g_add_host, (uint16_t)port, g_add_key, g_add_user);
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
    
    int w = 320, h = 300;
    RECT prc;
    GetWindowRect(parent, &prc);
    int x = prc.left + (prc.right - prc.left - w) / 2;
    int y = prc.top + (prc.bottom - prc.top - h) / 2;
    
    g_add_dlg = CreateWindowExA(WS_EX_DLGMODALFRAME, "AddServerDlg", "Add Server",
        WS_POPUP | WS_CAPTION | WS_SYSMENU, x, y, w, h, parent, NULL, GetModuleHandle(NULL), NULL);
    
    ShowWindow(g_add_dlg, SW_SHOW);
    UpdateWindow(g_add_dlg);
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
            g_font_title = CreateFontA(32, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
            g_font_normal = CreateFontA(15, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
            g_font_button = CreateFontA(16, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
            g_font_small = CreateFontA(12, 0, 0, 0, FW_MEDIUM, 0, 0, 0, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
            
            g_brush_bg = CreateSolidBrush(CLR_BG_DARK);
            g_brush_card = CreateSolidBrush(CLR_BG_CARD);
            g_brush_input = CreateSolidBrush(CLR_BG_INPUT);
            g_brush_accent = CreateSolidBrush(CLR_ACCENT);
            
            RECT rc;
            GetClientRect(hwnd, &rc);
            init_layout(rc.right, rc.bottom);
            
            if (g_ctx) {
                g_selected_server = g_ctx->current_server >= 0 ? g_ctx->current_server : 0;
                g_full_tunnel = g_ctx->mode == HW_MODE_FULL_TUNNEL;
                g_kill_switch = g_ctx->kill_switch;
            }
            
            g_show_tutorial = check_first_run();
            g_tutorial_step = 0;
            
            SetTimer(hwnd, ID_TIMER_UPDATE, 1000, NULL);
            return 0;
        }
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);
            HDC mem = CreateCompatibleDC(hdc);
            HBITMAP bmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            HBITMAP old = SelectObject(mem, bmp);
            
            if (g_show_tutorial) {
                paint_tutorial(hwnd, mem);
            } else {
                paint_main(hwnd, mem);
            }
            
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
            g_hover_connect = PtInRect(&g_rect_btn_connect, (POINT){x, y});
            g_hover_add = PtInRect(&g_rect_btn_add, (POINT){x, y});
            g_hover_dropdown = PtInRect(&g_rect_server_box, (POINT){x, y});
            if (h1 != g_hover_connect || h2 != g_hover_add || h3 != g_hover_dropdown)
                InvalidateRect(hwnd, NULL, FALSE);
            TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, hwnd, 0};
            TrackMouseEvent(&tme);
            return 0;
        }
        
        case WM_MOUSELEAVE:
            g_hover_connect = g_hover_add = g_hover_dropdown = 0;
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        
        case WM_LBUTTONDOWN: {
            int x = GET_X_LPARAM(lp), y = GET_Y_LPARAM(lp);
            RECT rc;
            GetClientRect(hwnd, &rc);
            
            if (g_show_tutorial) {
                RECT next_btn = {rc.right - 160, rc.bottom - 80, rc.right - 40, rc.bottom - 40};
                RECT back_btn = {rc.right - 280, rc.bottom - 80, rc.right - 180, rc.bottom - 40};
                
                if (PtInRect(&next_btn, (POINT){x, y})) {
                    if (g_tutorial_step < 4) g_tutorial_step++;
                    else g_show_tutorial = 0;
                    InvalidateRect(hwnd, NULL, TRUE);
                } else if (g_tutorial_step > 0 && PtInRect(&back_btn, (POINT){x, y})) {
                    g_tutorial_step--;
                    InvalidateRect(hwnd, NULL, TRUE);
                }
                return 0;
            }
            
            if (PtInRect(&g_rect_btn_connect, (POINT){x, y})) {
                if (g_ctx) {
                    if (g_ctx->status == HW_CONNECTED) {
                        hw_disconnect(g_ctx);
                    } else if (g_ctx->status == HW_DISCONNECTED) {
                        if (g_ctx->server_count == 0) {
                            MessageBoxA(hwnd, "Please add a server first using the + ADD button.", "No Server", MB_OK);
                        } else {
                            g_ctx->current_server = g_selected_server;
                            g_ctx->mode = g_full_tunnel ? HW_MODE_FULL_TUNNEL : HW_MODE_PROXY;
                            g_ctx->kill_switch = g_kill_switch;
                            hw_connect(g_ctx);
                        }
                    }
                }
                InvalidateRect(hwnd, NULL, TRUE);
            } else if (PtInRect(&g_rect_btn_add, (POINT){x, y})) {
                show_add_server_dialog(hwnd);
            } else if (PtInRect(&g_rect_server_box, (POINT){x, y})) {
                g_dropdown_open = !g_dropdown_open;
                InvalidateRect(hwnd, NULL, FALSE);
            } else if (g_dropdown_open && PtInRect(&g_rect_server_dropdown, (POINT){x, y})) {
                int idx = (y - g_rect_server_dropdown.top - 12) / 40;
                if (idx >= 0 && g_ctx && idx < g_ctx->server_count) g_selected_server = idx;
                g_dropdown_open = 0;
                InvalidateRect(hwnd, NULL, FALSE);
            } else if (PtInRect(&g_rect_chk_full, (POINT){x, y})) {
                g_full_tunnel = !g_full_tunnel;
                InvalidateRect(hwnd, NULL, FALSE);
            } else if (PtInRect(&g_rect_chk_kill, (POINT){x, y})) {
                g_kill_switch = !g_kill_switch;
                InvalidateRect(hwnd, NULL, FALSE);
            } else if (g_dropdown_open) {
                g_dropdown_open = 0;
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }
        
        case WM_ERASEBKGND:
            return 1;
        
        case WM_DESTROY:
            KillTimer(hwnd, ID_TIMER_UPDATE);
            DeleteObject(g_font_title);
            DeleteObject(g_font_normal);
            DeleteObject(g_font_button);
            DeleteObject(g_font_small);
            DeleteObject(g_brush_bg);
            DeleteObject(g_brush_card);
            DeleteObject(g_brush_input);
            DeleteObject(g_brush_accent);
            PostQuitMessage(0);
            return 0;
        
        case WM_CLOSE:
            if (g_ctx && g_ctx->status == HW_CONNECTED) {
                if (MessageBoxA(hwnd, "Still connected. Disconnect and exit?", "HelloWorld", MB_YESNO) != IDYES)
                    return 0;
                hw_disconnect(g_ctx);
            }
            DestroyWindow(hwnd);
            return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show) {
    (void)prev; (void)cmd;
    SetProcessDPIAware();
    g_ctx = hw_create();
    
    WNDCLASSEXA wc = {sizeof(wc), CS_HREDRAW | CS_VREDRAW, window_proc, 0, 0, inst,
        NULL, LoadCursor(NULL, IDC_ARROW), NULL, NULL, "HelloWorldClass", NULL};
    RegisterClassExA(&wc);
    
    int w = 380, h = 520;
    int x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
    
    g_hwnd = CreateWindowExA(WS_EX_APPWINDOW, "HelloWorldClass", "HelloWorld",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, w, h, NULL, NULL, inst, NULL);
    
    ShowWindow(g_hwnd, show);
    UpdateWindow(g_hwnd);
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    hw_destroy(g_ctx);
    return (int)msg.wParam;
}

#endif
