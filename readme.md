Computer opponent for Counter-Strike.

Needs metamod to operate: http://www.metamod.org

Short description how to get this thing compiling:

in order to compile it, be sure you have :

metamod 1.19 half-lifde 1 sdk - multiplayer

include directories needed are: hlsdk\multiplayer source\dlls hlsdk\multiplayer source\common hlsdk\multiplayer source\pm_shared hlsdk\multiplayer source\engine metamod 1.19\metamod

Be sure you have it in this order and have the Windows include directory be AFTER this order.

The repository contains the above metamod and hlsdk dependencies, so you do not have to worry about the correct versions and such. Just use SVN to get the source, open the SLN file and compile. Be sure you look into the gif file, in the dependencies directory so you know what directories you need.
