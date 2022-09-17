#include "CriticalSectionMutex.hpp"

CriticalSectionMutex::CriticalSectionMutex()
{
    critical_section_init(&mCriticalSection);
}

CriticalSectionMutex::~CriticalSectionMutex() {}

void CriticalSectionMutex::lock()
{
    critical_section_enter_blocking(&mCriticalSection);
}

void CriticalSectionMutex::unlock()
{
    critical_section_exit(&mCriticalSection);
}
