#include "UsbGamepad.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "class/hid/hid.h"

UsbGamepad::UsbGamepad(uint8_t interfaceId, uint8_t reportId) :
  interfaceId(interfaceId),
  reportId(reportId),
  currentDpad(),
  currentButtons(0),
  buttonsUpdated(false)
{}

bool UsbGamepad::isButtonPressed()
{
  return (currentDpad[DPAD_UP] || currentDpad[DPAD_DOWN] || currentDpad[DPAD_LEFT]
    || currentDpad[DPAD_RIGHT] || currentButtons != 0
    || currentLeftAnalog[0] != 0x80 || currentLeftAnalog[1] != 0x80 || currentLeftAnalog[2] != 0
    || currentRightAnalog[0] != 0x80 || currentRightAnalog[1] != 0x80 || currentRightAnalog[2] != 0);
}

//--------------------------------------------------------------------+
// EXTERNAL API
//--------------------------------------------------------------------+
void UsbGamepad::setAnalogThumbX(bool isLeft, uint8_t x)
{
  int8_t lastX = 0;
  if (isLeft)
  {
    lastX = currentLeftAnalog[0];
    currentLeftAnalog[0] = x;
  }
  else
  {
    lastX = currentRightAnalog[0];
    currentRightAnalog[0] = x;
  }
  buttonsUpdated = buttonsUpdated || (x != lastX);
}

void UsbGamepad::setAnalogThumbY(bool isLeft, uint8_t y)
{
  int8_t lastY = 0;
  if (isLeft)
  {
    lastY = currentLeftAnalog[1];
    currentLeftAnalog[1] = y;
  }
  else
  {
    lastY = currentRightAnalog[1];
    currentRightAnalog[1] = y;
  }
  buttonsUpdated = buttonsUpdated || (y != lastY);
}

void UsbGamepad::setAnalogTrigger(bool isLeft, uint8_t z)
{
  int8_t lastZ = 0;
  if (isLeft)
  {
    lastZ = currentLeftAnalog[2];
    currentLeftAnalog[2] = z;
  }
  else
  {
    lastZ = currentRightAnalog[2];
    currentRightAnalog[2] = z;
  }
  buttonsUpdated = buttonsUpdated || (z != lastZ);
}

uint8_t UsbGamepad::getAnalogThumbX(bool isLeft)
{
  if (isLeft)
  {
    return currentLeftAnalog[0];
  }
  else
  {
    return currentRightAnalog[0];
  }
}

uint8_t UsbGamepad::getAnalogThumbY(bool isLeft)
{
  if (isLeft)
  {
    return currentLeftAnalog[1];
  }
  else
  {
    return currentRightAnalog[1];
  }
}

uint8_t UsbGamepad::getAnalogTrigger(bool isLeft)
{
  if (isLeft)
  {
    return currentLeftAnalog[2];
  }
  else
  {
    return currentRightAnalog[2];
  }
}

void UsbGamepad::setDigitalPad(UsbGamepad::DpadButtons button, bool isPressed)
{
  bool oldValue = currentDpad[button];
  currentDpad[button] = isPressed;
  buttonsUpdated = buttonsUpdated || (oldValue != currentDpad[button]);
}

void UsbGamepad::setButtonMask(uint16_t mask, bool isPressed)
{
  uint16_t lastButtons = currentButtons;
  if (isPressed)
  {
    currentButtons |= mask;
  }
  else
  {
    currentButtons &= ~mask;
  }
  buttonsUpdated = buttonsUpdated || (lastButtons != currentButtons);
}

void UsbGamepad::setButton(uint8_t button, bool isPressed)
{
  setButtonMask(1 << button, isPressed);
}

void UsbGamepad::updateAllReleased()
{
  if (isButtonPressed())
  {
    currentLeftAnalog[0] = 0x80;
    currentLeftAnalog[1] = 0x80;
    currentLeftAnalog[2] = 0;
    currentRightAnalog[0] = 0x80;
    currentRightAnalog[1] = 0x80;
    currentRightAnalog[2] = 0;
    currentDpad[DPAD_UP] = false;
    currentDpad[DPAD_DOWN] = false;
    currentDpad[DPAD_LEFT] = false;
    currentDpad[DPAD_RIGHT] = false;
    currentButtons = 0;
    buttonsUpdated = true;
  }
}

uint8_t UsbGamepad::getHatValue()
{
  if (currentDpad[DPAD_UP])
  {
    if (currentDpad[DPAD_LEFT])
    {
      return GAMEPAD_HAT_UP_LEFT;
    }
    else if (currentDpad[DPAD_RIGHT])
    {
      return GAMEPAD_HAT_UP_RIGHT;
    }
    else
    {
      return GAMEPAD_HAT_UP;
    }
  }
  else if (currentDpad[DPAD_DOWN])
  {
    if (currentDpad[DPAD_LEFT])
    {
      return GAMEPAD_HAT_DOWN_LEFT;
    }
    else if (currentDpad[DPAD_RIGHT])
    {
      return GAMEPAD_HAT_DOWN_RIGHT;
    }
    else
    {
      return GAMEPAD_HAT_DOWN;
    }
  }
  else if (currentDpad[DPAD_LEFT])
  {
    return GAMEPAD_HAT_LEFT;
  }
  else if (currentDpad[DPAD_RIGHT])
  {
    return GAMEPAD_HAT_RIGHT;
  }
  return GAMEPAD_HAT_CENTERED;
}

bool UsbGamepad::send(bool force)
{
  if (buttonsUpdated || force)
  {
    bool sent = sendReport(interfaceId, reportId);
    if (sent)
    {
      buttonsUpdated = false;
    }
    return sent;
  }
  else
  {
    return true;
  }
}

uint8_t UsbGamepad::getReportSize()
{
  return sizeof(hid_gamepad_report_t);
}

void UsbGamepad::getReport(uint8_t *buffer, uint16_t reqlen)
{
  // Build the report
  hid_gamepad_report_t report;
  report.x = static_cast<int32_t>(currentLeftAnalog[0]) - 128;
  report.y = static_cast<int32_t>(currentLeftAnalog[1]) - 128;
  report.z = static_cast<int32_t>(currentLeftAnalog[2]) - 128;
  report.rx = static_cast<int32_t>(currentRightAnalog[0]) - 128;
  report.ry = static_cast<int32_t>(currentRightAnalog[1]) - 128;
  report.rz = static_cast<int32_t>(currentRightAnalog[2]) - 128;
  report.hat = getHatValue();
  report.buttons = currentButtons;
  // Copy report into buffer
  uint16_t setLen = (sizeof(report) <= reqlen) ? sizeof(report) : reqlen;
  memcpy(buffer, &report, setLen);
}
