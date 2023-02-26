# ZetaLoader

A modification to fix technical issues with Halo Infinite on PC.

## Fixes

1. External Framerate Limiter Game Window Thread Mitigation.

    > References: 
    > 1. https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling-priorities  
    > 2. https://forums.guru3d.com/threads/msi-ab-rtss-development-news-thread.412822/page-161#post-5949434
    > 3. https://www.reddit.com/r/halo/comments/puq7ks/psa_rtss_causes_excessive_screen_tearing_in_halo/

    This fix simply resolves the following when External Framerate Limiter is being used:
    1. Intense Screen Tearing.
    2. Jittery/Stuttery Mouse Input.    
    **Note: Using high process priority with Halo Infinite, makes this mitigation useless!**  

    Setting the window thread priority to `THREAD_PRIORITY_HIGHEST` resolves the mentioned issues by giving the window thread enough of a timeslice.    

    > ZetaLoader sets the window thread priority to `THREAD_PRIORITY_TIME_CRITICAL` just to assign a higher priority over threads that use `THREAD_PRIORITY_HIGHEST`.    

    You can verify if this works by using a lower thread priority set it using the function `SetThreadPriority` in the source code or setting the game's process priority to `High` via Task Manager.   

2. Borderless Fullscreen

    > **Available with Borderless Fullscreen only at game startup.**

    ZetaLoader overrides Halo Infinite's borderless fullscreen/window style.            
    Halo Infinite uses the following styles `WS_VISIBLE | WS_OVERLAPPED | WS_CLIPSIBLINGS` when it is borderless fullscreen.  
    
    The window style `WS_OVERLAPPED` doesn't fill the screen area correctly if the display mode is below `1440x810` resulting in the window to extend its client area beyond the screen area itself. 

    This is simply fixed by using the following styles:      
    - Window Styles: `WS_VISIBLE | WS_POPUP` 
    - Extended Window Styles: `WS_EX_APPWINDOW`
    - Topmost Window: `HWND_TOPMOST`

## Features   
1. User Specified Display Mode Support

    > **Available with Borderless Fullscreen only at game startup.**   

    This feature restores the game's ability to run at any user specified display mode.                
    Here is what this feature emulates/restores:              
    - Automatically Minimize, when the game is not the foreground window.
    - Dynamically switch between your desired game display mode & native display mode like in exclusive fullscreen.

2. 0.5 Timer Resolution

    Using the hidden `NtSetTimerResolution` function, Halo Infinite can use a minimum timer resolution of 0.5 ms.
    >  Halo Infinite uses 1 ms by default, we can force 0.5 ms using NtSetTimerResolution.        
    Starting with Windows 2004, setting the timer resolution is no longer global but on a per process basis.       
    Reference: https://learn.microsoft.com/en-us/windows/win32/api/timeapi/nf-timeapi-timebeginperiod#remarks    

3.  Multimedia Class Schedule Service Scheduling

    Using the function `DwmEnableMMCSS` makes the calling process opt in for multimedia class schedule service scheduling which boosts performance in non-fullscreen applications.  
    > References:
    > 1. https://github.com/djdallmann/GamingPCSetup/blob/master/CONTENT/RESEARCH/WINSERVICES/README.md#multimedia-class-scheduler-service-mmcss
    > 2. https://www.overclock.net/threads/if-you-play-non-fullscreen-exclusive-games-you-might-get-a-boost-in-performance-with-dwmenablemmcss.1775433/
    > 3. https://learn.microsoft.com/en-us/windows/win32/procthread/multimedia-class-scheduler-service

4. Above Normal Process Priority      

    This feature makes Halo Infinite automatically have `Above Normal Process Priority`, this will make Windows give Halo Infinite more processor time whenever possible.

5. User Specified DLLs Support             
    This feature allows one to load any 3rd party DLL into Halo Infinite.

## Result
> Specifications:     
    1. i7-10700K @ 4.8 GHz HT Disabled @ 1.15V   
    2. GTX 1650 @ 2055 ~ 2070 MHz (135 MHz+ Core) & 7200 MHz (1200 MHz+ RAM) OCs   
    3. 2666 MHz RAM   

**Note: My minimum framerate is set to 960 FPS in the linked video.**      
Result Video: https://www.youtube.com/watch?v=o9u0oAyv3dc

## Command Line Options
- To configure ZetaLoader, modify the game's launch options.     

    | Argument| Description |
    |-|-|
    |`-width` `<width>`|Sets the display mode's width.<br>**📝 Borderless Fullscreen must be enabled.**|
    |`-height` `<height>`|Sets the display mode's height.<br>**📝 Borderless Fullscreen must be enabled.**|
    |`-refresh` `<rate>`|Sets the display mode's refresh rate.<br>**📝 Borderless Fullscreen must be enabled.**|
    |`-dll` `<name.dll>`|Loads the specified DLL.|

## Installation

### Automatic

You can the use the following command to automatically install ZetaLoader.    
**[View Installer Script Source Code.](https://github.com/couleur-tweak-tips/TweakList/blob/master/modules/Installers/Install-ZetaLoader.ps1)**           
 
1. Paste the following command into PowerShell.

    ```ps
    irm "tl.ctt.cx" | iex; Get ZetaLoader
    ```
2. Launch Halo Infinite.

### Manual
1. Download `dinput8.dll` from [GitHub Releases](https://github.com/Aetopia/ZetaLoader).   

2. Browse Halo Infinite's local files.  

    ![Properties](https://i.imgur.com/8HKvH4U.png)

3. Place `dinput8.dll` into the game's installation directory. 

    ![ZetaLoader Installation](https://i.imgur.com/Xu1hNuN.png)

## Uninstallation
Delete `dinput8.dll` from Halo Infinite's installation directory.

# Building
1. Install `GCC`.   
    > **Make sure it is in the system path!**

2. Run `build.bat`.
    > Optional UPX compression is also performed if `upx.exe` is in the system path.

# Limitations

**Here are a few things you should note about ZetaLoader.**

1. Fixes specific techincal issues with the Halo Infinite.
2. Designed for the Steam release of Halo Infinite.
3. Might work with the UWP release or running under a Windows Compatibility Layer on Linux but the source code might need to be rewritten for these platforms.
