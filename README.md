# ZetaLoader

A utility to fix technical issues with Halo Infinite on PC.

## Features

1. Highest Priority Window Thread  
    Reference: https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling-priorities#priority-level

    Microsoft recommends for any thread that handles input should use `THREAD_PRIORITY_HIGHEST | THREAD_PRIORITY_ABOVE_NORMAL`.        
    In the case of Halo Infinite, its window thread also handles input. 
    > **This only an assumption!**    
    > Reason being altering the window thread priority affects how input is processed by the game.

    For **some reason** the window thread priority is set THREAD_PRIORITY_NORMAL, one can verify this using `GetThreadPriority`.         

    Because the thread priority is normal, input in general doesn't feel responsive or feels stuttery.  
    You can also change the thread priority to `THREAD_PRIORITY_IDLE` in the function `IsPIDWnd` in [`src/Zeta.c`](https://github.com/Aetopia/ZetaLoader/blob/main/src/ZetaLoader.c).
    This is way to verify how "unresponsive" input is a result of a lower thread priority.

2. Borderless Fullscreen
    ZetaLoader (`Zeta.dll`) overrides Halo Infinite's borderless fullscreen/window style.        
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
