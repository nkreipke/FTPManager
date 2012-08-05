//
//  FTPManager.m
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

#import "FTPManager.h"

#define And(val1, val2) { val1 = val1 && val2; }
#define Check(val1) { if (val1 == NO) return NO; }

@interface FTPManager ()

@property (nonatomic, readonly) uint8_t *         buffer;
@property (nonatomic, assign)   size_t            bufferOffset;
@property (nonatomic, assign)   size_t            bufferLimit;
 
@end

@implementation FTPManager

@synthesize bufferOffset    = _bufferOffset;
@synthesize bufferLimit     = _bufferLimit;
@synthesize delegate        = _delegate;

- (uint8_t *)buffer
{
    return self->_buffer;
}

- (id)init
{
    self = [super init];
    if (self) {
        // Initialization code here.
    }
    
    return self;
}

- (BOOL) _checkFMServer:(FMServer*)server {
    BOOL success = YES;
    And(success, (server != nil));
    Check(success);
    And(success, (server.username != nil));
    And(success, (server.password != nil));
    return success;
}

- (unsigned long long) fileSizeOf:(NSURL*)file {
    NSDictionary* attrib = [[NSFileManager defaultManager] attributesOfItemAtPath:file.path error:nil];
    if (!attrib) {
        return 0;
    }
    return [attrib fileSize];
}

- (NSArray*) _createListingArrayFromDirectoryListingData:(NSMutableData*)data {
    NSMutableArray* listingArray = [NSMutableArray array];
    
    NSUInteger offset = 0;
    do {
        CFIndex bytesConsumed;
        CFDictionaryRef thisEntry = NULL;
        bytesConsumed = CFFTPCreateParsedResourceListing(NULL, &((const uint8_t *) directoryListingData.bytes)[offset],
                                                         directoryListingData.length - offset, &thisEntry);
        if (bytesConsumed > 0) {
            if (thisEntry != NULL) {
                [listingArray addObject:(__bridge NSDictionary*)thisEntry];
            }
            offset += bytesConsumed;
        }
        
        if (thisEntry != NULL) {
            CFRelease(thisEntry);
        }
        
        if (bytesConsumed == 0) {
            break;
        } else if (bytesConsumed < 0) {
            return nil;
        }
    } while (YES);
    
    return listingArray;
}

- (BOOL) _uploadData:(NSData *)data withFileName:(NSString *)fileName toServer:(FMServer *)server {
    BOOL success = YES;
    
    action = _FMCurrentActionUploadFile;
    
    fileSize = data.length;
    fileSizeProcessed = 0;
    
    NSURL * finalURL = [server.destination URLByAppendingPathComponent:fileName];    
    And(success, (finalURL != nil));
    Check(success);
    
    fileReader = [[NSInputStream alloc] initWithData:data];
    And(success, (fileReader != nil));
    Check(success);
    [fileReader open];
    
    CFWriteStreamRef writeStream = CFWriteStreamCreateWithFTPURL(NULL, (__bridge CFURLRef)finalURL);
    And(success, (writeStream != NULL));
    Check(success);
    serverStream = (__bridge NSOutputStream*) writeStream;
    
    And(success, [serverStream setProperty:server.username forKey:(id)kCFStreamPropertyFTPUserName]);
    And(success, [serverStream setProperty:server.password forKey:(id)kCFStreamPropertyFTPPassword]);
    Check(success);
    
    self.bufferOffset = 0;
    self.bufferLimit = 0;
    
    currentRunLoop = CFRunLoopGetCurrent();
    
    serverStream.delegate = self;
    [serverStream scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    [serverStream open];
    
    CFRunLoopRun();
    
    And(success, streamSuccess);
    
    return success;
}

- (BOOL) _uploadFile:(NSURL*)fileURL toServer:(FMServer*)server {
    BOOL success = YES;
    
    action = _FMCurrentActionUploadFile;
    
    fileSize = [self fileSizeOf:fileURL];
    fileSizeProcessed = 0;
    
    NSURL * finalURL = [server.destination URLByAppendingPathComponent:[fileURL lastPathComponent]];    
    And(success, (finalURL != nil));
    Check(success);
    
    fileReader = [[NSInputStream alloc] initWithFileAtPath:fileURL.path];
    And(success, (fileReader != nil));
    Check(success);
    [fileReader open];
    
    CFWriteStreamRef writeStream = CFWriteStreamCreateWithFTPURL(NULL, (__bridge CFURLRef)finalURL);
    And(success, (writeStream != NULL));
    Check(success);
    serverStream = (__bridge NSOutputStream*) writeStream;
    
    And(success, [serverStream setProperty:server.username forKey:(id)kCFStreamPropertyFTPUserName]);
    And(success, [serverStream setProperty:server.password forKey:(id)kCFStreamPropertyFTPPassword]);
    Check(success);
    
    self.bufferOffset = 0;
    self.bufferLimit = 0;
    
    currentRunLoop = CFRunLoopGetCurrent();
    
    serverStream.delegate = self;
    [serverStream scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    [serverStream open];
    
    CFRunLoopRun();
    
    And(success, streamSuccess);
    
    return success;
}
- (BOOL) _createNewFolder:(NSString*)folderName atServer:(FMServer*)server {
    BOOL success = YES;
    
    action = _FMCurrentActionCreateNewFolder;
    
    fileSize = 0;
    
    NSURL * finalURL = [server.destination URLByAppendingPathComponent:folderName isDirectory:YES];
    And(success, (finalURL != nil));
    Check(success);
    
    CFWriteStreamRef writeStream = CFWriteStreamCreateWithFTPURL(NULL, (__bridge CFURLRef)finalURL);
    And(success, (writeStream != NULL));
    Check(success);
    serverStream = (__bridge NSOutputStream*) writeStream;
    
    And(success, [serverStream setProperty:server.username forKey:(id)kCFStreamPropertyFTPUserName]);
    And(success, [serverStream setProperty:server.password forKey:(id)kCFStreamPropertyFTPPassword]);
    Check(success);
    
    self.bufferOffset = 0;
    self.bufferLimit = 0;
    
    currentRunLoop = CFRunLoopGetCurrent();
    
    serverStream.delegate = self;
    [serverStream scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    [serverStream open];
    
    CFRunLoopRun();
    
    And(success, streamSuccess);
    
    return success;
}

- (BOOL) _deleteFile:(NSString *)absolutePath fromServer:(FMServer *)server
{
    BOOL success = YES;
    
    action = _FMCurrentActionCreateNewFolder;
    
    fileSize = 0;
    
    if (![absolutePath hasSuffix:@"/"]) {
        //if the path does not end with an '/' the method fails.
        //no problem, we can fix this.
        absolutePath = [NSString stringWithFormat:@"%@/",absolutePath];
    }
    
    NSURL *fileURL = [[server destination] URLByAppendingPathComponent:absolutePath];
    NSString *unProtocolledString = [[[fileURL absoluteString] componentsSeparatedByString:@"ftp://"] objectAtIndex:1];
    NSString *authenticatedString = [NSString stringWithFormat:@"ftp://%@:%@@%@", server.username, server.password, unProtocolledString];
    
    And(success, (absolutePath != nil));
    Check(success);
    
    And(success, CFURLDestroyResource((__bridge CFURLRef)[NSURL URLWithString:authenticatedString], NULL));
    Check(success);
    
    return success;
}

- (NSArray*) _contentsOfServer:(FMServer*)server {
    BOOL success = YES;
    
    action = _FMCurrentActionContentsOfServer;
    
    fileSize = 0;
    
    directoryListingData = [[NSMutableData alloc] init];
    
    if (![server.destination.absoluteString hasSuffix:@"/"]) {
        //if the url does not end with an '/' the method fails.
        //no problem, we can fix this.
        server.destination = [NSURL URLWithString:[NSString stringWithFormat:@"%@/",server.destination.absoluteString]];
    }
    
    CFReadStreamRef readStream = CFReadStreamCreateWithFTPURL(NULL, (__bridge CFURLRef)server.destination);
    And(success, (readStream != NULL));
    if (!success) return nil;
    serverReadStream = (__bridge NSInputStream*) readStream;
    
    And(success, [serverReadStream setProperty:server.username forKey:(id)kCFStreamPropertyFTPUserName]);
    And(success, [serverReadStream setProperty:server.password forKey:(id)kCFStreamPropertyFTPPassword]);
    if (!success) return nil;
    
    self.bufferOffset = 0;
    self.bufferLimit = 0;
    
    currentRunLoop = CFRunLoopGetCurrent();
    
    serverReadStream.delegate = self;
    [serverReadStream scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    [serverReadStream open];
    
    CFRunLoopRun();
    
    And(success, streamSuccess);
    if (!success) return nil;
    
    NSArray* directoryContents = [self _createListingArrayFromDirectoryListingData:directoryListingData];
    directoryListingData = nil;
    
    return directoryContents;
}
- (BOOL) _downloadFile:(NSString*)fileName toDirectory:(NSURL*)directoryURL fromServer:(FMServer*)server {
    BOOL success = YES;
    
    action = _FMCurrentActionDownloadFile;
    
    fileSize = 0;
    fileSizeProcessed = 0;
    
    fileWriter = [[NSOutputStream alloc] initToFileAtPath:[directoryURL URLByAppendingPathComponent:fileName].path append:NO];
    And(success, (fileWriter != nil));
    Check(success);
    [fileWriter open];
    
    CFReadStreamRef readStream = CFReadStreamCreateWithFTPURL(NULL, (__bridge CFURLRef)[server.destination URLByAppendingPathComponent:fileName]);
    And(success, (readStream != NULL));
    Check(success);
    serverReadStream = (__bridge NSInputStream*) readStream;
    
    And(success, [serverReadStream setProperty:server.username forKey:(id)kCFStreamPropertyFTPUserName]);
    And(success, [serverReadStream setProperty:server.password forKey:(id)kCFStreamPropertyFTPPassword]);
    And(success, [serverReadStream setProperty:[NSNumber numberWithBool:YES] forKey:(id)kCFStreamPropertyFTPFetchResourceInfo]);
    Check(success);
    
    self.bufferOffset = 0;
    self.bufferLimit = 0;
    
    currentRunLoop = CFRunLoopGetCurrent();
    
    serverReadStream.delegate = self;
    [serverReadStream scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    [serverReadStream open];
    
    CFRunLoopRun();
    
    And(success, streamSuccess);
    
    return success;
}

- (BOOL) uploadData:(NSData*)data withFileName:(NSString *)fileName toServer:(FMServer*)server {
    if (![self _checkFMServer:server]) {
        return NO;
    }
    if (!data) {
        return NO;
    }
    return [self _uploadData:data withFileName:fileName toServer:server];
}

- (BOOL) uploadFile:(NSURL*)fileURL toServer:(FMServer*)server {
    if (![self _checkFMServer:server]) {
        return NO;
    }
    if (!fileURL) {
        return NO;
    }
    BOOL isDir;
    if (![[NSFileManager defaultManager] fileExistsAtPath:fileURL.path isDirectory:&isDir] || isDir) {
        return NO;
    }
    return [self _uploadFile:fileURL toServer:server];
}
- (BOOL) createNewFolder:(NSString*)folderName atServer:(FMServer*)server {
    if (![self _checkFMServer:server]) {
        return NO;
    }
    if (!folderName) {
        return NO;
    }
    if ([folderName isEqualToString:@""]) {
        return NO;
    }
    return [self _createNewFolder:folderName atServer:server];
}
- (BOOL) deleteFile:(NSString *)absolutePath fromServer:(FMServer *)server
{
    if (![self _checkFMServer:server]) {
        return NO;
    }
    if (!absolutePath) {
        return NO;
    }
    return [self _deleteFile:absolutePath fromServer:server];
}
- (NSArray*) contentsOfServer:(FMServer*)server {
    if (![self _checkFMServer:server]) {
        return nil;
    }
    return [self _contentsOfServer:server];
}
- (BOOL) downloadFile:(NSString*)fileName toDirectory:(NSURL*)directoryURL fromServer:(FMServer*)server {
    if (![self _checkFMServer:server]) {
        return NO;
    }
    if (!fileName) {
        return NO;
    }
    if ([fileName isEqualToString:@""]) {
        return NO;
    }
    if (!directoryURL) {
        return NO;
    }
    if (![[NSFileManager defaultManager] fileExistsAtPath:directoryURL.path]) {
        return NO;
    }
    return [self _downloadFile:fileName toDirectory:directoryURL fromServer:server];
}
- (NSMutableDictionary *) progress {
    //this does only work with uploadFile and downloadFile.
    NSStream* currentStream;
    switch (action) {
        case _FMCurrentActionUploadFile:
            currentStream = serverStream;
            break;
        case _FMCurrentActionDownloadFile:
            currentStream = serverReadStream;
            break;
        default:
            break;
    }
    
    if (!currentStream || fileSize == 0) {
        return nil;
    }
    
    NSMutableDictionary *returnValues = [[NSMutableDictionary alloc] init];
    
    [returnValues setValue:[NSNumber numberWithUnsignedLongLong:fileSize] forKey:kFMProcessInfoFileSize];
    [returnValues setValue:[NSNumber numberWithUnsignedLongLong:bytesProcessed] forKey:kFMProcessInfoBytesProcessed];
    [returnValues setValue:[NSNumber numberWithUnsignedLongLong:fileSizeProcessed] forKey:kFMProcessInfoFileSizeProcessed];
    [returnValues setValue:[NSNumber numberWithFloat:(float)fileSizeProcessed / (float)fileSize] forKey:kFMProcessInfoProgress];
    
    return returnValues;
}

- (void) _streamDidEndWithSuccess:(BOOL)success failureReason:(FMStreamFailureReason)failureReason {
    action = _FMCurrentActionNone;
    streamSuccess = success;
    if (!streamSuccess) {
        switch (failureReason) {
            case FMStreamFailureReasonReadError:
                NSLog(@"ftp stream failed: error while reading data");
                break;
            case FMStreamFailureReasonWriteError:
                NSLog(@"ftp stream failed: error while writing data");
                break;
            case FMStreamFailureReasonGeneralError:
                NSLog(@"ftp stream failed: general stream error (check credentials?)");
                break;
            default:
                break;
        }
    }
    if (serverStream) {
        [serverStream removeFromRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
        serverStream.delegate = nil;
        [serverStream close];
        serverStream = nil;
    }
    if (serverReadStream) {
        [serverReadStream removeFromRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
        serverReadStream.delegate = nil;
        [serverReadStream close];
        serverReadStream = nil;
    }
    if (fileReader) {
        [fileReader close];
        fileReader = nil;
    }
    if (fileWriter) {
        [fileWriter close];
        fileWriter = nil;
    }
    CFRunLoopStop(currentRunLoop);
}

- (void)stream:(NSStream *)theStream handleEvent:(NSStreamEvent)streamEvent {
    switch (streamEvent) {
        case NSStreamEventOpenCompleted:
            if (action == _FMCurrentActionDownloadFile) {
                fileSize = [[theStream propertyForKey:(id)kCFStreamPropertyFTPResourceSize] longLongValue];
                
                if (self.delegate && [self.delegate respondsToSelector:@selector(ftpManagerDownloadProgressDidChange:)]) {
                    [self.delegate ftpManagerDownloadProgressDidChange:[self progress]];
                }
            }
            break;
        case NSStreamEventHasBytesAvailable:
            if (action == _FMCurrentActionContentsOfServer) {
                NSInteger       bytesRead;
                
                bytesRead = [serverReadStream read:self.buffer maxLength:kSendBufferSize];
                if (bytesRead == -1) {
                    [self _streamDidEndWithSuccess:NO failureReason:FMStreamFailureReasonReadError];
                } else if (bytesRead == 0) {
                    [self _streamDidEndWithSuccess:YES failureReason:FMStreamFailureReasonNone];
                } else {
                    [directoryListingData appendBytes:self.buffer length:bytesRead];
                }
            } else if (action == _FMCurrentActionDownloadFile) {
                if (self.bufferOffset == self.bufferLimit) {
                    //fill buffer with data from server
                    NSInteger   bytesRead;
                    
                    bytesRead = [serverReadStream read:self.buffer maxLength:kSendBufferSize];
                    
                    if (bytesRead == -1) {
                        [self _streamDidEndWithSuccess:NO failureReason:FMStreamFailureReasonReadError];
                    } else if (bytesRead == 0) {
                        [self _streamDidEndWithSuccess:YES failureReason:FMStreamFailureReasonNone];
                    } else {
                        self.bufferOffset = 0;
                        self.bufferLimit  = bytesRead;
                        fileSizeProcessed += bytesRead;
                        bytesProcessed = bytesRead;
                        
                        if (self.delegate && [self.delegate respondsToSelector:@selector(ftpManagerDownloadProgressDidChange:)]) {
                            [self.delegate ftpManagerDownloadProgressDidChange:[self progress]];
                        }
                    }
                }
                if (self.bufferOffset != self.bufferLimit) {
                    //fill file with buffer
                    NSInteger   bytesWritten;
                    bytesWritten = [fileWriter write:&self.buffer[self.bufferOffset] maxLength:self.bufferLimit - self.bufferOffset];
                    if (bytesWritten == -1 || bytesWritten == 0) {
                        [self _streamDidEndWithSuccess:NO failureReason:FMStreamFailureReasonWriteError];
                    } else {
                        self.bufferOffset += bytesWritten;
                    }
                }
            } else {
                //something went wrong here...
                [self _streamDidEndWithSuccess:NO failureReason:FMStreamFailureReasonGeneralError];
            }
            break;
        case NSStreamEventHasSpaceAvailable:
            if (action == _FMCurrentActionUploadFile) {
                if (self.bufferOffset == self.bufferLimit) {
                    //read process
                    //fill buffer with data
                    NSInteger   bytesRead;
                    
                    bytesRead = [fileReader read:self.buffer maxLength:kSendBufferSize];
                    
                    if (bytesRead == -1) {
                        [self _streamDidEndWithSuccess:NO failureReason:FMStreamFailureReasonReadError];
                    } else if (bytesRead == 0) {
                        [self _streamDidEndWithSuccess:YES failureReason:FMStreamFailureReasonNone];
                    } else {
                        self.bufferOffset = 0;
                        self.bufferLimit  = bytesRead;
                    }
                }
                
                if (self.bufferOffset != self.bufferLimit) {
                    //write process
                    //write data out of buffer to server
                    NSInteger   bytesWritten;
                    bytesWritten = [serverStream write:&self.buffer[self.bufferOffset] maxLength:self.bufferLimit - self.bufferOffset];
                    if (bytesWritten == -1 || bytesWritten == 0) {
                        [self _streamDidEndWithSuccess:NO failureReason:FMStreamFailureReasonWriteError];
                    } else {
                        self.bufferOffset += bytesWritten;
                        fileSizeProcessed += bytesWritten;
                        bytesProcessed = bytesWritten;
                        
                        if (self.delegate && [self.delegate respondsToSelector:@selector(ftpManagerUploadProgressDidChange:)]) {
                            [self.delegate ftpManagerUploadProgressDidChange:[self progress]];
                        }
                    }
                }
            } else {
                //something went wrong here...
                [self _streamDidEndWithSuccess:NO failureReason:FMStreamFailureReasonGeneralError];
            }
            break;
        case NSStreamEventErrorOccurred:
            [self _streamDidEndWithSuccess:NO failureReason:FMStreamFailureReasonGeneralError];
            break;
        case NSStreamEventEndEncountered:
            if (action == _FMCurrentActionCreateNewFolder) {
                [self _streamDidEndWithSuccess:YES failureReason:FMStreamFailureReasonNone];
            }
            break;
        default:
            break;
    }
}

-(void)abort {
    NSStream* currentStream;
    switch (action) {
        case _FMCurrentActionUploadFile:
            currentStream = serverStream;
            break;
        case _FMCurrentActionDownloadFile:
            currentStream = serverReadStream;
            break;
        case _FMCurrentActionCreateNewFolder:
            currentStream = serverStream;
            break;
        case _FMCurrentActionContentsOfServer:
            currentStream = serverReadStream;
            break;
        default:
            break;
    }
    if (!currentStream) {
        return;
    }
    [currentStream close];
    [self _streamDidEndWithSuccess:YES failureReason:FMStreamFailureReasonAborted];
}


@end


@implementation FMServer
@synthesize password, username, destination;
+(FMServer*)serverWithDestination:(NSURL*)dest username:(NSString*)user password:(NSString*)pass {
    FMServer* server = [[FMServer alloc] init];
    server.destination = dest;
    server.username = user;
    server.password = pass;
    return server;
}
@end