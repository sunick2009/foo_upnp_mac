#include "SDK/foobar2000.h"

#import "DmsBrowserWindowController.h"

#import "DmsBrowserViewController.h"

@implementation DmsBrowserWindowController

// Deliberate singleton: the window is created once, close only orders
// it out (releasedWhenClosed=NO), and reopen reuses the same
// controller — there is no per-open controller that could go stale.
// The layout-hosted ui_element instance is a separate
// DmsBrowserViewController and is unaffected by this window.
+ (void)showBrowser {
    static DmsBrowserWindowController* shared;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        NSWindow* window = [[NSWindow alloc]
            initWithContentRect:NSMakeRect(0, 0, 680, 460)
                      styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                                NSWindowStyleMaskResizable |
                                NSWindowStyleMaskMiniaturizable
                        backing:NSBackingStoreBuffered
                          defer:YES];
        window.title = @"DMS Browser";
        window.releasedWhenClosed = NO;
        window.contentViewController = [DmsBrowserViewController new];
        [window center];
        window.frameAutosaveName = @"DmsBrowserWindow";
        shared = [[DmsBrowserWindowController alloc] initWithWindow:window];
        // Lifecycle diagnostics for the intermittent reacquire issue
        // (issue #10): every transition leaves a console trace.
        FB2K_console_formatter() << "DMS Browser: standalone window created";
        [[NSNotificationCenter defaultCenter]
            addObserverForName:NSWindowWillCloseNotification
                        object:window
                         queue:[NSOperationQueue mainQueue]
                    usingBlock:^(NSNotification* note) {
                        (void)note;
                        FB2K_console_formatter()
                            << "DMS Browser: standalone window closed "
                               "(ordered out, controller retained)";
                    }];
    });
    FB2K_console_formatter() << "DMS Browser: standalone window shown";
    [shared showWindow:nil];
    [shared.window makeKeyAndOrderFront:nil];
}

@end
