# ZetaLoader

A utility to fix technical issues with Halo Infinite on PC.

## Fixes

1. Highest Priority Window Thread  
    Reference: <https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling-priorities#priority-class>     

    Microsoft recommends for any thread that handles input should use THREAD_PRIORITY_HIGHEST.        
    In the case of Halo Infinite, its window thread also handles input.      
    For **some reason** the window thread priority is set THREAD_PRIORITY_NORMAL, one can verify this using `GetThreadPriority`.         

    Because the thread priority is normal, input in general doesn't feel responsive or feels stuttery.  
    You can also change the thread priority to `THREAD_PRIORITY_IDLE` in the function `IsPIDWnd` in [`src/Zeta.c`](https://github.com/Aetopia/ZetaLoader/blob/main/src/Zeta.c).
    This is way to verify how "unresponsive" input can become.

## Limitations

**Here are a few things you should note about ZetaLoader.**

1. Fixes specific techincal issues with the Halo Infinite.
2. Designed for the Steam release of Halo Infinite.
3. Might work with the UWP release or running under a Windows Compatibility Layer on Linux, but the source code might need to be rewritten for these platforms.
