# ZetaLoader

A utility to fix technical issues with Halo Infinite on PC.

## Features

1. Highest Priority Window Thread  
    **This fixes screen tearing & input issues when an external framelimiter is being used.**
              
    When using an external framelimiter, the game has intense screen tearing.        
    The reason why this happens might be due:
    1. To the external framelimiter doesn't framepace correctly when the FPS is capped.
    2. The Window Thread goes to sleep for some reason when a external framelimiter is used and the Window Thread handles the `WM_INPUT` message.

    **Note: This are only my assumptions!**           
    Source: https://forums.guru3d.com/threads/msi-ab-rtss-development-news-thread.412822/page-161#post-5949434         

    Setting the window thread priority to `THREAD_PRIORITY_HIGHEST` fixes this issue entirely.
    > This fix was encountered when messing around with the window thread priority.
    You can verify if this works by using a lower thread priority is set by `SetThreadPriority` in the source code.

2. Borderless Fullscreen 
    ZetaLoader overrides Halo Infinite's borderless fullscreen/window style.            
    Halo Infinite uses the following styles `WS_VISIBLE | WS_OVERLAPPED | WS_CLIPSIBLINGS` when going borderless fullscreen but this causes an issue where any resolution below `1440x810` will make the game window to extend its client area beyond the resolution itself.           
    This is simply fixed by using the following styles:      
    - Window Styles: `WS_VISIBLE | WS_POPUP` 
    - Extended Window Styles `WS_EX_APPWINDOW | WS_EX_TOPMOST`  
    
    > You can use something like [Borderless Gaming](https://github.com/Codeusa/Borderless-Gaming) to fix this issue.
    
3. User Defined Display Resolution/Mode Support
    This feature restores the game's ability to run at any user defined resolution. Here is what this feature emulates/restores:
    - Automatically Minimize, when the game is not the foreground window.
    - Dynamically switch between your desired game display resolution & native display resolution like in exclusive fullscreen.            

    > Sometimes the taskbar can appear on top of Halo Infinite's window.        
    To fix this simply Alt + Tab back to the game to resolve the issue.

## Limitations

**Here are a few things you should note about ZetaLoader.**

1. Fixes specific techincal issues with the Halo Infinite.
2. Designed for the Steam release of Halo Infinite.
3. Might work with the UWP release or running under a Windows Compatibility Layer on Linux, but the source code might need to be rewritten for these platforms.
