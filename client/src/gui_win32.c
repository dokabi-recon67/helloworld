/*
 * HelloWorld - Windows GUI
 */

#ifdef _WIN32

#include "helloworld.h"
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define ID_BTN_CONNECT   1001
#define ID_BTN_SETTINGS  1002
#define ID_CMB_SERVER    1003
#define ID_CHK_FULLMODE  1004
#define ID_CHK_KILLSW    1005
#define ID_LBL_STATUS    1006
#define ID_LBL_IP        1007
#define ID_LBL_TIME      1008
#define ID_TIMER_UPDATE  2001

static hw_ctx_t* g_ctx = NULL;
static HWND g_hwnd = NULL;
static HWND g_btn_connect = NULL;
static HWND g_cmb_server = NULL;
static HWND g_lbl_status = NULL;
static HWND g_lbl_ip = NULL;
static HWND g_lbl_time = NULL;
static HWND g_chk_fullmode = NULL;
static HWND g_chk_killsw = NULL;
static HFONT g_font_normal = NULL;
static HFONT g_font_large = NULL;
static HBRUSH g_brush_bg = NULL;

static void update_ui(void) {
    if (!g_ctx) return;
    
    const char* status_text = hw_status_str(g_ctx->status);
    SetWindowTextA(g_lbl_status, status_text);
    
    char ip_text[128];
    if (g_ctx->status == HW_CONNECTED && g_ctx->stats.public_ip[0]) {
        snprintf(ip_text, sizeof(ip_text), "IP: %s", g_ctx->stats.public_ip);
    } else {
        snprintf(ip_text, sizeof(ip_text), "IP: --");
    }
    SetWindowTextA(g_lbl_ip, ip_text);
    
    char time_text[128];
    if (g_ctx->status == HW_CONNECTED) {
        time_t elapsed = time(NULL) - g_ctx->stats.start_time;
        int hours = (int)(elapsed / 3600);
        int mins = (int)((elapsed % 3600) / 60);
        int secs = (int)(elapsed % 60);
        snprintf(time_text, sizeof(time_text), "Time: %02d:%02d:%02d", hours, mins, secs);
    } else {
        snprintf(time_text, sizeof(time_text), "Time: --:--:--");
    }
    SetWindowTextA(g_lbl_time, time_text);
    
    if (g_ctx->status == HW_CONNECTED) {
        SetWindowTextA(g_btn_connect, "DISCONNECT");
    } else if (g_ctx->status == HW_CONNECTING) {
        SetWindowTextA(g_btn_connect, "CONNECTING...");
    } else {
        SetWindowTextA(g_btn_connect, "CONNECT");
    }
    
    EnableWindow(g_btn_connect, g_ctx->status != HW_CONNECTING);
    EnableWindow(g_cmb_server, g_ctx->status == HW_DISCONNECTED);
    EnableWindow(g_chk_fullmode, g_ctx->status == HW_DISCONNECTED);
}

static void populate_servers(void) {
    if (!g_ctx || !g_cmb_server) return;
    
    SendMessageA(g_cmb_server, CB_RESETCONTENT, 0, 0);
    
    for (int i = 0; i < g_ctx->server_count; i++) {
        SendMessageA(g_cmb_server, CB_ADDSTRING, 0, (LPARAM)g_ctx->servers[i].name);
    }
    
    if (g_ctx->current_server >= 0 && g_ctx->current_server < g_ctx->server_count) {
        SendMessageA(g_cmb_server, CB_SETCURSEL, g_ctx->current_server, 0);
    }
}

static void on_connect_click(void) {
    if (!g_ctx) return;
    
    if (g_ctx->status == HW_CONNECTED) {
        hw_disconnect(g_ctx);
    } else if (g_ctx->status == HW_DISCONNECTED) {
        int sel = (int)SendMessageA(g_cmb_server, CB_GETCURSEL, 0, 0);
        if (sel >= 0) {
            g_ctx->current_server = sel;
            g_ctx->mode = SendMessageA(g_chk_fullmode, BM_GETCHECK, 0, 0) == BST_CHECKED 
                          ? HW_MODE_FULL_TUNNEL : HW_MODE_PROXY;
            g_ctx->kill_switch = SendMessageA(g_chk_killsw, BM_GETCHECK, 0, 0) == BST_CHECKED;
            hw_connect(g_ctx);
        } else {
            MessageBoxA(g_hwnd, "Please select a server first.", "HelloWorld", MB_OK | MB_ICONWARNING);
        }
    }
    update_ui();
}

static void show_settings_dialog(HWND parent) {
    char msg[1024];
    snprintf(msg, sizeof(msg),
             "HelloWorld v%s\n\n"
             "Servers configured: %d\n"
             "Config directory:\n%s\n\n"
             "To add a server, edit config.txt in the config directory.\n\n"
             "Format:\n"
             "[server]\n"
             "name = My Server\n"
             "host = example.com\n"
             "port = 443\n"
             "key = /path/to/key\n"
             "user = root",
             HW_VERSION, g_ctx->server_count, g_ctx->config_dir);
    
    MessageBoxA(parent, msg, "Settings", MB_OK | MB_ICONINFORMATION);
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE: {
            g_brush_bg = CreateSolidBrush(RGB(24, 24, 28));
            g_font_normal = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                        DEFAULT_PITCH, "Segoe UI");
            g_font_large = CreateFontA(28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                       CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                       DEFAULT_PITCH, "Segoe UI");
            
            HWND lbl_title = CreateWindowA("STATIC", "HelloWorld",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                20, 20, 260, 40, hwnd, NULL, NULL, NULL);
            SendMessageA(lbl_title, WM_SETFONT, (WPARAM)g_font_large, TRUE);
            
            g_cmb_server = CreateWindowA("COMBOBOX", "",
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                20, 80, 260, 200, hwnd, (HMENU)ID_CMB_SERVER, NULL, NULL);
            SendMessageA(g_cmb_server, WM_SETFONT, (WPARAM)g_font_normal, TRUE);
            
            g_btn_connect = CreateWindowA("BUTTON", "CONNECT",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                20, 130, 260, 50, hwnd, (HMENU)ID_BTN_CONNECT, NULL, NULL);
            SendMessageA(g_btn_connect, WM_SETFONT, (WPARAM)g_font_large, TRUE);
            
            g_chk_fullmode = CreateWindowA("BUTTON", "Full Tunnel Mode",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                20, 200, 260, 24, hwnd, (HMENU)ID_CHK_FULLMODE, NULL, NULL);
            SendMessageA(g_chk_fullmode, WM_SETFONT, (WPARAM)g_font_normal, TRUE);
            
            g_chk_killsw = CreateWindowA("BUTTON", "Kill Switch",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                20, 230, 260, 24, hwnd, (HMENU)ID_CHK_KILLSW, NULL, NULL);
            SendMessageA(g_chk_killsw, WM_SETFONT, (WPARAM)g_font_normal, TRUE);
            
            g_lbl_status = CreateWindowA("STATIC", "Disconnected",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                20, 280, 260, 24, hwnd, (HMENU)ID_LBL_STATUS, NULL, NULL);
            SendMessageA(g_lbl_status, WM_SETFONT, (WPARAM)g_font_normal, TRUE);
            
            g_lbl_ip = CreateWindowA("STATIC", "IP: --",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                20, 310, 260, 24, hwnd, (HMENU)ID_LBL_IP, NULL, NULL);
            SendMessageA(g_lbl_ip, WM_SETFONT, (WPARAM)g_font_normal, TRUE);
            
            g_lbl_time = CreateWindowA("STATIC", "Time: --:--:--",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                20, 340, 260, 24, hwnd, (HMENU)ID_LBL_TIME, NULL, NULL);
            SendMessageA(g_lbl_time, WM_SETFONT, (WPARAM)g_font_normal, TRUE);
            
            HWND btn_settings = CreateWindowA("BUTTON", "Settings",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                20, 380, 260, 30, hwnd, (HMENU)ID_BTN_SETTINGS, NULL, NULL);
            SendMessageA(btn_settings, WM_SETFONT, (WPARAM)g_font_normal, TRUE);
            
            populate_servers();
            
            if (g_ctx->mode == HW_MODE_FULL_TUNNEL) {
                SendMessageA(g_chk_fullmode, BM_SETCHECK, BST_CHECKED, 0);
            }
            if (g_ctx->kill_switch) {
                SendMessageA(g_chk_killsw, BM_SETCHECK, BST_CHECKED, 0);
            }
            
            SetTimer(hwnd, ID_TIMER_UPDATE, 1000, NULL);
            return 0;
        }
        
        case WM_TIMER:
            if (wp == ID_TIMER_UPDATE) {
                update_ui();
            }
            return 0;
        
        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case ID_BTN_CONNECT:
                    on_connect_click();
                    break;
                case ID_BTN_SETTINGS:
                    show_settings_dialog(hwnd);
                    break;
            }
            return 0;
        
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN: {
            HDC hdc = (HDC)wp;
            SetTextColor(hdc, RGB(220, 220, 220));
            SetBkMode(hdc, TRANSPARENT);
            return (LRESULT)g_brush_bg;
        }
        
        case WM_ERASEBKGND: {
            HDC hdc = (HDC)wp;
            RECT rc;
            GetClientRect(hwnd, &rc);
            FillRect(hdc, &rc, g_brush_bg);
            return 1;
        }
        
        case WM_DESTROY:
            KillTimer(hwnd, ID_TIMER_UPDATE);
            DeleteObject(g_font_normal);
            DeleteObject(g_font_large);
            DeleteObject(g_brush_bg);
            PostQuitMessage(0);
            return 0;
        
        case WM_CLOSE:
            if (g_ctx && g_ctx->status == HW_CONNECTED) {
                int result = MessageBoxA(hwnd, 
                    "You are still connected. Disconnect and exit?",
                    "HelloWorld", MB_YESNO | MB_ICONQUESTION);
                if (result != IDYES) return 0;
            }
            DestroyWindow(hwnd);
            return 0;
    }
    
    return DefWindowProcA(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show) {
    (void)prev;
    (void)cmd;
    
    InitCommonControls();
    
    g_ctx = hw_create();
    if (!g_ctx) {
        MessageBoxA(NULL, "Failed to initialize application.", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    WNDCLASSEXA wc = {0};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = window_proc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "HelloWorldClass";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    
    if (!RegisterClassExA(&wc)) {
        MessageBoxA(NULL, "Failed to register window class.", "Error", MB_OK | MB_ICONERROR);
        hw_destroy(g_ctx);
        return 1;
    }
    
    int width = 320;
    int height = 460;
    int x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
    
    g_hwnd = CreateWindowExA(
        0, "HelloWorldClass", "HelloWorld",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, width, height,
        NULL, NULL, inst, NULL);
    
    if (!g_hwnd) {
        MessageBoxA(NULL, "Failed to create window.", "Error", MB_OK | MB_ICONERROR);
        hw_destroy(g_ctx);
        return 1;
    }
    
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

