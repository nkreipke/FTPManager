# FTPManager
***An Objective-C class for simple, synchronous ftp access.***

The methods in this class hold the thread - so you may want to call them in a background thread. The class was originally intended to upload a fixed list of files in one of my apps, but when I encountered that there was no good FTP access class that can be used in Mac and iOS apps except for the sample by Apple, I decided to publicize the class in the hope that it would be helpful for other developers.

### Information
Copy FTPManager.h and FTPManager.m into your project to use it in your own app. To make this class work, you have to link to **CoreServices.framework** (Mac) or **CFNetwork.framework** (iOS).

**Notice:** I strongly recommend creating a strong property for FTPManager. There is an issue that ARC may remove the instance too early if it is declared directly in the method.

### Creators
Created by [nkreipke](http://nkreipke.de "nkreipke") and [jweinert](http://www.csundm.de "csundm") (both links refer to German pages).
For questions, email git@nkreipke.de.

For license information, please refer to the license file in the FTPManager directory.

Feel free to fork and improve this project.
