#ifndef __USB_GAMEPAD_H__
#define __USB_GAMEPAD_H__

#include <stdint.h>
#include "UsbControllerDevice.h"

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

  public:
    //! UsbKeyboard constructor
    //! @param[in] reportId  The report ID to use for this USB keyboard
    UsbGamepad(uint8_t interfaceId, uint8_t reportId = 0);
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
    void setButtonMask(uint16_t mask, bool isPressed);
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
    void getReport(uint8_t *buffer, uint16_t reqlen) final;

  protected:
    //! @returns the hat value based on current dpad state
    uint8_t getHatValue();

  private:
    const uint8_t interfaceId;
    //! The report ID to use when sending keys to host
    const uint8_t reportId;
    //! Current left analog states (x,y,z)
    int8_t currentLeftAnalog[3];
    //! Current right analog states (x,y,z)
    int8_t currentRightAnalog[3];
    //! Current d-pad buttons
    bool currentDpad[DPAD_COUNT];
    //! Current button states
    uint16_t currentButtons;
    //! True when something has been updated since the last successful send
    bool buttonsUpdated;
};

#endif // __USB_CONTROLLER_H__
