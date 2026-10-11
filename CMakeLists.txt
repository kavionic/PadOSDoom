cmake_minimum_required(VERSION 3.25)

set(PADOS_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Rainbow3DFirmware/PadOS" CACHE PATH "PadOS source directory")

if(NOT EXISTS "${PADOS_ROOT_DIR}/Include/System/AppDefinition.h")
    message(FATAL_ERROR "Set PADOS_ROOT_DIR to the PadOS source directory containing Include/System/AppDefinition.h.")
endif()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR AND NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${PADOS_ROOT_DIR}/Tools/CMake/ToolChains/arm-gcc.stm32h7xx.cmake" CACHE FILEPATH "PadOS toolchain")
endif()

project(PadOSDoom VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(PadOS_Doom STATIC
    doomgeneric/dummy.c
    doomgeneric/am_map.c
    doomgeneric/doomdef.c
    doomgeneric/doomstat.c
    doomgeneric/dstrings.c
    doomgeneric/d_event.c
    doomgeneric/d_items.c
    doomgeneric/d_iwad.c
    doomgeneric/d_loop.c
    doomgeneric/d_main.c
    doomgeneric/d_mode.c
    doomgeneric/d_net.c
    doomgeneric/f_finale.c
    doomgeneric/f_wipe.c
    doomgeneric/g_game.c
    doomgeneric/hu_lib.c
    doomgeneric/hu_stuff.c
    doomgeneric/info.c
    doomgeneric/i_cdmus.c
    doomgeneric/i_endoom.c
    doomgeneric/i_joystick.c
    doomgeneric/i_scale.c
    doomgeneric/i_sound.c
    doomgeneric/i_system.c
    doomgeneric/i_timer.c
    doomgeneric/memio.c
    doomgeneric/m_argv.c
    doomgeneric/m_bbox.c
    doomgeneric/m_cheat.c
    doomgeneric/m_config.c
    doomgeneric/m_controls.c
    doomgeneric/m_fixed.c
    doomgeneric/m_menu.c
    doomgeneric/m_misc.c
    doomgeneric/m_random.c
    doomgeneric/p_ceilng.c
    doomgeneric/p_doors.c
    doomgeneric/p_enemy.c
    doomgeneric/p_floor.c
    doomgeneric/p_inter.c
    doomgeneric/p_lights.c
    doomgeneric/p_map.c
    doomgeneric/p_maputl.c
    doomgeneric/p_mobj.c
    doomgeneric/p_plats.c
    doomgeneric/p_pspr.c
    doomgeneric/p_saveg.c
    doomgeneric/p_setup.c
    doomgeneric/p_sight.c
    doomgeneric/p_spec.c
    doomgeneric/p_switch.c
    doomgeneric/p_telept.c
    doomgeneric/p_tick.c
    doomgeneric/p_user.c
    doomgeneric/r_bsp.c
    doomgeneric/r_data.c
    doomgeneric/r_draw.c
    doomgeneric/r_main.c
    doomgeneric/r_plane.c
    doomgeneric/r_segs.c
    doomgeneric/r_sky.c
    doomgeneric/r_things.c
    doomgeneric/sha1.c
    doomgeneric/sounds.c
    doomgeneric/statdump.c
    doomgeneric/st_lib.c
    doomgeneric/st_stuff.c
    doomgeneric/s_sound.c
    doomgeneric/tables.c
    doomgeneric/v_video.c
    doomgeneric/wi_stuff.c
    doomgeneric/w_checksum.c
    doomgeneric/w_file.c
    doomgeneric/w_main.c
    doomgeneric/w_wad.c
    doomgeneric/z_zone.c
    doomgeneric/w_file_stdc.c
    doomgeneric/i_input.c
    doomgeneric/i_video.c
    doomgeneric/doomgeneric.c
)

add_library(PadOSDoom::PadOS_Doom ALIAS PadOS_Doom)

set_target_properties(PadOS_Doom PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    UNITY_BUILD OFF
)

target_include_directories(PadOS_Doom PRIVATE
    doomgeneric
    "${PADOS_ROOT_DIR}/Include"
)

target_compile_definitions(PadOS_Doom PRIVATE
    DOOMGENERIC_RESX=320
    DOOMGENERIC_RESY=200
)

if(TARGET PadOS::PadOS_Config)
    target_link_libraries(PadOS_Doom PRIVATE PadOS::PadOS_Config)
endif()

target_link_libraries(PadOS_Doom PRIVATE m)
