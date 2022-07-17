#ifndef __MUTEX_INTERFACE_H__
#define __MUTEX_INTERFACE_H__

//! This interface is used to decouple mutex functionality in HAL from the Dreamcast functionality.
class MutexInterface
{
    public:
        //! Constructor
        MutexInterface() {}

        //! Virtual destructor
        virtual ~MutexInterface() {}

        //! Blocks until mutex is available and then takes it
        virtual void lock() = 0;

        //! Releases the previously locked mutex
        virtual void unlock() = 0;
};

#endif // __MUTEX_INTERFACE_H__
