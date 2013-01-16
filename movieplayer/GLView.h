//
//  GLView.h
//  curve
//
//  Created by Joshua Fisher on 12/31/12.
//  Copyright (c) 2012 Joshua Fisher. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface GLView : NSOpenGLView
-(id)init;
-(void)open:(NSString*)path;
-(void)play;
-(void)pause;
@end
