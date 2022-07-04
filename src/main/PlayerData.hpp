#pragma once

#include "UsbGamepad.h"
#include "ScreenData.hpp"

//! Contains data that is tied to a specific player
struct PlayerData
{
    const uint32_t playerIndex;
    UsbGamepad& gamepad;
    ScreenData& screenData;
};