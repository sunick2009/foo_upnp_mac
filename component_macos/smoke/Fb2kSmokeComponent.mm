#import <Cocoa/Cocoa.h>

// This target deliberately avoids the foobar2000 SDK. It only proves that the
// local CLT toolchain can compile ObjC++ and link a loadable Cocoa bundle.
extern "C" bool foo_dms_browser_macos_smoke_can_load(void) {
    @autoreleasepool {
        Class bundleClass = [NSBundle class];
        Class appClass = [NSApplication class];
        return bundleClass != Nil && appClass != Nil;
    }
}
