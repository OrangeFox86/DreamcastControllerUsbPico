#include <hal/System/Identification.hpp>
#include "pico/unique_id.h"

void system_get_serial(char* buffer, std::uint32_t bufflen)
{
    pico_get_unique_board_id_string(buffer, bufflen);
}
