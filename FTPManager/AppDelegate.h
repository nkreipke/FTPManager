//
//  AppDelegate.h
//  FTPManager
//
//  Created by Nico Kreipke on 08.06.12.
//  Copyright (c) 2012 nkreipke. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "FTPManager.h"

enum actions {
    upload, download, list, newfolder, del, chmod, nothing
    };

@interface AppDelegate : NSObject <NSApplicationDelegate> {

    __weak NSTextField *_serverURLField;
    __weak NSTextField *_loginUserField;
    __weak NSSecureTextField *_loginPasswordField;
    __weak NSTextField *_directoryField;
    
    
    __unsafe_unretained NSPanel *_fileListOutputPanel;
    __unsafe_unretained NSTextView *_fileListOutputField;
    
    
    __unsafe_unretained NSPanel *_actionPanel;
    __weak NSTextField *_actionProgressField;
    __weak NSProgressIndicator *_actionProgressBar;
    
    __unsafe_unretained NSPanel *_downloadFilePanel;
    __weak NSTextField *_downloadFileField;
    
    __unsafe_unretained NSPanel *_directoryPanel;
    __weak NSTextField *_createDirectoryField;
    
    NSURL* fileURL;
    enum actions action;
    
    NSTimer* progressTimer;
    
    BOOL success;
    BOOL aborted;
    
    FTPManager* ftpManager;
    
}

@property (assign) IBOutlet NSWindow *window;

@property (weak) IBOutlet NSTextField *serverURLField;
@property (weak) IBOutlet NSTextField *loginUserField;
@property (weak) IBOutlet NSSecureTextField *loginPasswordField;
@property (weak) IBOutlet NSTextField *directoryField;
@property (weak) IBOutlet NSTextField *portField;

- (IBAction)pushUploadAFile:(id)sender;
- (IBAction)pushDownloadAFile:(id)sender;
- (IBAction)pushListFiles:(id)sender;
- (IBAction)pushCreateADirectory:(id)sender;


@property (unsafe_unretained) IBOutlet NSPanel *fileListOutputPanel;
@property (unsafe_unretained) IBOutlet NSTextView *fileListOutputField;
@property (unsafe_unretained) IBOutlet NSPanel *actionPanel;
@property (weak) IBOutlet NSProgressIndicator *actionProgressBar;
@property (weak) IBOutlet NSTextField *actionProgressField;
@property (unsafe_unretained) IBOutlet NSPanel *downloadFilePanel;
@property (weak) IBOutlet NSTextField *downloadFileField;
@property (unsafe_unretained) IBOutlet NSPanel *directoryPanel;
@property (weak) IBOutlet NSTextField *createDirectoryField;
@property (unsafe_unretained) IBOutlet NSPanel *deletePanel;
@property (weak) IBOutlet NSTextField *deleteFileField;
@property (unsafe_unretained) IBOutlet NSPanel *chmodPanel;
@property (weak) IBOutlet NSTextField *chmodFileField;
@property (weak) IBOutlet NSTextField *chmodModeField;
@end
