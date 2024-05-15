#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>

#include <stdint.h>
#include <stdlib.h>

#define WINDOW_NAME "Everything App"
#define MIN_WIDTH 300
#define MIN_HEIGHT 200
#define INIT_WIDTH 600
#define INIT_HEIGHT 400
#define BLOCK_SIZE 100

#define Red 0xFFFF0000       // Red
#define Green 0xFF00FF00     // Green
#define Blue 0xFF0000FF      // Blue
#define Yellow 0xFFFFFF00    // Yellow
#define Cyan 0xFF00FFFF      // Cyan
#define Magenta 0xFFFF00FF   // Magenta
#define Black 0xFF000000     // Black
#define White 0xFFFFFFFF     // White
#define Orange 0xFFFFA500    // Orange
#define Brown 0xFFA52A2A     // Brown
#define Gray 0xFF808080      // Gray
#define Purple 0xFF800080    // Purple
#define DarkGreen 0xFF008000 // Dark Green
#define NavyiBlue 0xFF000080 // Navy Blue
#define Olive 0xFF808000     // Olive
#define Maroon 0xFF800000    // Maroon
#define LightBlue 0xFFADD8E6 // Light Blue
#define Pink 0xFFFFC0CB      // Pink
#define Bronze 0xFFA52A2A    // Bronze
#define Burlywood 0xFFDEB887 // Burlywood

// Begin Utils

uint64_t getCurrentTime() {
  NSDate *now = [NSDate date];
  NSTimeInterval timeInterval = [now timeIntervalSince1970];
  uint64_t currentTimeInMilliseconds = (uint64_t)(timeInterval * 1000.0);

  return currentTimeInMilliseconds;
}

// End utils

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>

@property uint8_t *buffer;
@property size_t bufferSize;
@property(nonatomic, strong) NSWindow *window;

@property uint64_t lastFrameTime;
- (void)updateFrame;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
  int windowStyleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                        NSWindowStyleMaskResizable;

  self.window = [[NSWindow alloc]
      initWithContentRect:NSMakeRect(0, 0, INIT_WIDTH, INIT_HEIGHT)
                styleMask:windowStyleMask
                  backing:NSBackingStoreBuffered
                    defer:NO];

  [self.window setTitle:@WINDOW_NAME];
  [self.window setDelegate:self];

  [NSTimer scheduledTimerWithTimeInterval:1.0 / 60.0
                                   target:self
                                 selector:@selector(updateFrame)
                                 userInfo:nil
                                  repeats:YES];

  [self.window makeKeyAndOrderFront:nil];

  self.lastFrameTime = getCurrentTime();
  self.buffer = NULL;

  NSLog(@"Window initalized");
}

- (void)updateFrame {
  @autoreleasepool {
    uint64_t currentFrameTime = getCurrentTime();
    float dt = (currentFrameTime - self.lastFrameTime) / 1000.0;

    // Obtain the dimensions of the window's content view
    int width = self.window.contentView.bounds.size.width;
    int height = self.window.contentView.bounds.size.height;

    // Allocate a new buffer or reuse the existing one

    size_t newBufferSize = width * height * sizeof(uint32_t);
    if (!self.buffer || self.buffer && self.bufferSize != newBufferSize) {
      NSLog(@"New buffer allocated (%d, %d)", width, height);
      if (self.buffer)
        free(self.buffer);

      self.buffer = malloc(newBufferSize);
      self.bufferSize = newBufferSize;
    }

    // Update the buffer content here
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        uint32_t color;
        int sqaureX = x/BLOCK_SIZE;
        int sqaureY = y/BLOCK_SIZE;

        if ((sqaureX+sqaureY)%2 == 0) {
          color = Magenta;
        } else {
          color = White;
        }
      
        ((uint32_t *)self.buffer)[y * width + x] = color;
      }
    }

    // Create a new NSBitmapImageRep with the updated buffer
    size_t pitch = width * sizeof(uint32_t);
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
}

- (void)handleKeyDown:(NSEvent *)event {
  NSString *characters = [event characters];
  NSLog(@"Key pressed: %@", characters);
}

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)frameSize {
  frameSize.width = MAX(frameSize.width, MIN_WIDTH);
  frameSize.height = MAX(frameSize.height, MIN_HEIGHT);

  return frameSize;
}

- (void)windowWillClose:(NSNotification *)notification {
  [self.window release];
  exit(EXIT_SUCCESS);
}
@end

int main(int argc, const char *argv[]) {
  @autoreleasepool {
    NSApplication *application = [NSApplication sharedApplication];
    AppDelegate *appDelegate = [[AppDelegate alloc] init];

    [application setDelegate:appDelegate];
    [application run];
  }
  return 0;
}
