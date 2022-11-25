#ifndef __USB_INTERFACE_H__
#define __USB_INTERFACE_H__

#include "DreamcastControllerObserver.hpp"
#include "hal/System/MutexInterface.hpp"

//! @returns array of the USB controller observers
DreamcastControllerObserver** get_usb_controller_observers();
//! USB initialization
void usb_init();
//! USB task that needs to be called constantly by main()
void usb_task();
//! @returns number of USB controllers
uint32_t get_num_usb_controllers();

//! Interface for a dreamcast device to inherit when it must show as a file on mass storage
class UsbMscFile
{
    public:
        //! Virtual destructor
        virtual ~UsbMscFile() {}
        //! @returns file name
        virtual const char* getFileName() = 0;
        //! @returns file size in bytes (currently only up to 128KB supported)
        virtual uint32_t getFileSize() = 0;
        //! Blocking read (must only be called from the core not operating maple bus)
        //! @param[in] blockNum  Block number to read (a block is 512 bytes)
        //! @param[out] buffer  Buffer output
        //! @param[in] bufferLen  The length of buffer (only up to 512 bytes will be written)
        //! @param[in] timeoutUs  Timeout in microseconds
        //! @returns Positive value indicating how many bytes were read
        //! @returns Zero if read failure occurred
        //! @returns Negative value if timeout elapsed
        virtual int32_t read(uint8_t blockNum,
                             void* buffer,
                             uint16_t bufferLen,
                             uint32_t timeoutUs) = 0;
};

//! Add a file to the mass storage device
void usb_msc_add(UsbMscFile* file);
//! Remove a file from the mass storage device
void usb_msc_remove(UsbMscFile* file);
//! Set mutex to use between the above two calls and file access
void usb_msc_set_mutex(MutexInterface* mutex);



#endif // __USB_INTERFACE_H__
