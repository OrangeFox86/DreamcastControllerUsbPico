#include "hal/System/timing.hpp"

#include "pico/stdlib.h"

uint64_t get_time_us()
{
    return time_us_64();
}
