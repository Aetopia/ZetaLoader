# ZetaLoader

A utility to fix technical issues with Halo Infinite on PC.

## Fixes

1. Jittery/Stuttery Input Issues for Mouse & Keyboard

    > Reference: https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling-priorities  

    This fix simply resolves jittery input related issues with the game.      
    Setting the window thread priority to `THREAD_PRIORITY_HIGHEST` fixes this issue entirely. 

    > ZetaLoader sets the window thread priority to `THREAD_PRIORITY_TIME_CRITICAL` just to assign a higher priority over threads that use `THREAD_PRIORITY_HIGHEST`.

    As for the reason why this fixes the issue entirely is maybe due to the fact, the window thread doesn't get enough of a timeslice resulting in jittery/stuttery input.    
    Increasing the thread priority gives it enough of a timeslice that seems to resolve this issue.   

    You can verify if this works by using a lower thread priority set it using the function `SetThreadPriority` in the source code.       

2. Borderless Fullscreen

    > **Available with Borderless Fullscreen only at game startup.**

    ZetaLoader overrides Halo Infinite's borderless fullscreen/window style.            
    Halo Infinite uses the following styles `WS_VISIBLE | WS_OVERLAPPED | WS_CLIPSIBLINGS` when going borderless fullscreen.  
    
    The window style `WS_OVERLAPPED` doesn't fill the screen area correctly if the display resolution is below `1440x810` resulting in the window to extend its client area beyond the screen area itself. 

    This is simply fixed by using the following styles:      
    - Window Styles: `WS_VISIBLE | WS_POPUP` 
    - Extended Window Styles: `WS_EX_APPWINDOW`
    - Topmost Window: `HWND_TOPMOST`

## Features   
1. User Defined Display Resolution/Mode Support

    > **Available with Borderless Fullscreen only at game startup.**   

    This feature restores the game's ability to run at any user defined resolution. Here is what this feature emulates/restores:
    - Automatically Minimize, when the game is not the foreground window.
    - Dynamically switch between your desired game display resolution & native display resolution like in exclusive fullscreen.

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

    > **Why not use High Process Priority?** 
    > 
    > Using a high process priority might prevent other threads on the system from getting processor time. Microsoft recommends this process priority to be used, if only doing time critical tasks.    
    > Reference: https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling-priorities
    >        
    > Also this fix `Screen Tearing | Input Issues with External/Driver Based Framelimiters` becomes useless if the process priority is set to high.      
    > As for the reason, it likely maybe other threads within Halo Infinite getting a higher thread priority than before and preventing the window thread from getting enough of a timeslice, making the fix entirely useless.

## Result
> Specifications:     
    1. i7-10700K @ 4.8 GHz HT Disabled @ 1.15V   
    2. GTX 1650 @ 2055 ~ 2070 MHz (135 MHz+ Core) & 7200 MHz (1200 MHz+ RAM) OCs   
    3. 2933 MHz RAM (**But its mismatched!**)     

**Note: My minimum framerate is set to 960 FPS in the linked video.**      
Result Video: https://www.youtube.com/watch?v=o9u0oAyv3dc

## Installation
1. Download `DumpTool.exe` from [GitHub Releases](https://github.com/Aetopia/ZetaLoader).   

2. Browse Halo Infinite's local files.  

    ![Properties](https://i.imgur.com/8HKvH4U.png)

3. Replace the file called `DumpTool.exe` with ZetaLoader's `DumpTool.exe`.   

    ![Replace `DumpTool.exe`](https://i.imgur.com/h0wKBBk.png)

4. - Launch Halo Infinite. 
    - This will create 2 new files called `ZetaLoader.dll` & `ZetaLoader.ini`.

7.  To configure the resolution ZetaLoader applies to Halo Infinite, modify `ZetaLoader.ini`.         
    To launch Halo Infinite at `1280 x 720`:              
    ```
    Width = 1280
    Height = 720
    ```
    Also ensure **Borderless Fullscreen is enabled** & set this as the contents of `ZetaLoader.ini`.

## Uninstallation
1. Delete `DumpTool.exe`, `ZetaLoader.dll`, `ZetaLoader.txt` from Halo Infinite's installation directory.
2. Verify integrity of the game files to restore the original `DumpTool.exe`.

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
