#pragma once

#include "hal/Usb/DreamcastControllerObserver.hpp"
#include "ScreenData.hpp"

//! Contains data that is tied to a specific player
struct PlayerData
{
    const uint32_t playerIndex;
    DreamcastControllerObserver& gamepad;
    ScreenData& screenData;
};