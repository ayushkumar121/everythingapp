#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "config.h"
#include "hotreload.h"

Env env = { .scale = 1.0f };
AppModule module = {0};
bool app_initialised = false;
CGColorSpaceRef color_space = NULL;

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
@property bool leftReleased;
@property bool rightReleased;
@property double lastFrameTime;

@property(nonatomic, strong) NSWindow *window;

- (void)updateFrame;
- (void)handleInput:(NSEvent *)event;
@end

@implementation AppDelegate
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
	int windowStyleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
	                      NSWindowStyleMaskResizable
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
	color_space = CGColorSpaceCreateDeviceRGB();

	// Stopt app streching on resize
	self.window.contentView.wantsLayer = YES;
	self.window.contentView.layer.contentsGravity = kCAGravityTopLeft;

	// Allowing redraw during resize
	CADisplayLink *displayLink = [self.window.contentView displayLinkWithTarget:self selector:@selector(updateFrame)];
	[displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];

	NSEventMask inputEvents = NSEventMaskKeyDown | NSEventMaskMouseMoved | NSEventMaskScrollWheel
	                          | NSEventMaskLeftMouseDown | NSEventMaskLeftMouseUp | NSEventMaskLeftMouseDragged
	                          | NSEventMaskRightMouseDown | NSEventMaskRightMouseUp | NSEventMaskRightMouseDragged;
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

	// Render at the display's native resolution
	CGFloat scale = self.window.backingScaleFactor;
	int width = (int) (self.window.contentView.bounds.size.width * scale);
	int height = (int) (self.window.contentView.bounds.size.height * scale);
	env.scale = scale;

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

	if (!app_initialised)
	{
		module.app_init(&env);
		app_initialised = true;
	}

	// Updating the actual app
	module.app_update(&env);
	self.inputUsed = true;

	// Present the framebuffer, pixels are 0xAARRGGBB
	size_t pitch = width * sizeof(uint32_t);
	CGDataProviderRef provider = CGDataProviderCreateWithData(NULL, env.buffer, pitch * height, NULL);
	CGImageRef frame = CGImageCreate(width, height, 8, 32, pitch, color_space,
	                                 kCGBitmapByteOrder32Little | kCGImageAlphaFirst,
	                                 provider, NULL, false, kCGRenderingIntentDefault);
	self.window.contentView.layer.contentsScale = scale;
	self.window.contentView.layer.contents = (id)frame;
	CGImageRelease(frame);
	CGDataProviderRelease(provider);
	self.lastFrameTime = currentFrameTime;

	[self resetInput];
}

- (void)handleInput:(NSEvent *)event
{
	switch (event.type)
	{
	case NSEventTypeKeyDown:
		env.key_down = true;
		if (event.keyCode == 96)
		{
			AppStateHandle handle = module.app_pre_reload();
			load_module(&module, "./everything.dylib");
			module.app_post_reload(handle);
			module.app_init(&env);
		}
		env.key_code = event.keyCode;
		break;

	// Releases are applied after the frame so a quick click is still seen
	case NSEventTypeLeftMouseDown:
		env.mouse_left_down = true;
		self.leftReleased = false;
		break;
	case NSEventTypeLeftMouseUp:
		self.leftReleased = true;
		break;
	case NSEventTypeRightMouseDown:
		env.mouse_right_down = true;
		self.rightReleased = false;
		break;
	case NSEventTypeRightMouseUp:
		self.rightReleased = true;
		break;

	case NSEventTypeMouseMoved:
	case NSEventTypeLeftMouseDragged:
	case NSEventTypeRightMouseDragged:
		env.mouse_moved = true;
		break;

	case NSEventTypeScrollWheel:
	{
		// Trackpads report points, mouse wheels report lines
		float scale = self.window.backingScaleFactor;
		if (!event.hasPreciseScrollingDeltas) scale *= SCROLL_LINE_HEIGHT;
		env.scroll_x += event.scrollingDeltaX * scale;
		env.scroll_y += event.scrollingDeltaY * scale;
	}
	break;

	default:
		break;
	}

	if (event.type != NSEventTypeKeyDown)
	{
		NSPoint mouseLoc = [event locationInWindow];
		double screenHeight = self.window.contentView.bounds.size.height;
		mouseLoc.y = screenHeight - mouseLoc.y;

		env.mouse_x = (int) (mouseLoc.x * self.window.backingScaleFactor);
		env.mouse_y = (int) (mouseLoc.y * self.window.backingScaleFactor);
	}

	self.inputUsed = false;
}

- (void)resetInput
{
	if (!self.inputUsed) return;

	env.key_down = false;
	env.mouse_moved = false;
	env.scroll_x = 0;
	env.scroll_y = 0;

	if (self.leftReleased)
	{
		env.mouse_left_down = false;
		self.leftReleased = false;
	}
	if (self.rightReleased)
	{
		env.mouse_right_down = false;
		self.rightReleased = false;
	}
}

@end

int main(void)
{
	load_module(&module, "./everything.dylib");
	module.app_load();

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

