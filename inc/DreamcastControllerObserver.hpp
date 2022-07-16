#ifndef __DREAMCAST_CONTROLLER_OBSERVER_H__
#define __DREAMCAST_CONTROLLER_OBSERVER_H__

#include <stdint.h>

class DreamcastControllerObserver
{
    public:
        //! Structure used to unpack an 8-byte controller condition package
        struct ControllerCondition
        {
            uint8_t l; //!< 0: fully released; 255: fully pressed

            uint8_t r; //!< 0: fully released; 255: fully pressed

            // Digital bits:
            // 0: pressed
            // 1: released
            unsigned z:1;
            unsigned y:1;
            unsigned x:1;
            unsigned unknown5:1;
            unsigned unknown4:1;
            unsigned unknown3:1;
            unsigned unknown2:1;
            unsigned unknown1:1;

            unsigned c:1;
            unsigned b:1;
            unsigned a:1;
            unsigned start:1;
            unsigned up:1;
            unsigned down:1;
            unsigned left:1;
            unsigned right:1;

            uint8_t rAnalogUD; //!< Always 128

            uint8_t rAnalogLR; //!< Always 128

            uint8_t lAnalogUD; //!< 0: up; 128: neutral; 255: down

            uint8_t lAnalogLR; //!< 0: left; 128: neutral; 255: right
        } __attribute__ ((packed));

        //! Sets the current Dreamcast controller condition
        //! @param[in] controllerCondition  The current condition of the Dreamcast controller
        virtual void setControllerCondition(const ControllerCondition& controllerCondition) = 0;

        //! Called when controller connected
        virtual void controllerConnected() = 0;

        //! Called when controller disconnected
        virtual void controllerDisconnected() = 0;
};

#endif // __DREAMCAST_CONTROLLER_OBSERVER_H__
