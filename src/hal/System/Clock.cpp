#include "Clock.hpp"

#include "pico/stdlib.h"

uint64_t Clock::getTimeUs()
{
    return time_us_64();
}
