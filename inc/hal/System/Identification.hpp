#include <cstdint>

// size includes null terminator
extern const std::uint32_t SERIAL_SIZE;

void system_get_serial(char* buffer, std::uint32_t bufflen);
