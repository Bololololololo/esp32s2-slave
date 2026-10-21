/*
 * This header file declares functions for retrieving system information such as PSRAM size, CPU frequency, and heap sizes.
 * The implementation details are in the corresponding source file.
 */

#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <cstddef>
#include <cstdint>
namespace utils
{
    class SystemInfo
    {
    public:
        SystemInfo(/* args */);
        ~SystemInfo();

        std::uint32_t getCpuFreqMHz(void);
        std::size_t getTotalFreeDRAMSizeKB(void);
        std::size_t getFreeInternalDRAMSizeKB(void);
        std::size_t getTotalPsramSize(void);
        std::size_t getTotalInternalSizeKB(void);
    };
}

#endif // SYSTEM_INFO_H