# ZetaLoader

A utility to fix technical issues with Halo Infinite on PC.

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
    Halo Infinite uses the following styles `WS_VISIBLE | WS_OVERLAPPED | WS_CLIPSIBLINGS` when going borderless fullscreen.  
    
    The window style `WS_OVERLAPPED` doesn't fill the screen area correctly if the display mode is below `1440x810` resulting in the window to extend its client area beyond the screen area itself. 

    This is simply fixed by using the following styles:      
    - Window Styles: `WS_VISIBLE | WS_POPUP` 
    - Extended Window Styles: `WS_EX_APPWINDOW`
    - Topmost Window: `HWND_TOPMOST`

## Features   
1. User Defined Display Mode Support

    > **Available with Borderless Fullscreen only at game startup.**   

    This feature restores the game's ability to run at any user defined display mode. Here is what this feature emulates/restores:
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
    1. https://github.com/djdallmann/GamingPCSetup/blob/master/CONTENT/RESEARCH/WINSERVICES/README.md#multimedia-class-scheduler-service-mmcss
    2. https://www.overclock.net/threads/if-you-play-non-fullscreen-exclusive-games-you-might-get-a-boost-in-performance-with-dwmenablemmcss.1775433/

4. Above Normal Process Priority      

    This feature makes Halo Infinite automatically have `Above Normal Process Priority`, this will make Windows give Halo Infinite more processor time whenever possible.

## Result
> Specifications:     
    1. i7-10700K @ 4.8 GHz HT Disabled @ 1.15V   
    2. GTX 1650 @ 2055 ~ 2070 MHz (135 MHz+ Core) & 7200 MHz (1200 MHz+ RAM) OCs   
    3. 3467 MHz RAM (**But its mismatched!**)     

**Note: My minimum framerate is set to 960 FPS in the linked video.**      
Result Video: https://www.youtube.com/watch?v=o9u0oAyv3dc

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
1. Download `wininet.dll` from [GitHub Releases](https://github.com/Aetopia/ZetaLoader).   

2. Browse Halo Infinite's local files.  

    ![Properties](https://i.imgur.com/8HKvH4U.png)

3. Place `wininet.dll` into the game's installation directory. 

    ![ZetaLoader Installation](https://i.imgur.com/r3nGbx0.png)

### Configure
![Launch Options](https://i.imgur.com/Q4r4zTz.png)

- To configure ZetaLoader, modify the game's launch options.     

    - To launch Halo Infinite using a specified display mode.
        Using the following launch option:
        ```
        --DisplayMode: WxH_R
        ```
        - `W`: Width of the display mode.
        - `H`: Height of the display mode.
        - `R`: Refresh rate of the display mode. 

    Also ensure **Borderless Fullscreen is enabled**.

## Uninstallation
Delete `wininet.dll` from Halo Infinite's installation directory.

# Building
1. Install `Nim`.   
    > **Make sure it is in the system path!**
2. Install `winim`.

    ```
    nimble install winim
    ```
3. Run `build.bat`.
    > Optional UPX compression is also performed if `upx.exe` is in the system path.

# Limitations

**Here are a few things you should note about ZetaLoader.**

1. Fixes specific techincal issues with the Halo Infinite.
2. Designed for the Steam release of Halo Infinite.
3. Might work with the UWP release or running under a Windows Compatibility Layer on Linux but the source code might need to be rewritten for these platforms.
