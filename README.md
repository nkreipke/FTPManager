# FTPManager

This library is obsolete and I recommend against using it in new projects. The repository remains for historical purposes and to provide a list of alternatives.

## Alternatives

- **[FTPKit](https://github.com/PeqNP/FTPKit):** An Objective-C based asynchronous FTP API which uses [ftplib](http://nbpfaus.net/~pfau/ftplib/). Supports uploading, downloading, listing, chmod and deletion. Unfortunately not available on CocoaPods.
- **[libcurl](https://curl.haxx.se/libcurl/c/):** A relatively simple and very stable C-style API for uploads and downloads only. Also supports FTPS. Instructions on how to build libcurl to use it in iOS and macOS projects can be found [here](https://github.com/biasedbit/curl-ios-build-scripts).
- **[Rebekka](https://github.com/Constantine-Fry/rebekka):** An asynchronous FTP/FTPS library written in Swift, which supports uploading, downloading and listing. Unfortunately it uses *CFFTPStream* (similarly to FTPManager), so it suffers from the same [deprecation problems](https://developer.apple.com/reference/coreservices/cfftpstream). Available on [CocoaPods](https://cocoapods.org/pods/rebekka).
- There is also **[BlackRaccoon](https://github.com/lloydsargent/BlackRaccoon)**, another Objective-C library which uses *CFFTPStream*. Same thing with [GoldRaccoon](https://github.com/albertodebortoli/GoldRaccoon) and [WhiteRaccoon](https://github.com/valentinradu/WhiteRaccoon) (still waiting for TurquoisRaccoon).
