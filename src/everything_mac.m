#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "config.h"
#include "hotreload.h"

Env env = {0};
AppModule module = {0};

double getTime(void)
{
	@autoreleasepool
	{
		NSDate *now = [NSDate date];
		NSTimeInterval timeInterval = [now timeIntervalSince1970];
		double currentTimeInMilliseconds = (timeInterval * 1000.0);
		return currentTimeInMilliseconds;
	}
}

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>

@property bool inputUsed;
@property double lastFrameTime;

@property(nonatomic, strong) NSWindow *window;
@property(nonatomic, strong) NSImage *image;
@property(nonatomic, strong) NSBitmapImageRep *imageRep;

- (void)updateFrame;
- (void)handleInput:(NSEvent *)event;
@end

@implementation AppDelegate
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
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

	self.lastFrameTime = getTime();

	[NSTimer scheduledTimerWithTimeInterval:1.0 / 60.0
	 target:self
	 selector:@selector(updateFrame)
	 userInfo:nil
	 repeats:YES];

	int inputEvents = NSEventMaskKeyDown | NSEventMaskMouseMoved | NSEventMaskLeftMouseDown | NSEventMaskRightMouseDown;
	[NSEvent addLocalMonitorForEventsMatchingMask:inputEvents handler:^NSEvent *(NSEvent *event)
	{
		[self handleInput:event];
		return event;
	}];
}

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)frameSize
{
	frameSize.width = MAX(frameSize.width, MIN_WIDTH);
	frameSize.height = MAX(frameSize.height, MIN_HEIGHT);
	return frameSize;
}

- (void)windowWillClose:(NSNotification *)notification
{
	[self.window release];
	free(env.buffer);
	exit(EXIT_SUCCESS);
}

- (void)updateFrame
{
	double currentFrameTime = getTime();
	double dt = (currentFrameTime - self.lastFrameTime) / 1000.0;
	env.delta_time = dt;

	// Obtain the dimensions of the window's content view
	int width = (int) self.window.contentView.bounds.size.width;
	int height = (int) self.window.contentView.bounds.size.height;

	// Allocate a new buffer or reuse the existing one

	size_t newBufferSize = width * height * sizeof(uint32_t);
	size_t oldBufferSize = env.width * env.height * sizeof(uint32_t);
	if (!env.buffer || (env.buffer && oldBufferSize != newBufferSize))
	{
		if (env.buffer)
			free(env.buffer);

		env.buffer = malloc(newBufferSize);
		env.width = width;
		env.height = height;
	}

	// Updating the actual app
	module.app_update(&env);
	self.inputUsed = true;

	// Create a new NSBitmapImageRep with the updated buffer
	uint32_t pitch = width * sizeof(uint32_t);
	uint8_t *buffer = env.buffer;

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

- (void)handleInput:(NSEvent *)event
{
	env.key_down = event.type == NSEventTypeKeyDown;
	env.mouse_left_down = event.type == NSEventTypeLeftMouseDown;
	env.mouse_right_down = event.type == NSEventTypeRightMouseDown;
	env.mouse_moved = event.type == NSEventTypeMouseMoved;

	if (env.key_down)
	{
		if (event.keyCode == 96)
		{
			AppStateHandle handle = module.app_pre_reload();
			load_module(&module, "./everything.dylib");
			module.app_post_reload(handle);
		}

		env.key_code = event.keyCode;
	}

	if (env.mouse_left_down || env.mouse_right_down || env.mouse_moved)
	{
		NSPoint mouseLoc = [event locationInWindow];
		double screenHeight = self.window.contentView.bounds.size.height;
		mouseLoc.y = screenHeight - mouseLoc.y;

		env.mouse_x = (int) (mouseLoc.x);
		env.mouse_y = (int) (mouseLoc.y);
	}

	self.inputUsed = false;
}

- (void)resetInput
{
	if (!self.inputUsed) return;

	env.key_down = false;
	env.mouse_left_down = false;
	env.mouse_right_down = false;
	env.mouse_moved = false;
}

@end

int main(void)
{
	load_module(&module, "./everything.dylib");
	module.app_init();

	@autoreleasepool
	{
		NSApplication *application = [NSApplication sharedApplication];
		AppDelegate *appDelegate = [[AppDelegate alloc] init];
		[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

		[application setDelegate:appDelegate];
		[application run];
	}

	return EXIT_SUCCESS;
}

