#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "platform.h"

AppModule app = {0};

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>

// Input related
@property bool inputUsed;
@property AppKeyCode keyCode;
@property bool keyDown;
@property bool mouseLeftDown;
@property bool mouseRightDown;
@property bool mouseMouseMoved;

// Image related
@property uint8_t *buffer;
@property size_t bufferSize;
@property double lastFrameTime;

@property(nonatomic, strong) NSWindow *window;
@property(nonatomic, strong) NSImage *image;
@property(nonatomic, strong) NSBitmapImageRep *imageRep;

- (void)updateFrame;
- (void)handleInput:(NSEvent*)event;
@end

@implementation AppDelegate
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    int windowStyleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                          NSWindowStyleMaskResizable | NSWindowStyleMaskFullSizeContentView
                          | NSWindowStyleMaskMiniaturizable;

    self.window = [[NSWindow alloc]
            initWithContentRect:NSMakeRect(0, 0, INIT_WIDTH, INIT_HEIGHT)
                      styleMask:windowStyleMask
                        backing:NSBackingStoreBuffered
                          defer:NO];

    [self.window setTitle:@WINDOW_NAME];
    [self.window setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
    [self.window setDelegate:self];
    [self.window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];

    self.lastFrameTime = platform_get_time();
    self.buffer = NULL;

    app.app_init();

    [NSTimer scheduledTimerWithTimeInterval:1.0 / 60.0
                                     target:self
                                   selector:@selector(updateFrame)
                                   userInfo:nil
                                    repeats:YES];

    int inputEvents = NSEventMaskKeyDown|NSEventMaskMouseMoved|NSEventTypeLeftMouseDown|NSEventTypeRightMouseDown;
    [NSEvent addLocalMonitorForEventsMatchingMask:inputEvents handler:^NSEvent*(NSEvent *event){
        [self handleInput:event];
        return event;
    }];
}

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)frameSize {
    frameSize.width = MAX(frameSize.width, MIN_WIDTH);
    frameSize.height = MAX(frameSize.height, MIN_HEIGHT);
    return frameSize;
}

- (void)windowWillClose:(NSNotification *)notification {
    [self.window release];
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
                    .key_code = self.keyCode,
                    .key_down = self.keyDown,
                    .mouse_left_down = self.mouseLeftDown,
                    .mouse_right_down = self.mouseRightDown,
                    .mouse_moved = self.mouseMouseMoved,
            };
            app.app_update(&env);

           //NSLog(@"KeyDown = %d", env.key_down);
        }
        self.inputUsed = true;

        // Create a new NSBitmapImageRep with the updated buffer
        uint32_t pitch = width * sizeof(uint32_t);
        uint8_t *buffer = self.buffer;

        self.imageRep =
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

        // Releasing old images
        [self.image release];
        [self.imageRep release];

        // Update the NSImage with the new representation
        self.image = [[NSImage alloc] initWithSize:NSMakeSize(width, height)];
        [self.image addRepresentation:self.imageRep];

        // Update the layer contents with the new image
        self.window.contentView.layer.contents = self.image;
        self.lastFrameTime = currentFrameTime;

        [self resetInput];
}

- (void)handleInput:(NSEvent*)event {
    self.keyDown = event.type == NSEventTypeKeyDown;
    self.mouseLeftDown = event.type & NSEventTypeLeftMouseDown;
    self.mouseRightDown = event.type & NSEventTypeRightMouseDown;
    self.mouseMouseMoved = event.type & NSEventTypeMouseMoved;
    self.inputUsed = false;

    if(self.keyDown) {
        if (event.keyCode == 96) {
            NSLog(@"Hot reloading...");

            void* oldState = module.app_pre_reload();
            platform_load_module(&module, "./everything.dylib");
            module.app_post_reload(oldState);
        }

        self.keyCode = event.keyCode;
    }
}

- (void)resetInput {
    if(!self.inputUsed) return;

    self.keyDown = false;
    self.mouseLeftDown = false;
    self.mouseRightDown = false;
    self.mouseMouseMoved = false;
}

@end

int main(void) {
    platform_load_module(&module, "./everything.dylib");
    
    @autoreleasepool {
        NSApplication *application = [NSApplication sharedApplication];
        AppDelegate *appDelegate = [[AppDelegate alloc] init];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        [application setDelegate:appDelegate];
        [application run];
    }

    return EXIT_SUCCESS;
}

