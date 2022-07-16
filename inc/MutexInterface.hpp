#ifndef __MUTEX_INTERFACE_H__
#define __MUTEX_INTERFACE_H__

class MutexInterface
{
    public:
        MutexInterface() {}
        virtual ~MutexInterface() {}

        virtual void lock() = 0;

        virtual void unlock() = 0;
};

#endif // __MUTEX_INTERFACE_H__
