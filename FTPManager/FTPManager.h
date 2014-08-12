//
//  FTPManager.m
//  FTPManager
//
//  Created by Nico Kreipke on 11.08.11.
//  Copyright (c) 2014 nkreipke. All rights reserved.
//  http://nkreipke.de
//

//  Version 1.6.5
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
//         - fixed a bug where variables were not retained properly
//         - FMServer.destination is now NSString! You will have to change that in your code.
//         - In FMSever.port the port can be specified. This is 21 by default.
//     - fixed a bug where an empty file was created if downloadFile was not successful
//
// ** (1.6.1 -> release for CocoaPods)
//
// ** 1.6.2 (2013-04-24) by nkreipke
//     - fixed a bug that occured in iOS 6 (https://github.com/nkreipke/FTPManager/issues/5)
//     - fixed garbage value bug
//
// ** 1.6.3 (2014-01-25) by nkreipke
//     - fixed a memory leak
//
// ** 1.6.4 (2014-06-13) by nkreipke
//     - fixed crash that can occur when scheduling the stream (thanks to Kevin Paunovic)
//     - fixed race condition bug while aborting
//     - a separate NSThread is now used instead of using whatever thread FTPManager was called on
//     - fixed bug that prevented subdirectories from being accessed when port was not 21
//     - fixed bug that prevented subdirectories with names containing spaces from being accessed
//
// ** 1.6.5 (2014-08-12) by nkreipke
//     - kCFFTPResourceName entry is now converted into UTF8 encoding to cope with Non-ASCII characters
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

/**
 *  The URL of the FMServer.
 */
@property (strong) NSString* destination;

/**
 *  The password for the FMServer login.
 */
@property (strong) NSString* password;

/**
 *  The username for the FMServer login.
 */
@property (strong) NSString* username;

/**
 *  The port which is used for the connection.
 */
@property  (unsafe_unretained) int port;

/**
 *  Returns a FMServer initialized with the given URL and credentials.
 *
 *  @param dest The URL of the FTP server.
 *  @param user The username of the account which will be used to log in.
 *  @param pass The password which will be used to log in.
 *
 *  @return A FMServer object with the given URL, username and password.
 */
+ (FMServer*) serverWithDestination:(NSString*)dest username:(NSString*)user password:(NSString*)pass;

/**
 *  Returns a FMServer initialized with the given URL and anonymous login.
 *
 *  @param dest The URL of the FTP server.
 *
 *  @return A FMServer object with the given URL and anonymous login.
 */
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
    CFRunLoopRef currentRunLoop;
    
    _FMCurrentAction action;
    
    uint8_t _buffer[kSendBufferSize];
    size_t _bufferOffset;
    size_t _bufferLimit;
    
    unsigned long long fileSize;
    unsigned long long bytesProcessed;
    unsigned long long fileSizeProcessed;
    
    BOOL streamSuccess;
}

/**
 *  Input steam for reading from a local file
 */
@property (strong) NSInputStream *fileReader;

/**
 *  Output stream for writing to a local file
 */
@property (strong) NSOutputStream *fileWriter;

/**
 *  Input stream for reading from the server (remote file)
 */
@property (strong) NSInputStream *serverReadStream;

/**
 *  Output stream for writing to the server (remote file)
 */
@property (strong) NSOutputStream *serverStream;

@property (strong) NSMutableData *directoryListingData;


@property (assign) id<FTPManagerDelegate>delegate;

#pragma mark - Public Methods

// *** Information
// These methods hold the current thread. You will get an answer with a success information.

/**
 *  Uploads a file to a server. Existing remote files of the same name will be overwritten.
 *
 *  @param fileURL The local file which will be uploaded to the FTP server.
 *  @param server  The FTP server which the file will be uploaded to.
 *
 *  @return YES if the upload was successful, NO otherwise.
 */
- (BOOL) uploadFile:(NSURL*)fileURL toServer:(FMServer*)server;

/**
 *  Uploads NSData to a server. Existing remote files of the same name will be overwritten.
 *
 *  @param data     The data which will be written to the FTP server.
 *  @param fileName The name with which the new file will be created on the FTP server.
 *  @param server   The FTP server on which the file with the given data will be created.
 *
 *  @return YES if the upload was successful, NO otherwise.
 */
- (BOOL) uploadData:(NSData*)data withFileName:(NSString *)fileName toServer:(FMServer*)server;

/**
 *  Creates a new folder on the specified FTP server.
 *
 *  @param folderName The name of the folder to create.
 *  @param server     The FTP server on which the new folder should be created.
 *
 *  @return YES if the folder creation was successful, NO otherwise.
 */
- (BOOL) createNewFolder:(NSString*)folderName atServer:(FMServer*)server;

/**
 *  Returns a list of files and folders at the specified FTP server. as an NSArray containing instances of NSDictionary.
 *  The dictionaries contain objects declared in CFStream FTP Resource Constants. To get the name of the entry, get the object for the (id)kCFFTPResourceName key.
 *
 *  @param server The FTP server whose contents will be listed.
 *
 *  @return The NSArray containing instances of NSDictionary. An empty array if the server has no contents. nil if there was an error during the process.
 */
- (NSArray*) contentsOfServer:(FMServer*)server;

/**
 *  Downloads a file from the specified FTP server. Existing local files of the same name will be overwritten.
 *
 *  @param fileName     The file which will be downloaded from the specified FTP server.
 *  @param directoryURL The local directory the file will be downloaded to.
 *  @param server       The server the file will be downloaded from.
 *
 *  @return YES if the download was successful, NO otherwise
 */
- (BOOL) downloadFile:(NSString*)fileName toDirectory:(NSURL*)directoryURL fromServer:(FMServer*)server;

/**
 *  Delete a file from the specified FTP server and delete directories if they are empty.
 *
 *  @param fileName The file which will be deleted from the FTP server.
 *  @param server   The FTP server from which the file or directory will be deleted.
 *
 *  @return YES if the file was successfully deleted from the server, NO otherwise.
 */
- (BOOL) deleteFileNamed:(NSString*)fileName fromServer:(FMServer*)server;

/**
 *  Changes the mode of a file on a server. Works only on UNIX servers.
 *
 *  @param fileName The file whose permissions will be modified.
 *  @param mode     The mode which will be applied to the remote file in octal notation.
 *  @param server   The server on which the mode change operation will take place.
 *
 *  @return YES if the chmod command was successful, NO otherwise.
 */
- (BOOL) chmodFileNamed:(NSString*)fileName to:(int)mode atServer:(FMServer*)server;

/**
 *  Logs into the FTP server and logs out again. This can be used to check whether the credentials are correct before trying to do a file operation.
 *
 *  @param server The FMServer FTP object to log into.
 *
 *  @return YES if the login was successful, NO otherwise.
 */
- (BOOL) checkLogin:(FMServer*)server;

/**
 *  Returns information about the current process. As the FTP methods hold the thread, you may want to call this method from a different thread that updates the UI.
 *  See 'Process Info Dictionary Constants' above for information about the contents of the dictionary.
 *
 *  @return nil if no process is currently running or information could not be determined. This method only works when downloading or uploading a file.
 */
- (NSMutableDictionary *) progress;

/**
 *  Aborts the current process. As the FTP methods hold the thread, you may want to call this method from a different thread.
 */
- (void) abort;





//deprecated:
/**
 *  Deletes a file or directory from the specified FTP server. Uses absolute path on server as path parameter.
 *  DEPRECATED: Use deleteFileNamed:fromServer: instead.
 *
 *  @param absolutePath The absolute path to the file which will be deleted on the server. The URL must end with a slash ("/").
 *  @param server       The server from which the file will be deleted.
 *
 *  @return YES if the file was successfully deleted, NO otherwise.
 */
- (BOOL) deleteFile:(NSString *)absolutePath fromServer:(FMServer *)server DEPRECATED_ATTRIBUTE;

@end
