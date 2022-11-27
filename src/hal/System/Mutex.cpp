#include "Mutex.hpp"

Mutex::Mutex()
{
    mutex_init(&mMutex);
}

Mutex::~Mutex() {}

void Mutex::lock()
{
    mutex_enter_blocking(&mMutex);
}

void Mutex::unlock()
{
    mutex_exit(&mMutex);
}
