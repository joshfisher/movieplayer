//
//  AppDelegate.m
//  curve
//
//  Created by Joshua Fisher on 12/31/12.
//  Copyright (c) 2012 Joshua Fisher. All rights reserved.
//

#import "AppDelegate.h"
#import "GLView.h"

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
	self.glview = [[GLView alloc] init];
	self.window.contentView = self.glview;
	
	[NSTimer scheduledTimerWithTimeInterval:1.0/60.0 target:self selector:@selector(timer) userInfo:nil repeats:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
	return YES;
}

-(IBAction)open:(id)sender {
	NSOpenPanel* openDlg = [NSOpenPanel openPanel];
	
	[openDlg setCanChooseFiles:YES];
	[openDlg setCanChooseDirectories:NO];
	[openDlg setAllowsMultipleSelection:NO];
	
	if([openDlg runModal] == NSOKButton) {
		NSArray* files = [openDlg URLs];
		[self.glview open:[[files lastObject] path]];
	}
}

-(IBAction)play:(id)sender {
	if([sender state] == NSOnState) {
		[self.glview pause];
		[sender setState:NSOffState];
	}
	else {
		[self.glview play];
		[sender setState:NSOnState];
	}
}

- (void)timer {
	[self.glview setNeedsDisplay:YES];
}

@end
