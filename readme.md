# RealBot - by Stefan Hendriks
A server-side computer opponent (AI) for Counter-Strike 1.6

Install it as server admin, or on your own (listened) server to practice.


# How to get RealBot up and running (to play)

## Install Metamod
Download metamod from: http://www.metamod.org

Instructions are for Windows, but are almost the same for Linux/Mac. Look into [this guide for more information](https://nscodes.com/how-to-install-counter-strike-1-6-server-metamod-ammmodx-dproto/).

Assuming your `HL_HOME` is `Program Files\Steam\steamapps\common\Half-life\`

Go to `HL_HOME`:
- go to `cstrike` sub dir
- create dir `addons` (if already exists, skip this)
- go into `addons` dir
- create dir `metamod`, and go into it
- create dir `dlls`.

You should have something like:

`HL_HOME\cstrike\addons\metamod\dlls\`

- If you haven't yet, download the metamod DLL (windows binary), and unpack it into the `dlls` dir:

`HL_HOME\cstrike\addons\metamod\dlls\metamod.dll`

Within the `metamod` dir (not `dlls`!), create a file `plugins.ini` if it does not exist.

`HL_HOME\cstrike\addons\metamod\plugins.ini`

## Make sure Metamod is used by counter-strike
Go to `HL_HOME` and go into sub-dir `cstrike`.

Find, and open, the file `liblist.gam`. Then go to, the part saying `gamedll` and change it into:

```
gamedll "addons/metamod/dlls/metamod.dll"
```


## Install RealBot as metamod plugin
Simply download the latest RealBot version from Bots-United. Then, unpack it into the `HL_HOME` folder. This gives you:

`HL_HOME\realbot`, which is on the same level as `cstrike` and `valve`.

Then in the `plugins.ini` within the `metamod` folder (see above where), simply add this line:

```
win32 ../realbot/dll/realbot_mm.dll
```

# How to get RealBot to compile
Fairly easy. The required dependencies are delivered in the repo.

## dependencies
* Metamod 1.19
* Half-Life 1 SDK - Multiplayer

Both are delivered in the repo already.

## general steps
- `git clone` this project
- open up the `realbot_mm` visual studio solution
- make sure your `include paths` are pointing to the correct source folder. They should point into your realbot project folder; instead of some arbitrary path you may find. You can find any source of the dependencies in folder `dependencies`

Include directories needed are:
* hlsdk\multiplayer source\dlls
* hlsdk\multiplayer source\common
* hlsdk\multiplayer source\pm_shared
* hlsdk\multiplayer source\engine
* metamod 1.19\metamod

Be sure you have it in this order and have the Windows include directory be AFTER this order.

- make sure the post-build step (which copies realbot_mm.dll file into the Realbot folder as mentioned above) works - or remove it and copy by hand yourself