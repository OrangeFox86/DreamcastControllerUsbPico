#ifndef __CRITICAL_SECTION_MUTEX_H__
#define __CRITICAL_SECTION_MUTEX_H__

#include "MutexInterface.hpp"
#include "pico/critical_section.h"

class CriticalSectionMutex : public MutexInterface
{
    public:
        CriticalSectionMutex()
        {
            critical_section_init(&mCriticalSection);
        }

        virtual ~CriticalSectionMutex() {}

        virtual void lock() final
        {
            critical_section_enter_blocking(&mCriticalSection);
        }

        virtual void unlock() final
        {
            critical_section_exit(&mCriticalSection);
        }

    private:
        critical_section_t mCriticalSection;
};

#endif // __CRITICAL_SECTION_MUTEX_H__
