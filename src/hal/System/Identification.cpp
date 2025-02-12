#include <hal/System/Identification.hpp>
#include "pico/unique_id.h"

const std::uint32_t SERIAL_SIZE = (PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1);

void system_get_serial(char* buffer, std::uint32_t bufflen)
{
    pico_get_unique_board_id_string(buffer, bufflen);
}
