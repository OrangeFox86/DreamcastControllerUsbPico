#ifndef __CRITICAL_SECTION_MUTEX_H__
#define __CRITICAL_SECTION_MUTEX_H__

#include "hal/System/MutexInterface.hpp"
#include "pico/critical_section.h"

class CriticalSectionMutex : public MutexInterface
{
    public:
        CriticalSectionMutex();

        virtual ~CriticalSectionMutex();

        virtual void lock() final;

        virtual void unlock() final;

    private:
        critical_section_t mCriticalSection;
};

#endif // __CRITICAL_SECTION_MUTEX_H__
