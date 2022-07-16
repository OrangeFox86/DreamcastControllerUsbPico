#ifndef __USB_CONTROLLER_INTERFACE_H__
#define __USB_CONTROLLER_INTERFACE_H__

#include <stdint.h>

//! Interface for a USB controller
class UsbControllerInterface
{
  public:
    UsbControllerInterface() {}
    virtual ~UsbControllerInterface() {}
    //! @returns true iff there is at least 1 button pressed on the device
    virtual bool isButtonPressed() = 0;
    //! Release all currently pressed buttons
    virtual void updateAllReleased() = 0;
    //! Updates the host with pressed buttons
    //! @param[in] force  Set to true to update host regardless if key state has changed since last
    //!                   update
    //! @returns true if data has been successfully sent or if buttons didn't need to be updated
    virtual bool send(bool force = false) = 0;
    //! @returns the size of the report for this device
    virtual uint8_t getReportSize() = 0;
    //! Gets the report for the currently pressed buttons
    //! @param[out] buffer  Where the report is written
    //! @param[in] reqlen  The length of buffer
    virtual void getReport(uint8_t *buffer, uint16_t reqlen) = 0;
    //! Called only from callbacks to update USB connected state
    //! @param[in] connected  true iff USB connected
    virtual void updateUsbConnected(bool connected) = 0;
    //! @returns the current USB connected state
    virtual bool isUsbConnected() = 0;
    //! Called only from callbacks to update controller connected state
    //! @param[in] connected  true iff controller connected
    virtual void updateControllerConnected(bool connected) = 0;
    //! @returns the current controller connected state
    virtual bool isControllerConnected() = 0;
};

#endif // __USB_CONTROLLER_INTERFACE_H__
