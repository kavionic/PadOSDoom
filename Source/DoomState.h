// This file is part of PadOSDoom.
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>

namespace PadOSDoom
{

struct DoomStateRegion
{
    void* Address;
    size_t Size;
    bool ZeroInitialized;
};

extern const DoomStateRegion DOOM_STATE_REGIONS[];
extern const size_t DOOM_STATE_REGION_COUNT;

}
