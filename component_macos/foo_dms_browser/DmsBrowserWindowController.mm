#import "DmsBrowserWindowController.h"

#import "DmsBrowserViewController.h"

@implementation DmsBrowserWindowController

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
    });
    [shared showWindow:nil];
    [shared.window makeKeyAndOrderFront:nil];
}

@end
