#include <cstdint>

// size includes null terminator
#define SERIAL_SIZE 17

void system_get_serial(char* buffer, std::uint32_t bufflen);
