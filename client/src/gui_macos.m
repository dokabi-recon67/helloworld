/*
 * HelloWorld - macOS GUI
 */

#ifndef _WIN32
#ifdef __APPLE__

#import <Cocoa/Cocoa.h>
#include "helloworld.h"

static hw_ctx_t* g_ctx = NULL;

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSWindow* window;
@property (strong) NSButton* connectButton;
@property (strong) NSPopUpButton* serverPopup;
@property (strong) NSButton* fullModeCheck;
@property (strong) NSButton* killSwitchCheck;
@property (strong) NSTextField* statusLabel;
@property (strong) NSTextField* ipLabel;
@property (strong) NSTextField* timeLabel;
@property (strong) NSTimer* updateTimer;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    NSRect frame = NSMakeRect(0, 0, 300, 420);
    NSUInteger style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | 
                       NSWindowStyleMaskMiniaturizable;
    
    self.window = [[NSWindow alloc] initWithContentRect:frame
                                              styleMask:style
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    [self.window setTitle:@"HelloWorld"];
    [self.window center];
    
    NSView* content = [self.window contentView];
    [content setWantsLayer:YES];
    [[content layer] setBackgroundColor:[[NSColor colorWithRed:0.094 green:0.094 blue:0.11 alpha:1.0] CGColor]];
    
    NSTextField* title = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 360, 260, 40)];
    [title setStringValue:@"HelloWorld"];
    [title setBezeled:NO];
    [title setDrawsBackground:NO];
    [title setEditable:NO];
    [title setSelectable:NO];
    [title setAlignment:NSTextAlignmentCenter];
    [title setFont:[NSFont boldSystemFontOfSize:24]];
    [title setTextColor:[NSColor whiteColor]];
    [content addSubview:title];
    
    self.serverPopup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(20, 310, 260, 30) pullsDown:NO];
    for (int i = 0; i < g_ctx->server_count; i++) {
        [self.serverPopup addItemWithTitle:[NSString stringWithUTF8String:g_ctx->servers[i].name]];
    }
    if (g_ctx->current_server >= 0) {
        [self.serverPopup selectItemAtIndex:g_ctx->current_server];
    }
    [content addSubview:self.serverPopup];
    
    self.connectButton = [[NSButton alloc] initWithFrame:NSMakeRect(20, 250, 260, 50)];
    [self.connectButton setTitle:@"CONNECT"];
    [self.connectButton setBezelStyle:NSBezelStyleRounded];
    [self.connectButton setTarget:self];
    [self.connectButton setAction:@selector(connectClicked:)];
    [self.connectButton setFont:[NSFont boldSystemFontOfSize:18]];
    [content addSubview:self.connectButton];
    
    self.fullModeCheck = [[NSButton alloc] initWithFrame:NSMakeRect(20, 210, 260, 24)];
    [self.fullModeCheck setButtonType:NSButtonTypeSwitch];
    [self.fullModeCheck setTitle:@"Full Tunnel Mode"];
    [[self.fullModeCheck cell] setTextColor:[NSColor whiteColor]];
    if (g_ctx->mode == HW_MODE_FULL_TUNNEL) {
        [self.fullModeCheck setState:NSControlStateValueOn];
    }
    [content addSubview:self.fullModeCheck];
    
    self.killSwitchCheck = [[NSButton alloc] initWithFrame:NSMakeRect(20, 180, 260, 24)];
    [self.killSwitchCheck setButtonType:NSButtonTypeSwitch];
    [self.killSwitchCheck setTitle:@"Kill Switch"];
    [[self.killSwitchCheck cell] setTextColor:[NSColor whiteColor]];
    if (g_ctx->kill_switch) {
        [self.killSwitchCheck setState:NSControlStateValueOn];
    }
    [content addSubview:self.killSwitchCheck];
    
    self.statusLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 130, 260, 24)];
    [self.statusLabel setStringValue:@"Disconnected"];
    [self.statusLabel setBezeled:NO];
    [self.statusLabel setDrawsBackground:NO];
    [self.statusLabel setEditable:NO];
    [self.statusLabel setSelectable:NO];
    [self.statusLabel setAlignment:NSTextAlignmentCenter];
    [self.statusLabel setTextColor:[NSColor lightGrayColor]];
    [content addSubview:self.statusLabel];
    
    self.ipLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 100, 260, 24)];
    [self.ipLabel setStringValue:@"IP: --"];
    [self.ipLabel setBezeled:NO];
    [self.ipLabel setDrawsBackground:NO];
    [self.ipLabel setEditable:NO];
    [self.ipLabel setSelectable:NO];
    [self.ipLabel setAlignment:NSTextAlignmentCenter];
    [self.ipLabel setTextColor:[NSColor lightGrayColor]];
    [content addSubview:self.ipLabel];
    
    self.timeLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 70, 260, 24)];
    [self.timeLabel setStringValue:@"Time: --:--:--"];
    [self.timeLabel setBezeled:NO];
    [self.timeLabel setDrawsBackground:NO];
    [self.timeLabel setEditable:NO];
    [self.timeLabel setSelectable:NO];
    [self.timeLabel setAlignment:NSTextAlignmentCenter];
    [self.timeLabel setTextColor:[NSColor lightGrayColor]];
    [content addSubview:self.timeLabel];
    
    NSButton* settingsBtn = [[NSButton alloc] initWithFrame:NSMakeRect(20, 20, 260, 30)];
    [settingsBtn setTitle:@"Settings"];
    [settingsBtn setBezelStyle:NSBezelStyleRounded];
    [settingsBtn setTarget:self];
    [settingsBtn setAction:@selector(settingsClicked:)];
    [content addSubview:settingsBtn];
    
    self.updateTimer = [NSTimer scheduledTimerWithTimeInterval:1.0
                                                        target:self
                                                      selector:@selector(updateUI)
                                                      userInfo:nil
                                                       repeats:YES];
    
    [self.window makeKeyAndOrderFront:nil];
}

- (void)connectClicked:(id)sender {
    if (g_ctx->status == HW_CONNECTED) {
        hw_disconnect(g_ctx);
    } else if (g_ctx->status == HW_DISCONNECTED) {
        NSInteger sel = [self.serverPopup indexOfSelectedItem];
        if (sel >= 0) {
            g_ctx->current_server = (int)sel;
            g_ctx->mode = ([self.fullModeCheck state] == NSControlStateValueOn) 
                          ? HW_MODE_FULL_TUNNEL : HW_MODE_PROXY;
            g_ctx->kill_switch = ([self.killSwitchCheck state] == NSControlStateValueOn);
            hw_connect(g_ctx);
        }
    }
    [self updateUI];
}

- (void)settingsClicked:(id)sender {
    NSAlert* alert = [[NSAlert alloc] init];
    [alert setMessageText:@"HelloWorld Settings"];
    [alert setInformativeText:[NSString stringWithFormat:
        @"Version: %s\n"
        @"Servers: %d\n"
        @"Config: %s\n\n"
        @"Edit config.txt to add servers.",
        HW_VERSION, g_ctx->server_count, g_ctx->config_dir]];
    [alert addButtonWithTitle:@"OK"];
    [alert runModal];
}

- (void)updateUI {
    [self.statusLabel setStringValue:[NSString stringWithUTF8String:hw_status_str(g_ctx->status)]];
    
    if (g_ctx->status == HW_CONNECTED && g_ctx->stats.public_ip[0]) {
        [self.ipLabel setStringValue:[NSString stringWithFormat:@"IP: %s", g_ctx->stats.public_ip]];
    } else {
        [self.ipLabel setStringValue:@"IP: --"];
    }
    
    if (g_ctx->status == HW_CONNECTED) {
        time_t elapsed = time(NULL) - g_ctx->stats.start_time;
        int h = (int)(elapsed / 3600);
        int m = (int)((elapsed % 3600) / 60);
        int s = (int)(elapsed % 60);
        [self.timeLabel setStringValue:[NSString stringWithFormat:@"Time: %02d:%02d:%02d", h, m, s]];
        [self.connectButton setTitle:@"DISCONNECT"];
    } else if (g_ctx->status == HW_CONNECTING) {
        [self.timeLabel setStringValue:@"Time: --:--:--"];
        [self.connectButton setTitle:@"CONNECTING..."];
    } else {
        [self.timeLabel setStringValue:@"Time: --:--:--"];
        [self.connectButton setTitle:@"CONNECT"];
    }
    
    [self.connectButton setEnabled:(g_ctx->status != HW_CONNECTING)];
    [self.serverPopup setEnabled:(g_ctx->status == HW_DISCONNECTED)];
    [self.fullModeCheck setEnabled:(g_ctx->status == HW_DISCONNECTED)];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    return YES;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender {
    if (g_ctx && g_ctx->status == HW_CONNECTED) {
        hw_disconnect(g_ctx);
    }
    return NSTerminateNow;
}

@end

int main(int argc, const char* argv[]) {
    g_ctx = hw_create();
    if (!g_ctx) {
        fprintf(stderr, "Failed to initialize\n");
        return 1;
    }
    
    @autoreleasepool {
        NSApplication* app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        
        AppDelegate* delegate = [[AppDelegate alloc] init];
        [app setDelegate:delegate];
        
        [app activateIgnoringOtherApps:YES];
        [app run];
    }
    
    hw_destroy(g_ctx);
    return 0;
}

#endif
#endif

