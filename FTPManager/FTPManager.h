//
//  FTPManager.h
//  FTPTest
//
//  Created by Nico Kreipke on 11.08.11.
//  Copyright 2012 nkreipke. All rights reserved.
//  http://nkreipke.wordpress.com
//

//  Version 1.2
//  http://creativecommons.org/licenses/by/3.0/

// Information:
// Parts of this class are based on the SimpleFTPSample sample code by Apple.

// This class requires the following frameworks:
// - CoreServices.framework (OS X)
// - CFNetwork.framework (iOS)

// ***********
// CHANGELOG**
// ***********
//
// ** 1.2 (2012-05-28) by nkreipke
//     - Methods added:
//         - (float) progress
//         - (void)  abort
//     - downloadFile:toDirectory:fromServer: now sends an FTP STAT command to
//         determine the size of the file to download.
//
// ** 1.3 (2012-05-28) by jweinert (cs&m GmbH)
//     - Protocol declaration added:
//         FTPManagerDelegate
//     - Delegate Methods added (both optional):
//         - (void)FTPManagerUploadProgressDidChange:(NSDictionary *)upInfo
//         - (void)FTPManagerDownloadProgressDidChange:(NSDictionary *)downInfo
//              Both methods return an NSDictionary providing the following information (keys):
//                  * progress:             progress (0.0f to 1.0f, -1.0 at fail)
//                  * fileSize:             size of the currently processing file
//                  * bytesProcessed:       number of Bytes processed from the current session
//                  * fileSizeProcessed:    number of Bytes processed of the currently processing file
//
// ** 1.4 (2012-06-07) by jweinert (cs&m GmbH)
//     - Methods added:
//         - uploadData:withFileName:toServer:
//         - deleteFile:fromServer:
//
// ** 1.4.1 (2012-06-08) by nkreipke
//     - replaced string keys with (id)kFMProcessInfo constants.
//     - changed method declarations containing an info dictionary to name it processInfo.
//     - fixed broken createNewFolder method in ARC version

// +++++++++++++++++++++++++
// !! ARC ENABLED VERSION !!
// +++++++++++++++++++++++++

#import <Foundation/Foundation.h>

@interface FMServer : NSObject {
//FTPManager Server Object
@private
    NSURL* destination;
    NSString* password;
    NSString* username;
}
@property  NSURL* destination;
@property  NSString* password;
@property  NSString* username;
+ (FMServer*) serverWithDestination:(NSURL*)dest username:(NSString*)user password:(NSString*)pass;
@end

enum {
    kSendBufferSize = 32768
};

typedef enum {
    FMStreamFailureReasonNone,
    FMStreamFailureReasonReadError,
    FMStreamFailureReasonWriteError,
    FMStreamFailureReasonGeneralError,
    FMStreamFailureReasonAborted
} FMStreamFailureReason;

typedef enum {
    _FMCurrentActionUploadFile,
    _FMCurrentActionCreateNewFolder,
    _FMCurrentActionContentsOfServer,
    _FMCurrentActionDownloadFile,
    _FMCurrentActionNone
} _FMCurrentAction;

@protocol FTPManagerDelegate;

// Process Info Dictionary Constants: ************************************
#define kFMProcessInfoProgress @"progress" // 0.0 to 1.0
#define kFMProcessInfoFileSize @"fileSize"
#define kFMProcessInfoBytesProcessed @"bytesProcessed"
#define kFMProcessInfoFileSizeProcessed @"fileSizeProcessed"
// ---------------------------------(returns NSNumber values)-------------

@interface FTPManager : NSObject <NSStreamDelegate> {
    NSInputStream* fileReader;
    NSOutputStream* serverStream;
    NSInputStream* serverReadStream;
    NSOutputStream* fileWriter;
    CFRunLoopRef currentRunLoop;
    NSMutableData* directoryListingData;
    
    _FMCurrentAction action;
    
    uint8_t _buffer[kSendBufferSize];
    size_t _bufferOffset;
    size_t _bufferLimit;
    
    unsigned long long fileSize;
    unsigned long long bytesProcessed;
    unsigned long long fileSizeProcessed;
    
    BOOL streamSuccess;
}

@property (assign) id<FTPManagerDelegate>delegate;

//Public Methods:
// *** Information
// These methods hold the current thread. You will get an answer with a success information.
- (BOOL) uploadFile:(NSURL*)fileURL toServer:(FMServer*)server;
// -(BOOL) uploadFile:toServer:
// Uploads a file to a server.
// Returns YES if the upload was successful, otherwise returns NO.
// Any existing files will be overwritten.
- (BOOL) uploadData:(NSData*)data withFileName:(NSString *)fileName toServer:(FMServer*)server;
// -(BOOL) uploadData:toServer:
// Uploads NSData to a server.
// Returns YES if the upload was successful, otherwise returns NO.
// Any existing files will be overwritten.
- (BOOL) createNewFolder:(NSString*)folderName atServer:(FMServer*)server;
// -(BOOL) createNewFolder:atServer:
// Creates a new folder.
// Returns YES if the creation was successful, otherwise returns NO.
- (NSArray*) contentsOfServer:(FMServer*)server;
// -(NSArray*) contentsOfServer:
// Returns a list of files and folders at a server. Returns an array of NSDictionary.
// Returns nil if there was an error during the process. Returns an empty array if the server has no contents.
// To get the name of the entry, get the object for the (id)kCFFTPResourceName key.
// The dictionaries contain objects declared in CFStream FTP Resource Constants.
- (BOOL) downloadFile:(NSString*)fileName toDirectory:(NSURL*)directoryURL fromServer:(FMServer*)server;
// -(BOOL) downloadFile:toDirectory:fromServer:
// Downloads a file from a server.
// Returns YES if the download was successful, otherwise returns NO.
// Any existing files will be overwritten.
- (BOOL) deleteFile:(NSString *)absolutePath fromServer:(FMServer *)server;
// -(BOOL) deleteFile:fromServer:
// Deletes a file or directory from a server. Use absolute path on server as path parameter!
// When trying to delete directories, make sure that the directory is empty.
// The URL must end with slash (/)!
- (NSMutableDictionary *) progress;
// -(NSMutableDictionary *) progress
// Returns information about the current process. As the FTP methods hold the thread, you may
// want to call this method from a different thread that updates the UI.
// Returns an NSDictionary containing NSNumber values for the keys:
// kFMProcessInfoProgress, kFMProcessInfoFileSize,
// kFMProcessInfoBytesProcessed, kFMProcessInfoFileSizeProcessed
// Returns nil if no process is currently running or information could not be determined.
- (void) abort;
// -(void) abort
// Aborts the current process. As the FTP methods hold the thread, you may want to call this
// method from a different thread.

@end

@protocol FTPManagerDelegate <NSObject>

@optional
- (void)ftpManagerUploadProgressDidChange:(NSDictionary *)processInfo;
// Returns information about the current upload.
// See "Process Info Dictionary Constants" above for detailed info.
@optional
- (void)ftpManagerDownloadProgressDidChange:(NSDictionary *)processInfo;
// Returns information about the current download.
// See "Process Info Dictionary Constants" above for detailed info.

@end