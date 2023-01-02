# ZetaLoader

A utility to fix technical issues with Halo Infinite on PC.

## Fixes

1. Screen Tearing | Input Issues with External/Driver Based Framelimiters 

    > Source: https://forums.guru3d.com/threads/msi-ab-rtss-development-news-thread.412822/page-161#post-5949434   

    This fix simply resolves intense screen tearing and input related issues when an external/driver based framerate limiter is being used.
    Setting the window thread priority to `THREAD_PRIORITY_HIGHEST | THREAD_PRIORITY_TIME_CRITICAL` fixes this issue entirely.
    You can verify if this works by using a lower thread priority is set by `SetThreadPriority` in the source code and using an external framerate limiter like [RTSS](https://www.guru3d.com/files-details/rtss-rivatuner-statistics-server-download.html).       
    This fix was encountered when messing around with the window thread priority.

2. Borderless Fullscreen

    > **Available with Borderless fullscreen only at game startup.**

    ZetaLoader overrides Halo Infinite's borderless fullscreen/window style.            
    Halo Infinite uses the following styles `WS_VISIBLE | WS_OVERLAPPED | WS_CLIPSIBLINGS` when going borderless fullscreen but this causes an issue where any resolution below `1440x810` will make the game window to extend its client area beyond the resolution itself.           
    This is simply fixed by using the following styles:      
    - Window Styles: `WS_VISIBLE | WS_POPUP` 
    - Extended Window Styles `WS_EX_APPWINDOW`  
    
    > You can use something like [Borderless Gaming](https://github.com/Codeusa/Borderless-Gaming) to fix this issue.
## Features   
1. User Defined Display Resolution/Mode Support Restoration

    > **Available with Borderless fullscreen only at game startup.**
    This feature restores the game's ability to run at any user defined resolution. Here is what this feature emulates/restores:
    - Automatically Minimize, when the game is not the foreground window.
    - Dynamically switch between your desired game display resolution & native display resolution like in exclusive fullscreen.            

    > Sometimes the taskbar can appear on top of Halo Infinite's window.        
    To fix this simply Alt + Tab back to the game to resolve the issue.

2. Timer Resolution

    Using the hidden `NtSetTimerResolution` function, Halo Infinite can use a maximum timer resolution of 0.5 ms.
    >  Halo Infinite uses 1 ms by default, we can force 0.5 ms using NtSetTimerResolution.        
    Starting with Windows 2004, setting the timer resolution is no longer global but on a per process basis.       
    Reference: https://learn.microsoft.com/en-us/windows/win32/api/timeapi/nf-timeapi-timebeginperiod#remarks    

3.  Multimedia Class Schedule Service Scheduling

    Using the function `DwmEnableMMCSS` makes the calling process opt in for multimedia class schedule service scheduling which boost performance in non-fullscreen applications.  
    [More information can be found here](https://github.com/djdallmann/GamingPCSetup/blob/master/CONTENT/RESEARCH/WINSERVICES/README.md#multimedia-class-scheduler-service-mmcss).

## Installation:
1. Download ZetaLoader from [GitHub Releases](https://github.com/Aetopia/ZetaLoader).
2. 

## Limitations

**Here are a few things you should note about ZetaLoader.**

1. Fixes specific techincal issues with the Halo Infinite.
2. Designed for the Steam release of Halo Infinite.
3. Might work with the UWP release or running under a Windows Compatibility Layer on Linux, but the source code might need to be rewritten for these platforms.
