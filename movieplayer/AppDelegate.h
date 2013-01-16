//
//  AppDelegate.h
//  curve
//
//  Created by Joshua Fisher on 12/31/12.
//  Copyright (c) 2012 Joshua Fisher. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class GLView;

@interface AppDelegate : NSObject <NSApplicationDelegate>

@property (nonatomic,assign) IBOutlet NSWindow *window;
@property (nonatomic) GLView *glview;

-(IBAction)open:(id)sender;
-(IBAction)play:(id)sender;

@end
