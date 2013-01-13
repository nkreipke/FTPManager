//
//  FTPMSample1.m
//  FTPManager
//
//  Created by Nico Kreipke on 02.10.12.
//  Copyright (c) 2012 nkreipke. All rights reserved.
//


/* This is a very dirty sample.
 * Do not code like this.
 * However, this does not need anything in the header file. (Demonstration purposes, etc.)
 */

#import "FTPMSample1.h"
#import "FTPManager.h"

@implementation FTPMSample1

FMServer* server;
FTPManager* man;
NSString* filePath;
BOOL succeeded;
NSTimer* progTimer;

-(void)uploadFinished {
    [progTimer invalidate];
    progTimer = nil;
    filePath = nil;
    server = nil;
    man = nil;
    
    //test whether succeeded == YES
}

-(void)changeProgress {
    if (!man) {
        return;
    }
//    NSNumber* progress = [man.progress objectForKey:kFMProcessInfoProgress];
//    float p = progress.floatValue; //0.0f ≤ p ≤ 1.0f
    
    //use p here...
    //update some ui stuff, you know
}

-(void)startUploading {
    man = [[FTPManager alloc] init];
    
    succeeded = [man uploadFile:[NSURL URLWithString:filePath] toServer:server];
    
    [self performSelectorOnMainThread:@selector(uploadFinished) withObject:nil waitUntilDone:NO];
}

-(void)upload:(NSString*)file ftpUrl:(NSString*)url ftpUsr:(NSString*)user ftpPass:(NSString*)pass {
    server = [FMServer serverWithDestination:url username:user password:pass];
    filePath = file;
    progTimer = [NSTimer scheduledTimerWithTimeInterval:0.1 target:self selector:@selector(changeProgress) userInfo:nil repeats:YES];
    [self performSelectorInBackground:@selector(startUploading) withObject:nil];
}


/*
 
 usage:
 
 [self upload:@"/Users/sjobs/test.png" ftpUrl:@"apple.com" ftpPass:@"1234"];

*/

@end
