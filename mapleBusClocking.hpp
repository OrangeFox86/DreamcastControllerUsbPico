#ifndef __CLOCKING_H__
#define __CLOCKING_H__

#include <stdint.h>
#include "MapleBus.hpp"

void write(const MapleBus& mapleBus, uint8_t* bytes, uint32_t len);

void clocking_init();


#endif // __CLOCKING_H__
