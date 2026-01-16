/*
 * HelloWorld - macOS GUI
 */

#ifndef _WIN32
#ifdef __APPLE__

#import <Cocoa/Cocoa.h>
#include "helloworld.h"

static hw_ctx_t* g_ctx = NULL;
static BOOL g_showTutorial = NO;
static int g_tutorialStep = 0;

@interface AddServerWindow : NSWindowController
@property (strong) NSTextField* nameField;
@property (strong) NSTextField* hostField;
@property (strong) NSTextField* portField;
@property (strong) NSTextField* userField;
@property (strong) NSTextField* keyField;
@property (strong) NSButton* browseButton;
@property (strong) NSButton* addButton;
@property (strong) NSButton* cancelButton;
@end

@implementation AddServerWindow

- (instancetype)init {
    NSRect frame = NSMakeRect(0, 0, 400, 400);
    NSWindow* window = [[NSWindow alloc] initWithContentRect:frame
                                                   styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
    [window setTitle:@"Add Server"];
    [window center];
    
    self = [super initWithWindow:window];
    if (self) {
        NSView* content = [window contentView];
        [content setWantsLayer:YES];
        [[content layer] setBackgroundColor:[[NSColor colorWithRed:0.094 green:0.094 blue:0.11 alpha:1.0] CGColor]];
        
        int y = 350;
        
        NSTextField* nameLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, y, 100, 20)];
        [nameLabel setStringValue:@"Server Name:"];
        [nameLabel setBezeled:NO];
        [nameLabel setDrawsBackground:NO];
        [nameLabel setEditable:NO];
        [nameLabel setSelectable:NO];
        [nameLabel setTextColor:[NSColor whiteColor]];
        [content addSubview:nameLabel];
        
        y -= 30;
        self.nameField = [[NSTextField alloc] initWithFrame:NSMakeRect(20, y, 360, 24)];
        [self.nameField setBezelStyle:NSTextFieldRoundedBezel];
        [content addSubview:self.nameField];
        
        y -= 50;
        NSTextField* hostLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, y, 150, 20)];
        [hostLabel setStringValue:@"Host / IP Address:"];
        [hostLabel setBezeled:NO];
        [hostLabel setDrawsBackground:NO];
        [hostLabel setEditable:NO];
        [hostLabel setSelectable:NO];
        [hostLabel setTextColor:[NSColor whiteColor]];
        [content addSubview:hostLabel];
        
        y -= 30;
        self.hostField = [[NSTextField alloc] initWithFrame:NSMakeRect(20, y, 360, 24)];
        [self.hostField setBezelStyle:NSTextFieldRoundedBezel];
        [content addSubview:self.hostField];
        
        y -= 50;
        NSTextField* portLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, y, 50, 20)];
        [portLabel setStringValue:@"Port:"];
        [portLabel setBezeled:NO];
        [portLabel setDrawsBackground:NO];
        [portLabel setEditable:NO];
        [portLabel setSelectable:NO];
        [portLabel setTextColor:[NSColor whiteColor]];
        [content addSubview:portLabel];
        
        self.portField = [[NSTextField alloc] initWithFrame:NSMakeRect(70, y, 60, 24)];
        [self.portField setStringValue:@"443"];
        [self.portField setBezelStyle:NSTextFieldRoundedBezel];
        [content addSubview:self.portField];
        
        y -= 50;
        NSTextField* usernameLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, y, 100, 20)];
        [usernameLabel setStringValue:@"Username:"];
        [usernameLabel setBezeled:NO];
        [usernameLabel setDrawsBackground:NO];
        [usernameLabel setEditable:NO];
        [usernameLabel setSelectable:NO];
        [usernameLabel setTextColor:[NSColor whiteColor]];
        [content addSubview:usernameLabel];
        
        y -= 30;
        self.userField = [[NSTextField alloc] initWithFrame:NSMakeRect(20, y, 360, 24)];
        [self.userField setBezelStyle:NSTextFieldRoundedBezel];
        [content addSubview:self.userField];
        
        y -= 25;
        NSTextField* userHint = [[NSTextField alloc] initWithFrame:NSMakeRect(20, y, 360, 16)];
        [userHint setStringValue:@"Oracle: 'opc' | Google: your-username (check VM)"];
        [userHint setBezeled:NO];
        [userHint setDrawsBackground:NO];
        [userHint setEditable:NO];
        [userHint setSelectable:NO];
        [userHint setFont:[NSFont systemFontOfSize:11]];
        [userHint setTextColor:[NSColor lightGrayColor]];
        [content addSubview:userHint];
        
        y -= 40;
        NSTextField* keyLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, y, 180, 20)];
        [keyLabel setStringValue:@"SSH Private Key (file path):"];
        [keyLabel setBezeled:NO];
        [keyLabel setDrawsBackground:NO];
        [keyLabel setEditable:NO];
        [keyLabel setSelectable:NO];
        [keyLabel setTextColor:[NSColor whiteColor]];
        [content addSubview:keyLabel];
        
        y -= 30;
        self.keyField = [[NSTextField alloc] initWithFrame:NSMakeRect(20, y, 280, 24)];
        [self.keyField setBezelStyle:NSTextFieldRoundedBezel];
        [content addSubview:self.keyField];
        
        self.browseButton = [[NSButton alloc] initWithFrame:NSMakeRect(310, y, 70, 24)];
        [self.browseButton setTitle:@"Browse"];
        [self.browseButton setBezelStyle:NSBezelStyleRounded];
        [self.browseButton setTarget:self];
        [self.browseButton setAction:@selector(browseClicked:)];
        [content addSubview:self.browseButton];
        
        y -= 25;
        NSTextField* keyHint = [[NSTextField alloc] initWithFrame:NSMakeRect(20, y, 360, 16)];
        [keyHint setStringValue:@"e.g. /Users/You/.ssh/helloworld_key"];
        [keyHint setBezeled:NO];
        [keyHint setDrawsBackground:NO];
        [keyHint setEditable:NO];
        [keyHint setSelectable:NO];
        [keyHint setFont:[NSFont systemFontOfSize:11]];
        [keyHint setTextColor:[NSColor lightGrayColor]];
        [content addSubview:keyHint];
        
        y -= 50;
        self.addButton = [[NSButton alloc] initWithFrame:NSMakeRect(100, y, 90, 30)];
        [self.addButton setTitle:@"Add Server"];
        [self.addButton setBezelStyle:NSBezelStyleRounded];
        [self.addButton setTarget:self];
        [self.addButton setAction:@selector(addClicked:)];
        [self.addButton setKeyEquivalent:@"\r"];
        [content addSubview:self.addButton];
        
        self.cancelButton = [[NSButton alloc] initWithFrame:NSMakeRect(210, y, 60, 30)];
        [self.cancelButton setTitle:@"Cancel"];
        [self.cancelButton setBezelStyle:NSBezelStyleRounded];
        [self.cancelButton setTarget:self];
        [self.cancelButton setAction:@selector(cancelClicked:)];
        [self.cancelButton setKeyEquivalent:@"\e"];
        [content addSubview:self.cancelButton];
    }
    return self;
}

- (void)browseClicked:(id)sender {
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    [panel setCanChooseFiles:YES];
    [panel setCanChooseDirectories:NO];
    [panel setAllowsMultipleSelection:NO];
    [panel setAllowedFileTypes:@[@"key", @"pem", @"pub"]];
    
    if ([panel runModal] == NSModalResponseOK) {
        NSURL* url = [[panel URLs] firstObject];
        [self.keyField setStringValue:[url path]];
    }
}

- (void)addClicked:(id)sender {
    NSString* name = [self.nameField stringValue];
    NSString* host = [self.hostField stringValue];
    NSString* portStr = [self.portField stringValue];
    NSString* user = [self.userField stringValue];
    NSString* key = [self.keyField stringValue];
    
    if ([name length] == 0 || [host length] == 0) {
        NSAlert* alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Missing Info"];
        [alert setInformativeText:@"Please enter Name and Host."];
        [alert addButtonWithTitle:@"OK"];
        [alert setAlertStyle:NSWarningAlertStyle];
        [alert runModal];
        return;
    }
    
    if ([user length] == 0) {
        NSAlert* alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Username Required"];
        [alert setInformativeText:@"Please enter your username.\n\nOracle Cloud: 'opc'\nGoogle Cloud: Your Google account username\n\nTo find it: SSH into your VM and run 'whoami'"];
        [alert addButtonWithTitle:@"OK"];
        [alert setAlertStyle:NSWarningAlertStyle];
        [alert runModal];
        return;
    }
    
    if ([key length] == 0) {
        NSAlert* alert = [[NSAlert alloc] init];
        [alert setMessageText:@"SSH Key Required"];
        [alert setInformativeText:@"Please select your SSH private key file.\n\nClick Browse and navigate to your .ssh folder.\nSelect the private key (NOT the .pub file)."];
        [alert addButtonWithTitle:@"OK"];
        [alert setAlertStyle:NSWarningAlertStyle];
        [alert runModal];
        return;
    }
    
    if (![[NSFileManager defaultManager] fileExistsAtPath:key]) {
        NSAlert* alert = [[NSAlert alloc] init];
        [alert setMessageText:@"File Not Found"];
        [alert setInformativeText:[NSString stringWithFormat:@"SSH key file not found!\n\nMake sure the path is correct.\nExample: /Users/You/.ssh/helloworld_key"]];
        [alert addButtonWithTitle:@"OK"];
        [alert setAlertStyle:NSCriticalAlertStyle];
        [alert runModal];
        return;
    }
    
    int port = [portStr intValue];
    if (port <= 0) port = 443;
    
    hw_add_server(g_ctx, [name UTF8String], [host UTF8String], port, [key UTF8String], [user UTF8String]);
    hw_save_config(g_ctx);
    
    [self close];
}

- (void)cancelClicked:(id)sender {
    [self close];
}

@end

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSWindow* window;
@property (strong) NSButton* connectButton;
@property (strong) NSButton* addButton;
@property (strong) NSPopUpButton* serverPopup;
@property (strong) NSButton* fullModeCheck;
@property (strong) NSButton* killSwitchCheck;
@property (strong) NSTextField* statusLabel;
@property (strong) NSTextField* modeLabel;
@property (strong) NSTextField* ipLabel;
@property (strong) NSTextField* serverLabel;
@property (strong) NSTextField* timeLabel;
@property (strong) NSTimer* updateTimer;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    NSRect frame = NSMakeRect(0, 0, 360, 580);
    NSUInteger style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | 
                       NSWindowStyleMaskMiniaturizable;
    
    self.window = [[NSWindow alloc] initWithContentRect:frame
                                              styleMask:style
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    [self.window setTitle:@"HelloWorld"];
    [self.window center];
    [self.window setMinSize:NSMakeSize(360, 580)];
    
    NSView* content = [self.window contentView];
    [content setWantsLayer:YES];
    [[content layer] setBackgroundColor:[[NSColor colorWithRed:0.04 green:0.04 blue:0.05 alpha:1.0] CGColor]];
    
    int y = 540;
    
    NSTextField* title = [[NSTextField alloc] initWithFrame:NSMakeRect(20, y, 320, 44)];
    [title setStringValue:@"HelloWorld"];
    [title setBezeled:NO];
    [title setDrawsBackground:NO];
    [title setEditable:NO];
    [title setSelectable:NO];
    [title setAlignment:NSTextAlignmentCenter];
    [title setFont:[NSFont boldSystemFontOfSize:28]];
    [title setTextColor:[NSColor colorWithRed:0.0 green:1.0 blue:0.53 alpha:1.0]];
    [content addSubview:title];
    
    y -= 60;
    NSTextField* serverLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, y, 100, 16)];
    [serverLabel setStringValue:@"SERVER"];
    [serverLabel setBezeled:NO];
    [serverLabel setDrawsBackground:NO];
    [serverLabel setEditable:NO];
    [serverLabel setSelectable:NO];
    [serverLabel setFont:[NSFont systemFontOfSize:11]];
    [serverLabel setTextColor:[NSColor lightGrayColor]];
    [content addSubview:serverLabel];
    
    y -= 22;
    self.serverPopup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(20, y, 250, 40) pullsDown:NO];
    [self updateServerList];
    [content addSubview:self.serverPopup];
    
    self.addButton = [[NSButton alloc] initWithFrame:NSMakeRect(280, y, 60, 40)];
    [self.addButton setTitle:@"+ ADD"];
    [self.addButton setBezelStyle:NSBezelStyleRounded];
    [self.addButton setTarget:self];
    [self.addButton setAction:@selector(addServerClicked:)];
    [content addSubview:self.addButton];
    
    y -= 60;
    self.connectButton = [[NSButton alloc] initWithFrame:NSMakeRect(20, y, 320, 48)];
    [self.connectButton setTitle:@"CONNECT"];
    [self.connectButton setBezelStyle:NSBezelStyleRounded];
    [self.connectButton setTarget:self];
    [self.connectButton setAction:@selector(connectClicked:)];
    [self.connectButton setFont:[NSFont boldSystemFontOfSize:16]];
    [[self.connectButton cell] setBackgroundColor:[NSColor colorWithRed:0.0 green:1.0 blue:0.53 alpha:1.0]];
    [content addSubview:self.connectButton];
    
    y -= 70;
    self.fullModeCheck = [[NSButton alloc] initWithFrame:NSMakeRect(20, y, 320, 36)];
    [self.fullModeCheck setButtonType:NSButtonTypeSwitch];
    [self.fullModeCheck setTitle:@"Full Tunnel Mode"];
    [[self.fullModeCheck cell] setTextColor:[NSColor whiteColor]];
    if (g_ctx->mode == HW_MODE_FULL_TUNNEL) {
        [self.fullModeCheck setState:NSControlStateValueOn];
    }
    [content addSubview:self.fullModeCheck];
    
    y -= 50;
    self.killSwitchCheck = [[NSButton alloc] initWithFrame:NSMakeRect(20, y, 320, 36)];
    [self.killSwitchCheck setButtonType:NSButtonTypeSwitch];
    [self.killSwitchCheck setTitle:@"Kill Switch"];
    [[self.killSwitchCheck cell] setTextColor:[NSColor whiteColor]];
    if (g_ctx->kill_switch) {
        [self.killSwitchCheck setState:NSControlStateValueOn];
    }
    [content addSubview:self.killSwitchCheck];
    
    y -= 60;
    NSView* infoBox = [[NSView alloc] initWithFrame:NSMakeRect(20, 20, 320, y - 20)];
    [infoBox setWantsLayer:YES];
    [[infoBox layer] setBackgroundColor:[[NSColor colorWithRed:0.1 green:0.1 blue:0.14 alpha:1.0] CGColor]];
    [[infoBox layer] setCornerRadius:12];
    [[infoBox layer] setBorderWidth:1];
    [[infoBox layer] setBorderColor:[[NSColor colorWithRed:0.16 green:0.16 blue:0.22 alpha:1.0] CGColor]];
    [content addSubview:infoBox];
    
    int infoY = y - 30;
    NSTextField* statusLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(16, infoY, 288, 16)];
    [statusLabel setStringValue:@"STATUS"];
    [statusLabel setBezeled:NO];
    [statusLabel setDrawsBackground:NO];
    [statusLabel setEditable:NO];
    [statusLabel setSelectable:NO];
    [statusLabel setFont:[NSFont systemFontOfSize:11]];
    [statusLabel setTextColor:[NSColor lightGrayColor]];
    [infoBox addSubview:statusLabel];
    
    infoY -= 22;
    self.statusLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(16, infoY, 288, 22)];
    [self.statusLabel setStringValue:@"Disconnected"];
    [self.statusLabel setBezeled:NO];
    [self.statusLabel setDrawsBackground:NO];
    [self.statusLabel setEditable:NO];
    [self.statusLabel setSelectable:NO];
    [self.statusLabel setTextColor:[NSColor lightGrayColor]];
    [infoBox addSubview:self.statusLabel];
    
    infoY -= 40;
    NSTextField* modeLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(16, infoY, 288, 16)];
    [modeLabel setStringValue:@"MODE"];
    [modeLabel setBezeled:NO];
    [modeLabel setDrawsBackground:NO];
    [modeLabel setEditable:NO];
    [modeLabel setSelectable:NO];
    [modeLabel setFont:[NSFont systemFontOfSize:11]];
    [modeLabel setTextColor:[NSColor lightGrayColor]];
    [infoBox addSubview:modeLabel];
    
    infoY -= 22;
    self.modeLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(16, infoY, 288, 22)];
    [self.modeLabel setStringValue:@"Proxy Mode (SOCKS5)"];
    [self.modeLabel setBezeled:NO];
    [self.modeLabel setDrawsBackground:NO];
    [self.modeLabel setEditable:NO];
    [self.modeLabel setSelectable:NO];
    [self.modeLabel setTextColor:[NSColor whiteColor]];
    [infoBox addSubview:self.modeLabel];
    
    infoY -= 40;
    NSTextField* ipLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(16, infoY, 288, 16)];
    [ipLabel setStringValue:@"YOUR IP"];
    [ipLabel setBezeled:NO];
    [ipLabel setDrawsBackground:NO];
    [ipLabel setEditable:NO];
    [ipLabel setSelectable:NO];
    [ipLabel setFont:[NSFont systemFontOfSize:11]];
    [ipLabel setTextColor:[NSColor lightGrayColor]];
    [infoBox addSubview:ipLabel];
    
    infoY -= 22;
    self.ipLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(16, infoY, 288, 22)];
    [self.ipLabel setStringValue:@"--"];
    [self.ipLabel setBezeled:NO];
    [self.ipLabel setDrawsBackground:NO];
    [self.ipLabel setEditable:NO];
    [self.ipLabel setSelectable:NO];
    [self.ipLabel setFont:[NSFont boldSystemFontOfSize:14]];
    [self.ipLabel setTextColor:[NSColor colorWithRed:0.0 green:1.0 blue:0.53 alpha:1.0]];
    [infoBox addSubview:self.ipLabel];
    
    infoY -= 40;
    NSTextField* serverLabel2 = [[NSTextField alloc] initWithFrame:NSMakeRect(16, infoY, 288, 16)];
    [serverLabel2 setStringValue:@"SERVER"];
    [serverLabel2 setBezeled:NO];
    [serverLabel2 setDrawsBackground:NO];
    [serverLabel2 setEditable:NO];
    [serverLabel2 setSelectable:NO];
    [serverLabel2 setFont:[NSFont systemFontOfSize:11]];
    [serverLabel2 setTextColor:[NSColor lightGrayColor]];
    [infoBox addSubview:serverLabel2];
    
    infoY -= 22;
    self.serverLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(16, infoY, 288, 22)];
    [self.serverLabel setStringValue:@"--"];
    [self.serverLabel setBezeled:NO];
    [self.serverLabel setDrawsBackground:NO];
    [self.serverLabel setEditable:NO];
    [self.serverLabel setSelectable:NO];
    [self.serverLabel setTextColor:[NSColor whiteColor]];
    [infoBox addSubview:self.serverLabel];
    
    infoY -= 40;
    NSTextField* timeLabel2 = [[NSTextField alloc] initWithFrame:NSMakeRect(16, infoY, 288, 16)];
    [timeLabel2 setStringValue:@"DURATION"];
    [timeLabel2 setBezeled:NO];
    [timeLabel2 setDrawsBackground:NO];
    [timeLabel2 setEditable:NO];
    [timeLabel2 setSelectable:NO];
    [timeLabel2 setFont:[NSFont systemFontOfSize:11]];
    [timeLabel2 setTextColor:[NSColor lightGrayColor]];
    [infoBox addSubview:timeLabel2];
    
    infoY -= 22;
    self.timeLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(16, infoY, 288, 22)];
    [self.timeLabel setStringValue:@"--:--:--"];
    [self.timeLabel setBezeled:NO];
    [self.timeLabel setDrawsBackground:NO];
    [self.timeLabel setEditable:NO];
    [self.timeLabel setSelectable:NO];
    [self.timeLabel setTextColor:[NSColor whiteColor]];
    [infoBox addSubview:self.timeLabel];
    
    hw_load_config(g_ctx);
    [self updateServerList];
    
    self.updateTimer = [NSTimer scheduledTimerWithTimeInterval:1.0
                                                        target:self
                                                      selector:@selector(updateUI)
                                                      userInfo:nil
                                                       repeats:YES];
    
    [self.window makeKeyAndOrderFront:nil];
}

- (void)updateServerList {
    [self.serverPopup removeAllItems];
    if (g_ctx->server_count == 0) {
        [self.serverPopup addItemWithTitle:@"No servers"];
    } else {
        for (int i = 0; i < g_ctx->server_count; i++) {
            [self.serverPopup addItemWithTitle:[NSString stringWithUTF8String:g_ctx->servers[i].name]];
        }
        if (g_ctx->current_server >= 0 && g_ctx->current_server < g_ctx->server_count) {
            [self.serverPopup selectItemAtIndex:g_ctx->current_server];
        }
    }
}

- (void)addServerClicked:(id)sender {
    AddServerWindow* addWindow = [[AddServerWindow alloc] init];
    [addWindow showWindow:nil];
    [NSApp runModalForWindow:[addWindow window]];
    [self updateServerList];
}

- (void)connectClicked:(id)sender {
    if (g_ctx->status == HW_CONNECTED) {
        hw_disconnect(g_ctx);
    } else if (g_ctx->status == HW_DISCONNECTED) {
        if (g_ctx->server_count == 0) {
            NSAlert* alert = [[NSAlert alloc] init];
            [alert setMessageText:@"No Server"];
            [alert setInformativeText:@"Please add a server first.\n\nClick the + ADD button to add your server."];
            [alert addButtonWithTitle:@"OK"];
            [alert setAlertStyle:NSInformationalAlertStyle];
            [alert runModal];
            return;
        }
        
        NSInteger sel = [self.serverPopup indexOfSelectedItem];
        if (sel >= 0) {
            g_ctx->current_server = (int)sel;
            g_ctx->mode = ([self.fullModeCheck state] == NSControlStateValueOn) 
                          ? HW_MODE_FULL_TUNNEL : HW_MODE_PROXY;
            g_ctx->kill_switch = ([self.killSwitchCheck state] == NSControlStateValueOn);
            
            int result = hw_connect(g_ctx);
            if (result != 0) {
                NSString* errorDetail = [NSString stringWithUTF8String:g_ctx->error_msg[0] ? g_ctx->error_msg : "Unknown error"];
                NSString* errMsg = @"";
                
                if ([errorDetail rangeOfString:@"stunnel"].location != NSNotFound) {
                    errMsg = [NSString stringWithFormat:@"STUNNEL NOT INSTALLED!\n\nHelloWorld requires stunnel to work.\n\nTo install stunnel:\n1. Open Terminal\n2. Run: brew install stunnel\n   OR download from: https://www.stunnel.org/downloads.html\n3. Restart HelloWorld after installing\n\nOriginal error: %@", errorDetail];
                } else if ([errorDetail rangeOfString:@"SSH"].location != NSNotFound || [errorDetail rangeOfString:@"ssh"].location != NSNotFound) {
                    errMsg = [NSString stringWithFormat:@"SSH CONNECTION FAILED!\n\nError: %@\n\nTroubleshooting:\n1. Verify server IP is correct\n2. Check username (Oracle: 'opc', Google: 'your-username')\n3. Verify SSH key file path is correct\n4. Make sure key is authorized on server\n5. Test connection: ssh -i \"key\" user@server-ip\n6. Check server status: ssh into server and run 'helloworld-status'", errorDetail];
                } else {
                    errMsg = [NSString stringWithFormat:@"CONNECTION FAILED!\n\nError: %@\n\nStep-by-step checklist:\n1. Server running? SSH to server and run: helloworld-status\n2. Port 443 open? Check firewall rules on cloud provider\n3. SSH key correct? Verify file path and permissions\n4. Username correct? Oracle: 'opc', Google: check your VM user\n5. Server IP correct? Get it from cloud console\n6. Stunnel installed? Run: brew install stunnel\n\nFor detailed setup: https://github.com/dokabi-recon67/helloworld", errorDetail];
                }
                
                NSAlert* alert = [[NSAlert alloc] init];
                [alert setMessageText:@"Connection Failed"];
                [alert setInformativeText:errMsg];
                [alert addButtonWithTitle:@"OK"];
                [alert setAlertStyle:NSCriticalAlertStyle];
                [alert runModal];
            }
        }
    }
    [self updateUI];
}

- (void)updateUI {
    const char* statusStr = hw_status_str(g_ctx->status);
    [self.statusLabel setStringValue:[NSString stringWithUTF8String:statusStr]];
    
    if (g_ctx->status == HW_CONNECTED) {
        [self.statusLabel setTextColor:[NSColor colorWithRed:0.0 green:1.0 blue:0.53 alpha:1.0]];
        
        const char* modeStr = g_ctx->mode == HW_MODE_FULL_TUNNEL ? "Full Tunnel (All Traffic)" : "Proxy Mode (SOCKS5)";
        [self.modeLabel setStringValue:[NSString stringWithUTF8String:modeStr]];
        
        if (g_ctx->stats.public_ip[0]) {
            [self.ipLabel setStringValue:[NSString stringWithUTF8String:g_ctx->stats.public_ip]];
        } else {
            [self.ipLabel setStringValue:@"Fetching..."];
        }
        
        if (g_ctx->current_server >= 0 && g_ctx->current_server < g_ctx->server_count) {
            [self.serverLabel setStringValue:[NSString stringWithFormat:@"%s (%s:%d)",
                g_ctx->servers[g_ctx->current_server].name,
                g_ctx->servers[g_ctx->current_server].host,
                g_ctx->servers[g_ctx->current_server].port]];
        }
        
        time_t elapsed = time(NULL) - g_ctx->stats.start_time;
        int h = (int)(elapsed / 3600);
        int m = (int)((elapsed % 3600) / 60);
        int s = (int)(elapsed % 60);
        [self.timeLabel setStringValue:[NSString stringWithFormat:@"%02d:%02d:%02d", h, m, s]];
        
        [self.connectButton setTitle:@"DISCONNECT"];
        [[self.connectButton cell] setBackgroundColor:[NSColor colorWithRed:0.9 green:0.2 blue:0.2 alpha:1.0]];
    } else if (g_ctx->status == HW_CONNECTING) {
        [self.statusLabel setTextColor:[NSColor whiteColor]];
        [self.connectButton setTitle:@"CONNECTING..."];
        [self.connectButton setEnabled:NO];
    } else {
        [self.statusLabel setTextColor:[NSColor lightGrayColor]];
        [self.modeLabel setStringValue:@"--"];
        [self.ipLabel setStringValue:@"--"];
        [self.serverLabel setStringValue:@"--"];
        [self.timeLabel setStringValue:@"--:--:--"];
        [self.connectButton setTitle:@"CONNECT"];
        [self.connectButton setEnabled:YES];
        [[self.connectButton cell] setBackgroundColor:[NSColor colorWithRed:0.0 green:1.0 blue:0.53 alpha:1.0]];
    }
    
    [self.serverPopup setEnabled:(g_ctx->status == HW_DISCONNECTED)];
    [self.fullModeCheck setEnabled:(g_ctx->status == HW_DISCONNECTED)];
    [self.killSwitchCheck setEnabled:(g_ctx->status == HW_DISCONNECTED)];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    return YES;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender {
    if (g_ctx && g_ctx->status == HW_CONNECTED) {
        NSAlert* alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Still Connected"];
        [alert setInformativeText:@"Disconnect?"];
        [alert addButtonWithTitle:@"Disconnect"];
        [alert addButtonWithTitle:@"Cancel"];
        [alert setAlertStyle:NSWarningAlertStyle];
        
        if ([alert runModal] == NSAlertFirstButtonReturn) {
            hw_disconnect(g_ctx);
            return NSTerminateNow;
        }
        return NSTerminateCancel;
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
