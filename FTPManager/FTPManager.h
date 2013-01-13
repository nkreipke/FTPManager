//
//  FTPManager.h
//  FTPManager
//
//  Created by Nico Kreipke on 11.08.11.
//  Copyright (c) 2012 nkreipke. All rights reserved.
//  http://nkreipke.de
//

//  Version 1.5
//  SEE LICENSE FILE FOR LICENSE INFORMATION

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
//     - replaced string keys with kFMProcessInfo constants.
//     - changed method declarations containing an info dictionary to name it processInfo.
//     - fixed broken createNewFolder method in ARC version
//
// ** 1.5 (2012-10-25) by nkreipke
//     - FMServer:
//         + anonymousServerWithDestination:(NSURL*)
//     - added NSURL category FTPManagerNSURLAdditions
//     - the destination URL does NOT need the "ftp://" prefix anymore
//     - deleteFile:fromServer: is deprecated
//     - Methods added:
//         - deleteFileNamed:fromServer:
//         - chmodFileNamed:to:atServer:
//         - checkLogin:
//     - cleaned up the code a little
//
// ** 1.6 (2013-01-13) by nkreipke
//     - FMServer:
//         - FMServer.destination is now NSString! You will have to change this in your code.
//         - In FMSever.port the port can be specified. This is 21 by default.
//         - fixed a bug where variables were not retained properly
//     - fixed a bug where an empty file was created if downloadFile was not successful
//

// SCROLL DOWN TO SEE THE WELL COMMENTED PUBLIC METHODS. *****************

// LOOK AT FTPMSample1.m FOR AN EXAMPLE HOW TO USE THIS CLASS. ***********


#import <Foundation/Foundation.h>
#import <sys/socket.h>
#import <sys/types.h>
#import <netinet/in.h>
#import <netdb.h>

//FTPManager can log Socket answers if you got problems with
//deletion and chmod:
//#define FMSOCKET_VERBOSE

//these are used internally:
#define FTPANONYMOUS @"anonymous"
/*enum {
    kFTPAnswerSuccess = 200,
    kFTPAnswerLoggedIn = 230,
    kFTPAnswerFileActionOkay = 250,
    kFTPAnswerNeedsPassword = 331,
    kFTPAnswerNotAvailable = 421,
    kFTPAnswerNotLoggedIn = 530
};*/

@interface FMServer : NSObject {
@private
    NSString* destination;
    NSString* password;
    NSString* username;
    int port;
}
@property (strong) NSString* destination;
@property (strong) NSString* password;
@property (strong) NSString* username;
@property  (unsafe_unretained) int port;

+ (FMServer*) serverWithDestination:(NSString*)dest username:(NSString*)user password:(NSString*)pass;
+ (FMServer*) anonymousServerWithDestination:(NSString*)dest;

@end

@interface NSString (FTPManagerNSStringAdditions)
-(NSString*)stringWithoutProtocol;
-(NSURL*)ftpURLForPort:(int)port;
-(NSString*)fmhost;
-(NSString*)fmdir;
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
    _FMCurrentActionSOCKET,
    _FMCurrentActionNone
} _FMCurrentAction;

/* I do not recommend to use this delegate, because the methods will slow down
 * the process. On top of this they may have some threading issues that could
 * be pretty confusing. Use an NSTimer and [manager progress] instead. */
@protocol FTPManagerDelegate <NSObject>
@optional
- (void)ftpManagerUploadProgressDidChange:(NSDictionary *)processInfo;
// Returns information about the current upload.
// See "Process Info Dictionary Constants" below for detailed info.
- (void)ftpManagerDownloadProgressDidChange:(NSDictionary *)processInfo;
// Returns information about the current download.
// See "Process Info Dictionary Constants" below for detailed info.
@end

#pragma mark - Process Info Dictionary Constants

// Process Info Dictionary Constants (for [manager progress]): ******************
#define kFMProcessInfoProgress @"progress" // 0.0 to 1.0
#define kFMProcessInfoFileSize @"fileSize"
#define kFMProcessInfoBytesProcessed @"bytesProcessed"
#define kFMProcessInfoFileSizeProcessed @"fileSizeProcessed"
// ---------------------------------(returns NSNumber values)--------------------

#pragma mark -

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

#pragma mark - Public Methods

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
- (BOOL) deleteFileNamed:(NSString*)fileName fromServer:(FMServer*)server;
// -(BOOL) deleteFileNamed:fromServer:
// Deletes a file from a server. Also deletes directories if they are empty.
// Returns YES if the file was deleted.
- (BOOL) chmodFileNamed:(NSString*)fileName to:(int)mode atServer:(FMServer*)server;
// -(BOOL) chmodFileNamed:to:atServer:
// Changes the mode of a file on a server. Works only on UNIX servers.
// Returns YES if the chmod command was successful.
- (BOOL) checkLogin:(FMServer*)server;
// -(BOOL) checkLogin:
// Logs into the FTP server and logs out again. This can be used to check whether the credentials are
// correct before trying to do a file operation.
// Returns YES if the login was successful.
- (NSMutableDictionary *) progress;
// -(NSMutableDictionary *) progress
// Returns information about the current process. As the FTP methods hold the thread, you may
// want to call this method from a different thread that updates the UI.
// See 'Process Info Dictionary Constants' above for information about the contents of the
// dictionary.
// Returns nil if no process is currently running or information could not be determined. This
// method only works when downloading or uploading a file.
- (void) abort;
// -(void) abort
// Aborts the current process. As the FTP methods hold the thread, you may want to call this
// method from a different thread.





//deprecated:
- (BOOL) deleteFile:(NSString *)absolutePath fromServer:(FMServer *)server DEPRECATED_ATTRIBUTE;
// ** THIS METHOD IS DEPRECATED! Use deleteFileNamed:fromServer: instead. **
// -(BOOL) deleteFile:fromServer:
// Deletes a file or directory from a server. Use absolute path on server as path parameter!
// When trying to delete directories, make sure that the directory is empty.
// The URL must end with slash (/)!

@end
