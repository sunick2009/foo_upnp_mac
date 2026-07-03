#pragma once

#import <Cocoa/Cocoa.h>

// The standalone browser window (ADR-014 MVP shell).
@interface DmsBrowserWindowController : NSWindowController

+ (void)showBrowser;

@end
