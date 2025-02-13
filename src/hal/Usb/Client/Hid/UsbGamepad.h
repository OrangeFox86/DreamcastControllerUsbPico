// MIT License
//
// Copyright (c) 2022-2025 James Smith of OrangeFox86
// https://github.com/OrangeFox86/DreamcastControllerUsbPico
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef __USB_GAMEPAD_H__
#define __USB_GAMEPAD_H__

#include <stdint.h>
#include "UsbControllerDevice.h"
#include "usb_descriptors.h"

//! This class is designed to work with the setup code in usb_descriptors.c
class UsbGamepad : public UsbControllerDevice
{
  public:
    enum DpadButtons
    {
      DPAD_UP = 0,
      DPAD_DOWN,
      DPAD_LEFT,
      DPAD_RIGHT,
      DPAD_COUNT
    };

    enum GamepadButton : uint8_t
    {
      GAMEPAD_BUTTON_A = 0,
      GAMEPAD_BUTTON_B = 1,
      GAMEPAD_BUTTON_C = 2,
      GAMEPAD_BUTTON_X = 3,
      GAMEPAD_BUTTON_Y = 4,
      GAMEPAD_BUTTON_Z = 5,
      GAMEPAD_BUTTON_TL = 6,
      GAMEPAD_BUTTON_TR = 7,
      GAMEPAD_BUTTON_TL2 = 8,
      GAMEPAD_BUTTON_TR2 = 9,
      GAMEPAD_BUTTON_SELECT = 10,
      GAMEPAD_BUTTON_START = 11,
      GAMEPAD_BUTTON_MODE = 12,
      GAMEPAD_BUTTON_THUMBL = 13,
      GAMEPAD_BUTTON_THUMBR = 14,
      BUTTON15 = 15,
      BUTTON16 = 16,
      BUTTON17 = 17,
      BUTTON18 = 18,
      BUTTON19 = 19,
      BUTTON20 = 20,
      BUTTON21 = 21,
      BUTTON22 = 22,
      BUTTON23 = 23,
      BUTTON24 = 24,
      BUTTON25 = 25,
      BUTTON26 = 26,
      BUTTON27 = 27,
      BUTTON28 = 28,
      BUTTON29 = 29,
      BUTTON30 = 30,
      BUTTON31 = 31
    };

  public:
    //! UsbKeyboard constructor
    UsbGamepad(uint8_t playerIdx);
    //! @returns true iff any button is currently "pressed"
    bool isButtonPressed() final;
    //! Sets the analog stick for the X direction
    //! @param[in] isLeft true for left, false for right
    //! @param[in] x Value between -128 and 127
    void setAnalogThumbX(bool isLeft, int8_t x);
    //! Sets the analog stick for the Y direction
    //! @param[in] isLeft true for left, false for right
    //! @param[in] y Value between -128 and 127
    void setAnalogThumbY(bool isLeft, int8_t y);
    //! Sets the analog trigger value (Z)
    //! @param[in] isLeft true for left, false for right
    //! @param[in] z Value between -128 and 127
    void setAnalogTrigger(bool isLeft, int8_t z);
    //! @param[in] isLeft true for left, false for right
    //! @returns the current analog stick X value
    int8_t getAnalogThumbX(bool isLeft);
    //! @param[in] isLeft true for left, false for right
    //! @returns the current analog stick Y value
    int8_t getAnalogThumbY(bool isLeft);
    //! @param[in] isLeft true for left, false for right
    //! @returns the current analog trigger value (Z)
    int8_t getAnalogTrigger(bool isLeft);
    //! Sets the state of a digital pad button
    //! @param[in] button The button to set
    //! @param[in] isPressed The state of @p button
    void setDigitalPad(DpadButtons button, bool isPressed);
    //! Sets or releases one or more of the 16 buttons as a mask value
    //! @param[in] mask The mask value to set or release
    //! @param[in] isPressed True to set the mask or false to release
    void setButtonMask(uint32_t mask, bool isPressed);
    //! Sets the state of a single button
    //! @param[in] button Button value [0,15]
    //! @param[in] isPressed The state of @p button
    void setButton(uint8_t button, bool isPressed);
    //! Release all currently pressed keys
    void updateAllReleased() final;
    //! Updates the host with any newly pressed keys
    //! @param[in] force  Set to true to update host regardless if key state has changed since last
    //!                   update
    //! @returns true if data has been successfully sent or if keys didn't need to be updated
    bool send(bool force = false) final;
    //! @returns the size of the report for this device
    virtual uint8_t getReportSize();
    //! Gets the report for the currently pressed keys
    //! @param[out] buffer  Where the report is written
    //! @param[in] reqlen  The length of buffer
    uint16_t getReport(uint8_t *buffer, uint16_t reqlen) final;

  protected:
    //! @returns the hat value based on current dpad state
    uint8_t getHatValue();

  private:
    //! @param[in] analog  The analog value to check
    //! @returns true if the given analog is considered "pressed"
    inline bool isAnalogPressed(int16_t analog)
    {
      return (analog > ANALOG_PRESSED_TOL || analog < -ANALOG_PRESSED_TOL);
    }

    inline bool isTriggerPressed(int16_t analog)
    {
      return (analog > (MIN_TRIGGER_VALUE + ANALOG_PRESSED_TOL));
    }

  public:
    //! Tolerance for when analog is considered "pressed" for status LED
    static const int8_t ANALOG_PRESSED_TOL = 5;

  private:
    const uint8_t playerIdx;
    //! Current left analog states (x,y,z)
    int8_t currentLeftAnalog[3];
    //! Current right analog states (x,y,z)
    int8_t currentRightAnalog[3];
    //! Current d-pad buttons
    bool currentDpad[DPAD_COUNT];
    //! Current button states
    uint32_t currentButtons;
    //! True when something has been updated since the last successful send
    bool buttonsUpdated;
};

#endif // __USB_CONTROLLER_H__
