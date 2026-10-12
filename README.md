# PadOSDoom

This fork builds DoomGeneric as the `PadOS_Doom` static library and registers a
`doom` application through `PAppDefinition`. The first port uses a centered,
320 x 200 RGB565 framebuffer scaled to fit the window, keyboard input, and no
sound or music.

## Build and integration

`PADOS_ROOT_DIR` selects the PadOS source tree. `PADOS_INSTALL_DIR` selects the
installed dependencies (including CMSIS and nlohmann/json), and defaults to
`<PADOS_ROOT_DIR>/../Install`. The PadOS toolchain uses `PADOS_TOOLCHAIN_PATH`.
The library inherits the firmware's `PadOS::PadOS_Config` target, or loads the
installed PadOS CMake package for a standalone build.

Firmware projects can use `add_subdirectory()` and link `PadOSDoom::PadOS_Doom`
with whole-archive retention so the application's static registration is
retained. They must provide the PadOS GUI and system libraries and enough
memory for the engine.

## Run

Supply your own compatible IWAD file on a mounted filesystem, then launch:

```text
doom -iwad /path/to/doom1.wad
```

Arrow keys move and turn, Ctrl fires, Space uses doors and switches, Shift runs,
and Alt plus the left/right arrows strafes. Enter selects menu items, Escape
opens the menu, Tab opens the map, and number keys select weapons. Function
keys are unavailable through the current PadOS key-code interface. Touching
the view restores keyboard focus; touch gameplay controls are not implemented.

Display scaling defaults to `-scale auto`: the image fills as much of the window
as possible while preserving its 320:200 proportions. It stays centered with
black borders where needed and adjusts when the window is resized.

Use `-scale` with a positive number, including fractional values, to select a fixed display scale:

```text
doom -iwad /path/to/doom1.wad -scale 1
doom -iwad /path/to/doom1.wad -scale 1.5
doom -iwad /path/to/doom1.wad -scale 2
doom -iwad /path/to/doom1.wad -scale auto
```

Scale 1 displays 320 x 200, scale 1.5 displays 480 x 300, and scale 2 displays
640 x 400 when space permits. Fixed scales are capped to fit the current window
so the game is never cropped. The RA8875 driver uses software bilinear scaling
for resized frames; frame rate may decrease as the displayed image grows.
At native size, the adapter uses the existing unscaled copy path.

The engine framebuffer stays 320 x 200 RGB565: `-gfxmode` must remain `rgb565`
and the separate engine option `-scaling` must remain `1` if supplied.

Quit through Doom's menu or send the application a PadOS quit message.
Concurrent launches return an error.

Each session closes its files, frees engine allocations, and restores the engine's
initial global and function-local static state. The view is detached and GUI
access to the framebuffer completes before engine memory is freed.

The build uses GNU `objdump` and `objcopy` to generate reset metadata from the
engine's writable sections and redirect its allocation and file operations to
session tracking. Section names and alignment are preserved, so the firmware's
existing SRAM/SDRAM placement still applies. All engine writable sections are
retained, including otherwise unused data, and engine LTO is disabled. A temporary
heap snapshot of initialized data is kept while Doom runs; BSS needs no snapshot.
The metadata is part of the static library and needs no firmware linker changes.

The original DoomGeneric documentation follows.

# doomgeneric
The purpose of doomgeneric is to make porting Doom easier.
Of course Doom is already portable but with doomgeneric it is possible with just a few functions.

To try it you will need a WAD file (game data). If you don't own the game, shareware version is freely available (doom1.wad).

# porting
Create a file named doomgeneric_yourplatform.c and just implement these functions to suit your platform.
* DG_Init
* DG_DrawFrame
* DG_SleepMs
* DG_GetTicksMs
* DG_GetKey

|Functions            |Description|
|---------------------|-----------|
|DG_Init              |Initialize your platfrom (create window, framebuffer, etc...).
|DG_DrawFrame         |Frame is ready in DG_ScreenBuffer. Copy it to your platform's screen.
|DG_SleepMs           |Sleep in milliseconds.
|DG_GetTicksMs        |The ticks passed since launch in milliseconds.
|DG_GetKey            |Provide keyboard events.
|DG_SetWindowTitle    |Not required. This is for setting the window title as Doom sets this from WAD file.

### main loop
At start, call doomgeneric_Create().

In a loop, call doomgeneric_Tick().

In simplest form:
```
int main(int argc, char **argv)
{
    doomgeneric_Create(argc, argv);

    while (1)
    {
        doomgeneric_Tick();
    }
    
    return 0;
}
```

# sound
Sound is much harder to implement! If you need sound, take a look at SDL port. It fully supports sound and music! Where to start? Define FEATURE_SOUND, assign DG_sound_module and DG_music_module.

# platforms
Ported platforms include Windows, X11, SDL, emscripten. Just look at (doomgeneric_win.c, doomgeneric_xlib.c, doomgeneric_sdl.c).
Makefiles provided for each platform.

## emscripten
You can try it directly here:
https://ozkl.github.io/doomgeneric/

emscripten port is based on SDL port, so it supports sound and music! For music, timidity backend is used.

## Windows
![Windows](screenshots/windows.png)

## X11 - Ubuntu
![Ubuntu](screenshots/ubuntu.png)

## X11 - FreeBSD
![FreeBSD](screenshots/freebsd.png)

## SDL
![SDL](screenshots/sdl.png)
