//
//  MTStatusBarOverlay.m
//
//  Created by Matthias Tretter on 27.09.10.
//  Copyright (c) 2009-2011  Matthias Tretter, @myell0w. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// Credits go to:
// -------------------------------
// http://stackoverflow.com/questions/2833724/adding-view-on-statusbar-in-iphone
// http://www.cocoabyss.com/uikit/custom-status-bar-ios/
// @reederapp for inspiration
// -------------------------------

#import <Foundation/Foundation.h>
#import "MTStatusBarTableViewCell.h"

//===========================================================
#pragma mark -
#pragma mark Definitions and Types
//===========================================================

// Animation that happens, when the user touches the status bar overlay
typedef enum MTStatusBarOverlayAnimation {
	MTStatusBarOverlayAnimationNone,      // nothing happens
	MTStatusBarOverlayAnimationShrink,    // the status bar shrinks to the right side and only shows the activity indicator
	MTStatusBarOverlayAnimationFallDown   // the status bar falls down and displays more information
} MTStatusBarOverlayAnimation;


// Mode of the detail view
typedef enum MTDetailViewMode {
	MTDetailViewModeHistory,			// History of messages is recorded and displayed in detailView
	MTDetailViewModeDetailText,			// a text can be displayed easily
	MTDetailViewModeCustom				// the detailView can be customized in the way the developer wants
} MTDetailViewMode;

// indicates the type of a message
typedef enum MTMessageType {
	MTMessageTypeActivity,				// shows actvity indicator
	MTMessageTypeFinish,				// shows checkmark
	MTMessageTypeError,   				// shows error-mark
    MTMessageTypePlain                  // doesn't show anything special
} MTMessageType;


// keys used in the dictionary-representation of a status message
#define kMTStatusBarOverlayMessageKey			@"MessageText"
#define kMTStatusBarOverlayMessageTypeKey		@"MessageType"
#define kMTStatusBarOverlayDurationKey			@"MessageDuration"
#define kMTStatusBarOverlayAnimationKey			@"MessageAnimation"
#define kMTStatusBarOverlayImmediateKey			@"MessageImmediate"
#define kMTStatusBarOverlayKeyKey               @"MessageKey"
#define kSetProgressOnStatusBarKey              @"UniqueProgressLabelSetProgressKey"

// keys used for saving state to NSUserDefaults
#define kMTStatusBarOverlayStateShrinked        @"kMTStatusBarOverlayStateShrinked"


// forward-declaration of delegate-protocol
@protocol MTStatusBarOverlayDelegate;



//===========================================================
#pragma mark -
#pragma mark MTStatusBarOverlay Interface
//===========================================================


// This class provides an overlay over the iOS Status Bar that can display information
// and perform an animation when you touch it:
// it can either shrink and only overlap the battery-icon (like in Reeder) or it can display
// a detail-view that shows additional information. You can show a history of all the previous
// messages for free by setting historyEnabled to YES
@interface MTStatusBarOverlay : UIWindow <UITableViewDataSource, UITableViewDelegate> {
	// holds all subviews, is touchable to change size of Status Bar
	UIView *backgroundView_;
	// the view that is shown in animation mode "FallDown" when the user touches the status bar
	UIView *detailView_;

	// background of Status Bar Black or gray
	UIImageView *statusBarBackgroundImageView_;
	// for displaying Text information
	UILabel *statusLabel1_;
	UILabel *statusLabel2_;
	UILabel *__unsafe_unretained hiddenStatusLabel_;

    // used for displaying progress-information
    UIImageView *progressView_;
    float progress_;
    UIView *maskingProgressView_;
    UILabel *maskingLabel_;

	// for displaying activity indication
	UIActivityIndicatorView *activityIndicator_;
	UILabel *finishedLabel_;
	// if set to YES, neither activityIndicator nor finishedLabel are shown
	BOOL hidesActivity_;

	// Image of gray Status Bar
	UIImage *defaultStatusBarImage_;
	UIImage *defaultStatusBarImageShrinked_;

	// Animation-Type
	MTStatusBarOverlayAnimation animation_;
	// Small size of Status Bar
	CGRect smallFrame_;
	// temporary variable used for rotation
	CGRect oldBackgroundViewFrame_;
	// is set when finishWithMessage is called and the statusBar is set to be hidden
	// after a specified amount of time
	BOOL hideInProgress_;
	// flag that indicates whether currently a message is in the queue
	BOOL active_;

	// Queue stuff
	NSMutableArray *messageQueue_;
	// if YES older immediate messages in the queue get removed, when a new one gets posted
	BOOL canRemoveImmediateMessagesFromQueue_;

	// Detail View
	MTDetailViewMode detailViewMode_;
	NSString *detailText_;
	UITextView *detailTextView_;

	// Message history (is reset when finish is called)
	NSMutableArray *messageHistory_;
	UITableView *historyTableView_;
    NSMutableArray *keyArray;

	// the delegate
	id<MTStatusBarOverlayDelegate> __unsafe_unretained delegate_;
    
    //Used to determine whether MTStatusBarOverlay should reshow status bar after hiding itself.
    BOOL statusBarHidden_;
}

//===========================================================
#pragma mark -
#pragma mark Properties
//===========================================================
// the view that holds all the components of the overlay (except for the detailView)
@property (nonatomic, strong) UIView *backgroundView;
// the detailView is shown when animation is set to "FallDown"
@property (nonatomic, strong) UIView *detailView;
// the current progress
@property (nonatomic, assign) float progress;
// the frame of the status bar when animation is set to "Shrink" and it is shrinked
@property (nonatomic, assign) CGRect smallFrame;
// the current active animation
@property (nonatomic, assign) MTStatusBarOverlayAnimation animation;
// the label that holds the finished-indicator (either a checkmark, or a error-sign per default)
@property (nonatomic, strong) UILabel *finishedLabel;
// if this flag is set to YES, neither activityIndicator nor finishedLabel are shown
@property (nonatomic, assign) BOOL hidesActivity;
// the image used when the Status Bar Style is Default
@property (nonatomic, strong) UIImage *defaultStatusBarImage;
// the image used when the Status Bar Style is Default and the Overlay is shrinked
@property (nonatomic, strong) UIImage *defaultStatusBarImageShrinked;
// detect if status bar is currently shrinked
@property (nonatomic, readonly, getter=isShrinked) BOOL shrinked;
// detect if detailView is currently hidden
@property (nonatomic, readonly, getter=isDetailViewHidden) BOOL detailViewHidden;
// all messages that were displayed since the last finish-call
@property (nonatomic, strong, readonly) NSMutableArray *messageHistory;
// DEPRECATED: enable/disable history-tracking of messages
@property (nonatomic, assign, getter=isHistoryEnabled) BOOL historyEnabled;
// determines if immediate messages in the queue get removed or stay in the queue, when a new immediate message gets posted
@property (nonatomic, assign) BOOL canRemoveImmediateMessagesFromQueue;
// the mode of the detailView
@property (nonatomic, assign) MTDetailViewMode detailViewMode;
// the text displayed in the detailView (alternative to history)
@property (nonatomic, copy) NSString *detailText;
// the delegate of the overlay
@property (nonatomic, unsafe_unretained) id<MTStatusBarOverlayDelegate> delegate;
@property (nonatomic, strong) UIView *maskingProgressView;
@property (nonatomic, strong) UILabel *maskingLabel;
@property (nonatomic, strong) NSMutableArray *keyArray;
@property (nonatomic) BOOL statusBarHidden;

//===========================================================
#pragma mark -
#pragma mark Class Methods
//===========================================================

// Singleton Instance
+ (MTStatusBarOverlay *)sharedInstance;
+ (MTStatusBarOverlay *)sharedOverlay;
+ (MTStatusBarOverlay *)threadSafeSharedInstance;
+ (MTStatusBarOverlay *)threadSafeSharedOverlay;

//===========================================================
#pragma mark -
#pragma mark Instance Methods
//===========================================================

// for customizing appearance, automatically disabled userInteractionEnabled on view
- (void)addSubviewToBackgroundView:(UIView *)view;
- (void)addSubviewToBackgroundView:(UIView *)view atIndex:(NSInteger)index;

- (void)setStatusBarStyleManually:(UIStatusBarStyle)statusBarStyle;
- (void)setStatusBarStyleManually:(UIStatusBarStyle)statusBarStyle animated:(BOOL)animated;

// shows an activity indicator and the given message
- (void)postMessage:(NSString *)message key:(NSString *)key;
- (void)postMessage:(NSString *)message key:(NSString *)key duration:(NSTimeInterval)duration;
- (void)postMessage:(NSString *)message key:(NSString *)key duration:(NSTimeInterval)duration animated:(BOOL)animated;
- (void)postMessage:(NSString *)message key:(NSString *)key animated:(BOOL)animated;

- (void)removeMessageFromHistoryForKey:(id)key;
- (void)removeAllMessages;
- (void)setProgress:(double)progress key:(NSString *)key;
- (void)showErrorMessage:(NSString *)string forKey:(id)key duration:(double)duration;
- (void)showFinishMessage:(NSString *)string forKey:(id)key duration:(double)duration;
- (void)showMessage:(NSString *)string forType:(MTMessageType)messageType forKey:(id)key duration:(double)duration;

// clears the message queue and shows this message instantly
- (void)postImmediateMessage:(NSString *)message key:(NSString *)key animated:(BOOL)animated;
- (void)postImmediateMessage:(NSString *)message key:(NSString *)key duration:(NSTimeInterval)duration animated:(BOOL)animated;

// shows a checkmark instead of the activity indicator and hides the status bar after the specified duration
- (void)postFinishMessage:(NSString *)message key:(NSString *)key duration:(NSTimeInterval)duration;
- (void)postFinishMessage:(NSString *)message key:(NSString *)key duration:(NSTimeInterval)duration animated:(BOOL)animated;
// clears the message queue and shows this message instantly
- (void)postImmediateFinishMessage:(NSString *)message key:(NSString *)key duration:(NSTimeInterval)duration animated:(BOOL)animated;

// shows a error-sign instead of the activity indicator and hides the status bar after the specified duration
- (void)postErrorMessage:(NSString *)message key:(NSString *)key duration:(NSTimeInterval)duration;
- (void)postErrorMessage:(NSString *)message key:(NSString *)key duration:(NSTimeInterval)duration animated:(BOOL)animated;
// clears the message queue and shows this message instantly
- (void)postImmediateErrorMessage:(NSString *)message key:(NSString *)key duration:(NSTimeInterval)duration animated:(BOOL)animated;

- (void)postMessage:(NSString *)message key:(NSString *)key type:(MTMessageType)messageType duration:(NSTimeInterval)duration animated:(BOOL)animated immediate:(BOOL)immediate;

// hides the status bar overlay
- (void)hide;

// saves the state in NSUserDefaults and synchronizes them
- (void)saveState;
- (void)saveStateSynchronized:(BOOL)synchronizeAtEnd;
// restores the state from NSUserDefaults
- (void)restoreState;

@end




//===========================================================
#pragma mark -
#pragma mark Delegate Protocol
//===========================================================

@protocol MTStatusBarOverlayDelegate <NSObject>
@optional
// is called, when a gesture on the overlay is recognized
- (void)statusBarOverlayDidRecognizeGesture:(UIGestureRecognizer *)gestureRecognizer;
// is called when the status bar overlay gets hidden
- (void)statusBarOverlayDidHide;
// is called, when the status bar overlay changed it's displayed message from one message to another
- (void)statusBarOverlayDidSwitchFromOldMessage:(NSString *)oldMessage toNewMessage:(NSString *)newMessage;
// is called when an immediate message gets posted and therefore messages in the queue get lost
// it tells the delegate the lost messages and the delegate can then enqueue the messages again
- (void)statusBarOverlayDidClearMessageQueue:(NSArray *)messageQueue;
@end