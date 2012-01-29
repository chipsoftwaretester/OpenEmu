//
//  OEControlerImageView.h
//  OpenEmu
//
//  Created by Carl Leimbrock on 25.09.11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>
@class OEControlsViewController;
@interface OEControllerImageView : NSView 
@property (nonatomic) float overlayAlpha, ringAlpha;
@property (nonatomic) NSPoint ringPosition;
@property (nonatomic, retain) NSImage *image;
@property (nonatomic, retain) OEControlsViewController *controlsViewController;
- (NSPoint)ringPositionInView;
@end
