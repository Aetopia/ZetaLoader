> [!WARNING]
> ZetaLoader will be archived since [CU31](https://support.halowaypoint.com/hc/en-us/articles/24540901669780-Halo-Infinite-Content-Update-31-Patch-Notes) introduces Easy Anti-Cheat which blocks the modification from loading.

# ZetaLoader

A modification to fix technical issues with Halo Infinite on PC.

## Features
#### Borderless Fullscreen

> [!WARNING] 
> ZetaLoader's Borderless Fullscreen renders the following options unusable:<br>
> - Display Monitor
> - Limit Inactive Framerate
> - Inactive Mute
> - V-Sync
> - Maximum Framerate
> 
> Because of this consider using alternatives for the following:
> |Option|Alternative|
> |-|-|
> |V-Sync|Driver Based V-Sync|
> |Maximum Framerate|External Framerate Limiter|
> |Limit Inactive Framerate|Background Application Max Frame Rate (**NVIDIA Only!**)|

> [!IMPORTANT]
> - ZetaLoader's Borderless Fullscreen forces the game's window to be always on the primary monitor.<br>
> - To activate the game's window either click on its taskbar button or use <kbd>Alt</kbd> + <kbd>Tab</kbd>.

ZetaLoader overrides Halo Infinite's Borderless Fullscreen/window style, if its Borderless Fullscreen implementation is enabled.      
Halo Infinite uses the following styles `WS_VISIBLE | WS_OVERLAPPED | WS_CLIPSIBLINGS` for its Borderless Fullscreen implementation.  

The window style `WS_OVERLAPPED` doesn't fill the screen area correctly if the display mode is below `1440x810` resulting in the window to extend its client area beyond the screen area itself. 

This is simply fixed by using the following attributes:        
- **Window Styles - `WS_VISIBLE | WS_POPUP`:**   
    - Fixes incorrect window sizing caused by `WS_OVERLAPPED`.

- **Extended Window Styles - `WS_EX_APPWINDOW | WS_EX_NOACTIVATE`**:
    - Force the window to appear onto the taskbar.
    - Prevent window activation due to a mouse click or a window being minimized or closed.
    
- **Z-Order**:<br>
    - **`HWND_TOPMOST`**: The game's window becomes always on top when it is in the foreground.
    - **`HWND_BOTTOM`**: The game's window drops to the bottom if it is not in the foreground.



#### User Specified Display Mode  
> [!NOTE]
> User Specified Display Mode Demonstration: https://www.youtube.com/watch?v=FnzN4xTO6UA

> [!TIP]
> - ZetaLoader's Borderless Fullscreen must be enabled to use this feature.
> - Adjust Halo Infinite's UI's **`Text Size`**.
> - In your GPU's Control Panel set the Scaling Mode to **`Fullscreen`**│**`Stretched`**.
> - User Specified Display Mode handles display modes as follows:<br>
>   -  Only Landscape orientation based display modes can be used.
>   -  If no display mode is specified, the display mode stored in the Windows Registry will be used.

User Specified Display Mode provides Halo Infinite with the facility to have the game's window run at any arbitrary display mode of the user's choice as long as it is valid.



#### Jittery Mouse Input Fix
> [!IMPORTANT]
> Setting the game's process priority to `High` will negate this fix.    
> Reference: https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling-priorities       
> Issue + Fix Demonstration: https://www.youtube.com/watch?v=4pJd-dKW7WY      
      
Setting the game's window thread priority to `THREAD_PRIORITY_HIGHEST` resolves jittery mouse input when an external framerate limiter is being used by giving the game's window thread enough of a timeslice. <br>  
You can verify if this works by using a lower thread priority & setting it using the function `SetThreadPriority` in the source code. 

## Installation
### Automated
Run the following script in PowerShell to install ZetaLoader for Halo Infinite's Campaign and Multiplayer:<br>

```powershell
$ProgressPreference = $ErrorActionPreference = "SilentlyContinue"
Get-Content  "$(Split-Path $(([string]((Get-ItemPropertyValue `
-Path "Registry::HKEY_CLASSES_ROOT\steam\Shell\Open\Command" `
-Name "(Default)") `
-Split "-", 2, "SimpleMatch")[0]).Trim().Trim('"')))\config\libraryfolders.vdf" | 
ForEach-Object { 
    if ($_ -like '*"path"*') {
        [string]$Path = "$(([string]$_).Trim().Trim('"path"').Trim().Trim('"').Replace("\\", "\"))\steamapps\common\Halo Infinite" 
        if (Test-Path $Path) {
            Write-Host "ZetaLoader Installation Status:" -ForegroundColor Yellow
            try {
                Invoke-RestMethod `
                    -Uri  "$((Invoke-RestMethod "https://api.github.com/repos/Aetopia/ZetaLoader/releases/latest").assets[0].browser_download_url)" `
                    -OutFile "$Path\game\dpapi.dll"
                Write-Host "`tMultiplayer: Success" -ForegroundColor Green 
            }
            catch { Write-Host "`tMultiplayer: Failed" -ForegroundColor Red }

            try {
                Copy-Item "$Path\game\dpapi.dll" "$Path\subgames\CampaignS1\dpapi.dll"
                Write-Host "`tCampaign: Success" -ForegroundColor Green 
            }
            catch { Write-Host "`tCampaign: Failed" -ForegroundColor Red }
        }
    }
}
$ProgressPreference = $ErrorActionPreference = "Continue"
```

### Manual
1. Download the latest version of ZetaLoader from [GitHub Releases](https://github.com/Aetopia/ZetaLoader/releases/latest).
2. Open Halo Infinite's installation directory.
3. Place the dynamic link library in the following folders:<br>
    - Multiplayer: `<Installation Directory>\game`
    - Campaign: `<Installation Directory>\subgames\CampaignS1`

### Uninstallation
Simply remove any instances of `dpapi.dll` from Halo Infinite's installation directory.

## Configuration

> [!IMPORTANT] 
> You must restart the game for any configuration file changes to reflect.

To configure ZetaLoader, do the following:
1. Go to the following directory: 
    - `%LOCALAPPDATA%\HaloInfinite\Settings`
2. Create a new file called `ZetaLoader.ini`, this is ZetaLoader's configuration file.
3. Add the following contents into the file:
    ```ini
    [Settings]
    Fullscreen = 0 
    Width = 0
    Height = 0
    Frequency = 0
    ```

    |Key|Value|
    |-|-|
    |`Fullscreen`|<ul><li>`0` &rarr; Halo Infinite's Window Mode</li><li>`1` &rarr; ZetaLoader's Borderless Fullscreen</li></ol>|
    |`Width`|Display Resolution Width|
    |`Height`|Display Resolution Height|
    |`Frequency`|Display Refresh Rate|

This will make Halo Infinite run `1360`x`768` @ `60` Hz with ZetaLoader's Borderless Fullscreen:<br>
```ini
[Settings]
Fullscreen = 1
Width = 1360
Height = 768
Frequency = 60
```

## Building
1. Install [MSYS2](https://www.msys2.org/) & [UPX](https://upx.github.io/) for optional compression.
2. Update the MSYS2 Environment until there are no pending updates using:

    ```bash
    pacman -Syu --noconfirm
    ```

3. Install GCC & C++/WinRT using:

    ```bash
    pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cppwinrt --noconfirm
    ```

3. Make sure `<MSYS2 Installation Directory>\ucrt64\bin` is added to the Windows `PATH` environment variable.
4. Run [`Build.cmd`](Build.cmd).
