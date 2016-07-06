/*
 * consumer_sdl_osx.m -- An OS X compatibility shim for SDL
 * Copyright (C) 2010-2014 Meltytech, LLC
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#import <Cocoa/Cocoa.h>

void* mlt_cocoa_autorelease_init()
{
	return [[NSAutoreleasePool alloc] init];
}

void mlt_cocoa_autorelease_close( void* p )
{
	NSAutoreleasePool* pool = ( NSAutoreleasePool* ) p;
	[pool release];
}

#if 0
/* The code below is not used at this time - could not get it to work, but
 * it is based on code from gruntster on the avidemux project team.
 */

#import <Carbon/Carbon.h>

static NSWindow* nsWindow = nil;
static NSQuickDrawView* nsView = nil;

void sdl_cocoa_init(void* parent, int x, int y, int width, int height)
{
	NSRect contentRect;
	contentRect = NSMakeRect(x, y, width, height);		

	if (!nsWindow)
	{
		// initWithWindowRef always returns the same result for the same WindowRef
		nsWindow = [[NSWindow alloc] initWithWindowRef:(WindowRef)parent];
		[nsWindow setContentView:[[[NSView alloc] initWithFrame:contentRect] autorelease]];
	}

	if (!nsView)
	{
		nsView = [[NSQuickDrawView alloc] initWithFrame:contentRect];
		[[nsWindow contentView] addSubview:nsView];
		[nsView release];
		[nsWindow orderOut:nil];	// very important, otherwise window won't be placed correctly on repeat showings
		[nsWindow orderFront:nil];
	}
	else
	{
		[nsView setFrame:contentRect];
		[[nsWindow contentView] setFrame:contentRect];
	}

	// finally, set SDL environment variables with all this nonsense
	char SDL_windowhack[20];
	sprintf(SDL_windowhack, "%d", (int)nsWindow);
	setenv("SDL_NSWindowPointer", SDL_windowhack, 1);
	sprintf(SDL_windowhack,"%d", (int)nsView);
	setenv("SDL_NSQuickDrawViewPointer", SDL_windowhack, 1);
}

void sdl_cocoa_close(void)
{
  if (nsWindow)
  {
  	// Reference count cannot fall below 2 because SDL releases the window when closing
	  // and again when reinitialising (even though this is our own window...).
	  if ([nsWindow retainCount] > 2)
		  [nsWindow release];

  	// SDL takes care of all the destroying...a little too much, so make sure our Carbon
	  // window is still displayed (via its Cocoa wrapper)
	  [nsWindow makeKeyAndOrderFront:nil];
	 }
}
#endif
