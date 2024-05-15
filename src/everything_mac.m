#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "everything.h"

App app = {0};
void *appModuleHandle = NULL;

void loadAppModule(void) {
    if (!appModuleHandle) dlclose(appModuleHandle);

    appModuleHandle = dlopen("./everything.so", RTLD_NOW);
    if (!appModuleHandle) {
        fprintf(stderr, "ERROR: Error occurred during module loading: %s\n", dlerror());
        exit(EXIT_FAILURE);
    }

    app.app_init = dlsym(appModuleHandle, "app_init");
    if (!app.app_init) {
        fprintf(stderr, "ERROR: Error occurred during module symbol: %s\n", dlerror());
        exit(EXIT_FAILURE);
    }

    app.app_update = dlsym(appModuleHandle, "app_update");
    if (!app.app_update) {
        fprintf(stderr, "ERROR: Error occurred during module symbol: %s\n", dlerror());
        exit(EXIT_FAILURE);
    }
}


@interface  AppWindow:  NSWindow
@end

@implementation  AppWindow
- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)keyDown:(NSEvent *)event {
    NSString *characters = [event characters];
    NSLog(@"Key pressed: %@", characters);
}
@end

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>

@property uint8_t *buffer;
@property size_t bufferSize;
@property(nonatomic, strong) AppWindow *window;
@property double lastFrameTime;

- (void)updateFrame;
@end

@implementation AppDelegate
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    int windowStyleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                          NSWindowStyleMaskResizable | NSWindowStyleMaskFullSizeContentView
                          | NSWindowStyleMaskMiniaturizable;

    self.window = [[AppWindow alloc]
            initWithContentRect:NSMakeRect(0, 0, INIT_WIDTH, INIT_HEIGHT)
                      styleMask:windowStyleMask
                        backing:NSBackingStoreBuffered
                          defer:NO];

    [self.window setTitle:@WINDOW_NAME];
    [self.window setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
    [self.window makeKeyAndOrderFront:nil];
    [self.window makeFirstResponder:self.window];

    self.lastFrameTime = platform_get_time();
    self.buffer = NULL;

    app.app_init();

    [NSTimer scheduledTimerWithTimeInterval:1.0 / 60.0
                                     target:self
                                   selector:@selector(updateFrame)
                                   userInfo:nil
                                    repeats:YES];

    [NSEvent addGlobalMonitorForEventsMatchingMask:NSKeyDownMask handler:^(NSEvent *event){
        [self.window keyDown:event];
    }];
}

- (void)applicationDidBecomeActive:(NSNotification*)aNotification {
    NSLog(@"Is key window: %d", [self.window isKeyWindow]);
}

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)frameSize {
    frameSize.width = MAX(frameSize.width, MIN_WIDTH);
    frameSize.height = MAX(frameSize.height, MIN_HEIGHT);
    return frameSize;
}

- (void)windowWillClose:(NSNotification *)notification {
    free(self.buffer);
    exit(EXIT_SUCCESS);
}

- (void)updateFrame {
    double currentFrameTime = platform_get_time();
    double dt = (currentFrameTime - self.lastFrameTime) / 1000.0;

    // Obtain the dimensions of the window's content view
    int width = (int) self.window.contentView.bounds.size.width;
    int height = (int) self.window.contentView.bounds.size.height;

    // Allocate a new buffer or reuse the existing one

    size_t newBufferSize = width * height * sizeof(uint32_t);
    if (!self.buffer || (self.buffer && self.bufferSize != newBufferSize)) {
        if (self.buffer)
            free(self.buffer);

        self.buffer = malloc(newBufferSize);
        self.bufferSize = newBufferSize;
    }

    // Updating the actual app
    {
        Env env = {
                .buffer = self.buffer,
                .delta_time = dt,
                .window_width = width,
                .window_height = height,
        };
        app.app_update(&env);
    }

    // Create a new NSBitmapImageRep with the updated buffer
    uint32_t pitch = width * sizeof(uint32_t);
    uint8_t *buffer = self.buffer;

    NSBitmapImageRep *rep =
            [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:&buffer
                                                    pixelsWide:width
                                                    pixelsHigh:height
                                                 bitsPerSample:8
                                               samplesPerPixel:4
                                                      hasAlpha:YES
                                                      isPlanar:NO
                                                colorSpaceName:NSDeviceRGBColorSpace
                                                   bytesPerRow:pitch
                                                  bitsPerPixel:32];

    // Update the NSImage with the new representation
    NSImage *image = [[NSImage alloc] initWithSize:NSMakeSize(width, height)];
    [image addRepresentation:rep];

    // Update the layer contents with the new image
    self.window.contentView.layer.contents = image;
    self.lastFrameTime = currentFrameTime;
}

@end

int main(int argc, const char *argv[]) {
    (void) argc;
    (void) argv;

    loadAppModule();

    @autoreleasepool {
        NSApplication *application = [NSApplication sharedApplication];
        AppDelegate *appDelegate = [[AppDelegate alloc] init];

        [application setDelegate:appDelegate];
        [application run];
    }
    return 0;
}

/*  Platform functions */
double platform_get_time(void) {
    NSDate *now = [NSDate date];
    NSTimeInterval timeInterval = [now timeIntervalSince1970];
    double currentTimeInMilliseconds = (timeInterval * 1000.0);

    return currentTimeInMilliseconds;
}
