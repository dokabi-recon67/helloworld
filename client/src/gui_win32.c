/*
 * HelloWorld - Windows GUI
 * Modern Dark Theme UI
 */

#ifdef _WIN32

#include "helloworld.h"
#include <commctrl.h>
#include <windowsx.h>
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define ID_BTN_CONNECT   1001
#define ID_BTN_SETTINGS  1002
#define ID_CMB_SERVER    1003
#define ID_CHK_FULLMODE  1004
#define ID_CHK_KILLSW    1005
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
#define CLR_CONNECTED    RGB(0, 255, 136)

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

static RECT g_rect_title;
static RECT g_rect_server_label;
static RECT g_rect_server_box;
static RECT g_rect_server_dropdown;
static RECT g_rect_btn_connect;
static RECT g_rect_chk_full;
static RECT g_rect_chk_kill;
static RECT g_rect_status;
static RECT g_rect_ip;
static RECT g_rect_time;
static RECT g_rect_btn_settings;

static int g_hover_connect = 0;
static int g_hover_settings = 0;
static int g_hover_dropdown = 0;
static int g_dropdown_open = 0;

static void init_layout(int w, int h) {
    int margin = 24;
    int card_x = margin;
    int card_w = w - margin * 2;
    int y = 30;
    
    SetRect(&g_rect_title, card_x, y, card_x + card_w, y + 50);
    y += 70;
    
    SetRect(&g_rect_server_label, card_x, y, card_x + card_w, y + 20);
    y += 24;
    
    SetRect(&g_rect_server_box, card_x, y, card_x + card_w, y + 44);
    SetRect(&g_rect_server_dropdown, card_x, y + 44, card_x + card_w, y + 44 + 120);
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
    y += 44;
    
    SetRect(&g_rect_btn_settings, card_x, y, card_x + card_w, y + 36);
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

static void draw_checkbox(HDC hdc, RECT* rc, const char* text, int checked, int hover) {
    int box_size = 20;
    RECT box_rc = {rc->left, rc->top + 4, rc->left + box_size, rc->top + 4 + box_size};
    
    HBRUSH fill = checked ? g_brush_accent : g_brush_input;
    draw_rounded_rect(hdc, &box_rc, 4, fill, checked ? CLR_ACCENT : CLR_BORDER);
    
    if (checked) {
        SetTextColor(hdc, CLR_BG_DARK);
        SetBkMode(hdc, TRANSPARENT);
        HFONT old = SelectObject(hdc, g_font_small);
        RECT check_rc = box_rc;
        DrawTextA(hdc, "V", -1, &check_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, old);
    }
    
    RECT text_rc = {rc->left + box_size + 12, rc->top, rc->right, rc->bottom};
    draw_text_left(hdc, text, &text_rc, g_font_normal, hover ? CLR_TEXT : CLR_TEXT_DIM);
}

static void paint_window(HWND hwnd, HDC hdc) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    
    HBRUSH bg = CreateSolidBrush(CLR_BG_DARK);
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);
    
    draw_text_centered(hdc, "HelloWorld", &g_rect_title, g_font_title, CLR_ACCENT);
    
    draw_text_left(hdc, "SERVER", &g_rect_server_label, g_font_small, CLR_TEXT_DIM);
    
    draw_rounded_rect(hdc, &g_rect_server_box, 8, g_brush_input, 
                      g_hover_dropdown ? CLR_ACCENT : CLR_BORDER);
    
    const char* server_name = "Select a server...";
    if (g_ctx && g_ctx->server_count > 0 && g_selected_server >= 0 && g_selected_server < g_ctx->server_count) {
        server_name = g_ctx->servers[g_selected_server].name;
    } else if (g_ctx && g_ctx->server_count == 0) {
        server_name = "No servers configured";
    }
    
    RECT server_text_rc = g_rect_server_box;
    server_text_rc.left += 16;
    server_text_rc.right -= 40;
    draw_text_left(hdc, server_name, &server_text_rc, g_font_normal, CLR_TEXT);
    
    RECT arrow_rc = g_rect_server_box;
    arrow_rc.left = arrow_rc.right - 40;
    draw_text_centered(hdc, g_dropdown_open ? "^" : "v", &arrow_rc, g_font_normal, CLR_TEXT_DIM);
    
    if (g_dropdown_open && g_ctx && g_ctx->server_count > 0) {
        draw_rounded_rect(hdc, &g_rect_server_dropdown, 8, g_brush_card, CLR_BORDER);
        
        int item_height = 36;
        for (int i = 0; i < g_ctx->server_count && i < 3; i++) {
            RECT item_rc = {
                g_rect_server_dropdown.left + 8,
                g_rect_server_dropdown.top + 8 + i * item_height,
                g_rect_server_dropdown.right - 8,
                g_rect_server_dropdown.top + 8 + (i + 1) * item_height
            };
            
            COLORREF item_color = (i == g_selected_server) ? CLR_ACCENT : CLR_TEXT;
            draw_text_left(hdc, g_ctx->servers[i].name, &item_rc, g_font_normal, item_color);
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
        btn_brush = CreateSolidBrush(CLR_BG_INPUT);
        btn_text = CLR_TEXT_DIM;
        btn_label = "CONNECTING...";
    } else {
        btn_brush = g_hover_connect ? CreateSolidBrush(CLR_ACCENT_DARK) : g_brush_accent;
        btn_text = CLR_BG_DARK;
        btn_label = "CONNECT";
    }
    
    draw_rounded_rect(hdc, &g_rect_btn_connect, 8, btn_brush, 
                      g_ctx && g_ctx->status == HW_CONNECTED ? CLR_ERROR : CLR_ACCENT);
    draw_text_centered(hdc, btn_label, &g_rect_btn_connect, g_font_button, btn_text);
    
    if (btn_brush != g_brush_accent) DeleteObject(btn_brush);
    
    draw_checkbox(hdc, &g_rect_chk_full, "Full Tunnel (route all traffic)", g_full_tunnel, 0);
    draw_checkbox(hdc, &g_rect_chk_kill, "Kill Switch (block if disconnected)", g_kill_switch, 0);
    
    const char* status_text;
    COLORREF status_color;
    
    if (g_ctx) {
        switch (g_ctx->status) {
            case HW_CONNECTED:
                status_text = "CONNECTED";
                status_color = CLR_CONNECTED;
                break;
            case HW_CONNECTING:
                status_text = "CONNECTING...";
                status_color = CLR_TEXT_DIM;
                break;
            case HW_ERROR:
                status_text = "ERROR";
                status_color = CLR_ERROR;
                break;
            default:
                status_text = "DISCONNECTED";
                status_color = CLR_TEXT_DIM;
        }
    } else {
        status_text = "INITIALIZING";
        status_color = CLR_TEXT_DIM;
    }
    
    draw_text_centered(hdc, status_text, &g_rect_status, g_font_normal, status_color);
    
    char ip_text[128];
    if (g_ctx && g_ctx->status == HW_CONNECTED && g_ctx->stats.public_ip[0]) {
        snprintf(ip_text, sizeof(ip_text), "IP: %s", g_ctx->stats.public_ip);
    } else {
        snprintf(ip_text, sizeof(ip_text), "IP: ---");
    }
    draw_text_centered(hdc, ip_text, &g_rect_ip, g_font_small, CLR_TEXT_DIM);
    
    char time_text[128];
    if (g_ctx && g_ctx->status == HW_CONNECTED) {
        time_t elapsed = time(NULL) - g_ctx->stats.start_time;
        int h = (int)(elapsed / 3600);
        int m = (int)((elapsed % 3600) / 60);
        int s = (int)(elapsed % 60);
        snprintf(time_text, sizeof(time_text), "Duration: %02d:%02d:%02d", h, m, s);
    } else {
        snprintf(time_text, sizeof(time_text), "Duration: --:--:--");
    }
    draw_text_centered(hdc, time_text, &g_rect_time, g_font_small, CLR_TEXT_DIM);
    
    draw_rounded_rect(hdc, &g_rect_btn_settings, 8, g_brush_card,
                      g_hover_settings ? CLR_ACCENT : CLR_BORDER);
    draw_text_centered(hdc, "Settings", &g_rect_btn_settings, g_font_normal,
                       g_hover_settings ? CLR_ACCENT : CLR_TEXT_DIM);
}

static void on_connect_click(void) {
    if (!g_ctx) return;
    
    if (g_ctx->status == HW_CONNECTED) {
        hw_disconnect(g_ctx);
    } else if (g_ctx->status == HW_DISCONNECTED) {
        if (g_ctx->server_count == 0) {
            MessageBoxA(g_hwnd, 
                "No servers configured.\n\n"
                "Edit config.txt in:\n"
                "%APPDATA%\\HelloWorld\\config.txt\n\n"
                "Add a [server] section with:\n"
                "name = My Server\n"
                "host = your.server.ip\n"
                "port = 443\n"
                "key = C:\\path\\to\\ssh\\key\n"
                "user = root",
                "HelloWorld", MB_OK | MB_ICONINFORMATION);
            return;
        }
        
        g_ctx->current_server = g_selected_server;
        g_ctx->mode = g_full_tunnel ? HW_MODE_FULL_TUNNEL : HW_MODE_PROXY;
        g_ctx->kill_switch = g_kill_switch;
        hw_connect(g_ctx);
    }
    
    InvalidateRect(g_hwnd, NULL, TRUE);
}

static void show_settings(void) {
    char msg[1024];
    char config_path[512];
    
    if (g_ctx) {
        snprintf(config_path, sizeof(config_path), "%s\\config.txt", g_ctx->config_dir);
    } else {
        snprintf(config_path, sizeof(config_path), "%%APPDATA%%\\HelloWorld\\config.txt");
    }
    
    snprintf(msg, sizeof(msg),
        "HelloWorld v%s\n\n"
        "Config file:\n%s\n\n"
        "Servers: %d\n\n"
        "To add a server, create/edit config.txt:\n\n"
        "[server]\n"
        "name = My VPS\n"
        "host = 123.45.67.89\n"
        "port = 443\n"
        "key = C:\\Users\\You\\.ssh\\id_ed25519\n"
        "user = root\n\n"
        "Need help? github.com/dokabi-recon67/helloworld",
        HW_VERSION,
        config_path,
        g_ctx ? g_ctx->server_count : 0);
    
    MessageBoxA(g_hwnd, msg, "HelloWorld Settings", MB_OK | MB_ICONINFORMATION);
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE: {
            g_font_title = CreateFontA(36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
            g_font_normal = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
            g_font_button = CreateFontA(18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
            g_font_small = CreateFontA(12, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
            
            g_brush_bg = CreateSolidBrush(CLR_BG_DARK);
            g_brush_card = CreateSolidBrush(CLR_BG_CARD);
            g_brush_input = CreateSolidBrush(CLR_BG_INPUT);
            g_brush_accent = CreateSolidBrush(CLR_ACCENT);
            
            RECT rc;
            GetClientRect(hwnd, &rc);
            init_layout(rc.right, rc.bottom);
            
            if (g_ctx && g_ctx->current_server >= 0) {
                g_selected_server = g_ctx->current_server;
            }
            g_full_tunnel = g_ctx && g_ctx->mode == HW_MODE_FULL_TUNNEL;
            g_kill_switch = g_ctx && g_ctx->kill_switch;
            
            SetTimer(hwnd, ID_TIMER_UPDATE, 1000, NULL);
            return 0;
        }
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            RECT rc;
            GetClientRect(hwnd, &rc);
            HDC mem_dc = CreateCompatibleDC(hdc);
            HBITMAP mem_bmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            HBITMAP old_bmp = SelectObject(mem_dc, mem_bmp);
            
            paint_window(hwnd, mem_dc);
            
            BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem_dc, 0, 0, SRCCOPY);
            
            SelectObject(mem_dc, old_bmp);
            DeleteObject(mem_bmp);
            DeleteDC(mem_dc);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_TIMER:
            if (wp == ID_TIMER_UPDATE) {
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        
        case WM_MOUSEMOVE: {
            int x = GET_X_LPARAM(lp);
            int y = GET_Y_LPARAM(lp);
            
            int old_hover_connect = g_hover_connect;
            int old_hover_settings = g_hover_settings;
            int old_hover_dropdown = g_hover_dropdown;
            
            g_hover_connect = PtInRect(&g_rect_btn_connect, (POINT){x, y});
            g_hover_settings = PtInRect(&g_rect_btn_settings, (POINT){x, y});
            g_hover_dropdown = PtInRect(&g_rect_server_box, (POINT){x, y});
            
            if (g_hover_connect != old_hover_connect ||
                g_hover_settings != old_hover_settings ||
                g_hover_dropdown != old_hover_dropdown) {
                InvalidateRect(hwnd, NULL, FALSE);
            }
            
            TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, hwnd, 0};
            TrackMouseEvent(&tme);
            return 0;
        }
        
        case WM_MOUSELEAVE:
            g_hover_connect = 0;
            g_hover_settings = 0;
            g_hover_dropdown = 0;
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        
        case WM_LBUTTONDOWN: {
            int x = GET_X_LPARAM(lp);
            int y = GET_Y_LPARAM(lp);
            
            if (PtInRect(&g_rect_btn_connect, (POINT){x, y})) {
                on_connect_click();
            } else if (PtInRect(&g_rect_btn_settings, (POINT){x, y})) {
                show_settings();
            } else if (PtInRect(&g_rect_server_box, (POINT){x, y})) {
                g_dropdown_open = !g_dropdown_open;
                InvalidateRect(hwnd, NULL, FALSE);
            } else if (g_dropdown_open && PtInRect(&g_rect_server_dropdown, (POINT){x, y})) {
                int item_height = 36;
                int idx = (y - g_rect_server_dropdown.top - 8) / item_height;
                if (idx >= 0 && g_ctx && idx < g_ctx->server_count) {
                    g_selected_server = idx;
                }
                g_dropdown_open = 0;
                InvalidateRect(hwnd, NULL, FALSE);
            } else if (PtInRect(&g_rect_chk_full, (POINT){x, y})) {
                g_full_tunnel = !g_full_tunnel;
                InvalidateRect(hwnd, NULL, FALSE);
            } else if (PtInRect(&g_rect_chk_kill, (POINT){x, y})) {
                g_kill_switch = !g_kill_switch;
                InvalidateRect(hwnd, NULL, FALSE);
            } else {
                if (g_dropdown_open) {
                    g_dropdown_open = 0;
                    InvalidateRect(hwnd, NULL, FALSE);
                }
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
                int res = MessageBoxA(hwnd,
                    "You are still connected.\nDisconnect and exit?",
                    "HelloWorld", MB_YESNO | MB_ICONQUESTION);
                if (res != IDYES) return 0;
                hw_disconnect(g_ctx);
            }
            DestroyWindow(hwnd);
            return 0;
    }
    
    return DefWindowProcA(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show) {
    (void)prev;
    (void)cmd;
    
    SetProcessDPIAware();
    
    g_ctx = hw_create();
    
    WNDCLASSEXA wc = {0};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = window_proc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "HelloWorldClass";
    wc.hbrBackground = NULL;
    
    RegisterClassExA(&wc);
    
    int width = 340;
    int height = 520;
    int x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
    
    g_hwnd = CreateWindowExA(
        WS_EX_APPWINDOW,
        "HelloWorldClass",
        "HelloWorld",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, width, height,
        NULL, NULL, inst, NULL);
    
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
