//
//  OEPrefControlsController.m
//  OpenEmu
//
//  Created by Christoph Leimbrock on 07.06.11.
//  Copyright 2011 none. All rights reserved.
//

#import <Quartz/Quartz.h>

#import "OEPrefControlsController.h"
#import "OEBackgroundGradientView.h"
#import "OEBackgroundImageView.h"
#import "OELibraryDatabase.h"

#import "OEPlugin.h"
#import "OEDBSystem.h"
#import "OESystemPlugin.h"
#import "OESystemController.h"

#import "OEControllerImageView.h"
#import "OEControlsSetupView.h"

#import "OEHIDEvent.h"

@interface OEPrefControlsController ()
{
    OEHIDEventAxis readingAxis;
}

- (void)OE_rebuildSystemsMenu;
- (void)OE_setupControllerImageViewWithTransition:(NSString *)transition;
- (void)OE_openPaneWithNotification:(NSNotification *)notification;
@end

@implementation OEPrefControlsController

#pragma mark Properties
@synthesize controllerView, controllerContainerView;

@synthesize consolesPopupButton, playerPopupButton, inputPopupButton, keyBindings;

@synthesize gradientOverlay;
@synthesize controlsContainer;
@synthesize controlsSetupView;
@synthesize selectedPlayer;
@synthesize selectedBindingType;
@synthesize selectedKey;

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

#pragma mark -
#pragma mark ViewController Overrides

- (void)awakeFromNib
{
    [super awakeFromNib];
    
    [[self controlsSetupView] setTarget:self];
    [[self controlsSetupView] setAction:@selector(changeInputControl:)];
    
    NSUserDefaults *sud = [NSUserDefaults standardUserDefaults];
    
    NSImage *controlsBackgroundImage = [NSImage imageNamed:@"controls_background"];
    [(OEBackgroundImageView *)[self view] setImage:controlsBackgroundImage];
    
    /** ** ** ** ** ** ** ** **/
    // Setup controls popup console list
    [self OE_rebuildSystemsMenu];
    
    // restore previous state
    NSInteger binding = [sud integerForKey:UDControlsDeviceTypeKey];
    [[self inputPopupButton] selectItemWithTag:binding];
    [self changeInputDevice:self];
    
    NSString *pluginName = [sud stringForKey:UDControlsPluginIdentifierKey];
    [[self consolesPopupButton] selectItemAtIndex:0];
    for(NSMenuItem *anItem in [[self consolesPopupButton] itemArray])
    {
        if([[anItem representedObject] isEqual:pluginName])
        {
            [[self consolesPopupButton] selectItem:anItem];
            break;
        }
    }
    
    [CATransaction setDisableActions:YES];
    [self changeSystem:[self consolesPopupButton]];
    [CATransaction commit];
    
    [self gradientOverlay].topColor = [NSColor colorWithDeviceWhite:0.0 alpha:0.3];
    [self gradientOverlay].bottomColor = [NSColor colorWithDeviceWhite:0.0 alpha:0.0];
    
    [[self controllerView] setWantsLayer:YES];    
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(systemsChanged) name:OEDBSystemsChangedNotificationName object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(OE_openPaneWithNotification:) name:OEPreferencesOpenPaneNotificationName object:nil];
}

- (void)animationDidStart:(CAAnimation *)theAnimation
{
    [[self controllerView].layer setValue:[NSNumber numberWithFloat:1.0] forKeyPath:@"filters.pixellate.inputScale"];
}

- (void)animationDidStop:(CAAnimation *)theAnimation finished:(BOOL)flag
{
    if(flag)
    {          
        [[self controllerView].layer setValue:[NSNumber numberWithInt:10.0] forKeyPath:@"filters.pixellate.inputScale"];
    }
}

- (NSString *)nibName
{
    return @"OEPrefControlsController";
}

- (void)systemsChanged
{
    NSLog(@"OEDBSystemsChangedNotificationName");
    NSMenuItem *menuItem = [[self consolesPopupButton] selectedItem];
    NSString *selectedSystemIdentifier = [menuItem representedObject];

    [self OE_rebuildSystemsMenu];
    
    [[self consolesPopupButton] selectItemAtIndex:0];
    for(NSMenuItem *anItem in [[self consolesPopupButton] itemArray])
    {
        if([[anItem representedObject] isEqual:selectedSystemIdentifier])
        {
            [[self consolesPopupButton] selectItem:anItem];
            break;
        }
    }
    
    [CATransaction setDisableActions:YES];
    [self changeSystem:[self consolesPopupButton]];
    [CATransaction commit];
}

- (OESystemController *)currentSystemController;
{
    return [selectedPlugin controller];
}

#pragma mark -
#pragma mark UI Methods

- (void)OE_setupPlayerMenuForNumberOfPlayers:(NSUInteger)numberOfPlayers;
{
    NSMenu *playerMenu = [[NSMenu alloc] init];
    for(NSUInteger player = 0; player < numberOfPlayers; player++)
    {
        NSString   *playerTitle = [NSString stringWithFormat:NSLocalizedString(@"Player %ld", @""), player + 1];
        NSMenuItem *playerItem  = [[NSMenuItem alloc] initWithTitle:playerTitle action:NULL keyEquivalent:@""];
        [playerItem setTag:player + 1];
        [playerMenu addItem:playerItem];
    }
    
    [[self playerPopupButton] setMenu:playerMenu];
    
    // Hide player PopupButton if there is only one player
    [[self playerPopupButton] setHidden:(numberOfPlayers == 1)];
    [[self playerPopupButton] selectItemWithTag:[[NSUserDefaults standardUserDefaults] integerForKey:UDControlsPlayerKey]];
}

- (NSMenu *)OE_playerMenuForPlayerCount:(NSUInteger)numberOfPlayers;
{
    NSMenu *playerMenu = [[NSMenu alloc] init];
    for(NSUInteger player = 0; player < numberOfPlayers; player++)
    {
        NSString   *playerTitle = [NSString stringWithFormat:NSLocalizedString(@"Player %ld", @""), player + 1];
        NSMenuItem *playerItem  = [[NSMenuItem alloc] initWithTitle:playerTitle action:NULL keyEquivalent:@""];
        [playerItem setTag:player + 1];
        [playerMenu addItem:playerItem];
    }
    
    return playerMenu;
}

- (IBAction)changeSystem:(id)sender
{
    NSUserDefaults *sud = [NSUserDefaults standardUserDefaults];
    
    NSMenuItem *menuItem = [[self consolesPopupButton] selectedItem];
    NSString *systemIdentifier = [menuItem representedObject];
    
    NSString *oldPluginName = [selectedPlugin systemName];
    
    OESystemPlugin *nextPlugin = [OESystemPlugin gameSystemPluginForIdentifier:systemIdentifier];
    if(selectedPlugin != nil && nextPlugin == selectedPlugin) return;
    selectedPlugin = nextPlugin;
    
    OESystemController *systemController = [self currentSystemController];
    
    [self setKeyBindings:[[systemController controllerKeyPositions] allKeys]];
    
    // Rebuild player menu
    [self OE_setupPlayerMenuForNumberOfPlayers:[systemController numberOfPlayers]];
    
    OEControlsSetupView *preferenceView = [self controlsSetupView];
    [preferenceView setupWithControlList:[systemController controlPageList]];
    [preferenceView setAutoresizingMask:NSViewMaxXMargin | NSViewMaxYMargin];
    
    NSRect rect = (NSRect){ .size = { [self controlsSetupView].bounds.size.width, preferenceView.frame.size.height }};
    [preferenceView setFrame:rect];
    
    NSScrollView *scrollView = [[self controlsSetupView] enclosingScrollView];
    [[self controlsSetupView] setFrameOrigin:(NSPoint){ 0, scrollView.frame.size.height - rect.size.height}];
    
    if([[self controlsSetupView] frame].size.height <= scrollView.frame.size.height)
    {
        [scrollView setVerticalScrollElasticity:NSScrollElasticityNone];
    }
    else
    {
        [scrollView setVerticalScrollElasticity:NSScrollElasticityAutomatic];
        [scrollView flashScrollers];
    }
    
    [sud setObject:systemIdentifier forKey:UDControlsPluginIdentifierKey];
    
    [self changePlayer:[self playerPopupButton]];
    [self changeInputDevice:[self inputPopupButton]];
    
    NSComparisonResult order = [oldPluginName compare:[selectedPlugin systemName]];
    [self OE_setupControllerImageViewWithTransition:(order == NSOrderedDescending ? kCATransitionFromLeft : kCATransitionFromRight)];
}

- (void)OE_setupControllerImageViewWithTransition:(NSString *)transition;
{
    OESystemController *systemController = [self currentSystemController];
    
    OEControllerImageView *newControllerView = [[OEControllerImageView alloc] initWithFrame:[[self controllerContainerView] bounds]];
    [newControllerView setImage:[systemController controllerImage]];
    [newControllerView setImageMask:[systemController controllerImageMask]];
    [newControllerView setKeyPositions:[systemController controllerKeyPositions]];
    [newControllerView setTarget:self];
    [newControllerView setAction:@selector(changeInputControl:)];
    
    // Animation for controller image swapping
    CATransition *controllerTransition = [CATransition animation];
    [controllerTransition setType:kCATransitionPush];
    [controllerTransition setTimingFunction:[CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionDefault]];
    [controllerTransition setDuration:1.0];
    [controllerTransition setSubtype:transition];
    [controllerTransition setRemovedOnCompletion:YES];
    
    [[self controllerContainerView] setAnimations:[NSDictionary dictionaryWithObject:controllerTransition forKey:@"subviews"]];
    
    if(controllerView != nil)
        [[[self controllerContainerView] animator] replaceSubview:controllerView with:newControllerView];
    else
        [[[self controllerContainerView] animator] addSubview:newControllerView];
    
    [[self controllerContainerView] setAnimations:[NSDictionary dictionary]];
    
    [self setControllerView:newControllerView];
}

- (IBAction)changePlayer:(id)sender
{
    NSInteger player = 1;
    if(sender && [sender respondsToSelector:@selector(selectedTag)])
        player = [sender selectedTag];
    else if(sender && [sender respondsToSelector:@selector(tag)])
        player = [sender tag];
    
    [self setSelectedPlayer:player];
    [self resetKeyBindings];
    [[NSUserDefaults standardUserDefaults] setInteger:player forKey:UDControlsPlayerKey];
}

- (IBAction)changeInputDevice:(id)sender
{
    NSInteger bindingType = 0;
    if(sender && [sender respondsToSelector:@selector(selectedTag)])
        bindingType = [sender selectedTag];
    else if(sender && [sender respondsToSelector:@selector(tag)])
        bindingType = [sender tag];
    
    [self setSelectedBindingType:bindingType];
    [self resetKeyBindings];
    [[NSUserDefaults standardUserDefaults] setInteger:bindingType forKey:UDControlsDeviceTypeKey];
}

- (IBAction)changeInputControl:(id)sender
{
    if(sender == [self controllerView] || sender == [self controlsSetupView])
        [self setSelectedKey:[sender selectedKey]];
}

- (void)setSelectedKey:(NSString *)value
{
    if(selectedKey == value) value = nil;
    
    selectedKey = [value copy];
    
    [[self controlsSetupView] setSelectedKey:selectedKey];
    [[self controllerView]    setSelectedKey:selectedKey animated:YES];
    
    [[[self view] window] makeFirstResponder:selectedKey != nil ? [self view] : nil];
}

- (void)setSelectedBindingType:(NSInteger)value
{
    if(selectedBindingType != value)
    {
        selectedBindingType = value;
        [[self inputPopupButton] selectItemWithTag:selectedBindingType];
    }
}

#pragma mark -
#pragma mark Input and bindings management methods

- (void)resetBindingsWithKeys:(NSArray *)keys
{
    for(NSString *key in keys)
    {
        [self willChangeValueForKey:key];
        [self didChangeValueForKey:key];
    }
}

- (void)resetKeyBindings
{
    [self resetBindingsWithKeys:[self keyBindings]];
}

- (BOOL)isKeyboardEventSelected
{
    return selectedBindingType == 0;
}

- (NSString *)keyPathForKey:(NSString *)aKey
{
    NSUInteger player = [self selectedPlayer];
    
    return player != NSNotFound ? [[self currentSystemController] playerKeyForKey:aKey player:player] : aKey;
}

- (void)registerEvent:(id)anEvent
{
    if([self selectedKey] != nil)
    {
        [self setValue:anEvent forKey:[self selectedKey]];
        [[self controlsSetupView] selectNextKeyButton];
        [self changeInputControl:[self controlsSetupView]];
    }
}

- (void)keyDown:(NSEvent *)theEvent
{
}

- (void)keyUp:(NSEvent *)theEvent
{
}

- (void)axisMoved:(OEHIDEvent *)anEvent
{
    OEHIDEventAxis axis    = [anEvent axis];
    OEHIDAxisDirection dir = [anEvent direction];
    
    if(readingAxis == OEHIDAxisNone && axis != OEHIDAxisNone && dir != OEHIDAxisDirectionNull)
    {
        readingAxis = axis;
        
        [self setSelectedBindingType:1];
        [self registerEvent:anEvent];
    }
    else if(readingAxis == axis && dir == OEHIDAxisDirectionNull)
        readingAxis = OEHIDAxisNone;
}

- (void)buttonDown:(OEHIDEvent *)anEvent
{
    [self setSelectedBindingType:1];
    [self registerEvent:anEvent];
}

- (void)hatSwitchChanged:(OEHIDEvent *)anEvent;
{
    if([anEvent hatDirection] != OEHIDHatDirectionNull)
    {
        [self setSelectedBindingType:1];
        [self registerEvent:anEvent];
    }
}

- (void)HIDKeyDown:(OEHIDEvent *)anEvent
{
    [self setSelectedBindingType:0];
    [self registerEvent:anEvent];
}

- (id)valueForKey:(NSString *)key
{
    if([[[self currentSystemController] genericControlNames] containsObject:key])
    {
        id anEvent = nil;
        if([self isKeyboardEventSelected])
            anEvent = [[self currentSystemController] keyboardEventForKey:[self keyPathForKey:key]];
        else
            anEvent = [[self currentSystemController] HIDEventForKey:[self keyPathForKey:key]];
        
        return (anEvent != nil
                ? ([anEvent respondsToSelector:@selector(displayDescription)]
                   ? [anEvent displayDescription]
                   : [anEvent description])
                : @"<empty>");
    }
    return [super valueForKey:key];
}

- (void)setValue:(id)value forKey:(NSString *)key
{
    // should be mutually exclusive
    if([[[self currentSystemController] genericControlNames] containsObject:key])
    {
        [[self currentSystemController] registerEvent:value forKey:[self keyPathForKey:key]];
        [self resetKeyBindings];
    }
    else [super setValue:value forKey:key];
}

- (void)mouseDown:(NSEvent *)theEvent
{
    if([self selectedKey]) [self setSelectedKey:[self selectedKey]];
}

#pragma mark -
#pragma mark OEPreferencePane Protocol

- (NSImage *)icon
{
    return [NSImage imageNamed:@"controls_tab_icon"];
}

- (NSString *)title
{
    return @"Controls";
}

- (NSString *)localizedTitle
{
    return NSLocalizedString([self title], @"");
}

- (NSSize)viewSize
{
    return NSMakeSize(561, 534);
}

- (NSColor *)toolbarSeparationColor
{
    return [NSColor colorWithDeviceWhite:0.32 alpha:1.0];
}

#pragma mark -
- (void)OE_openPaneWithNotification:(NSNotification *)notification
{
    NSDictionary *userInfo = [notification userInfo];
    NSString     *paneName = [userInfo valueForKey:OEPreferencesOpenPanelUserInfoPanelNameKey];
    
    if([paneName isNotEqualTo:[self title]]) return;
    
    NSString *systemIdentifier = [userInfo valueForKey:OEPreferencesOpenPanelUserInfoSystemIdentifierKey];

    NSUInteger i = 0;
    for(i = 0; i < [[[self consolesPopupButton] itemArray] count] - 1; i++)
    {
        NSMenuItem *item = [[[self consolesPopupButton] itemArray] objectAtIndex:i];
        
        if([[item representedObject] isEqual:systemIdentifier]) break;
    }

    [[self consolesPopupButton] selectItemAtIndex:i];
    [self changeSystem:nil];
}

- (void)OE_rebuildSystemsMenu
{
    NSMenu *consolesMenu = [[NSMenu alloc] init];
    NSArray *enabledSystems = [[OELibraryDatabase defaultDatabase] enabledSystems]; 

    for(OEDBSystem *system in enabledSystems)
    {
        OESystemPlugin *plugin = [system plugin];
        NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:[plugin systemName] action:@selector(changeSystem:) keyEquivalent:@""];
        [item setTarget:self];
        [item setRepresentedObject:[plugin systemIdentifier]];
        
        [item setImage:[plugin systemIcon]];
        
        [consolesMenu addItem:item];
    }
    
    [[self consolesPopupButton] setMenu:consolesMenu];
}

@end
