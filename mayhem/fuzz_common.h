// Shared helpers for the gnss-sdr harnesses (silence glog, unpack bytes to '0'/'1' strings).
#ifndef MAYHEM_FUZZ_COMMON_H
#define MAYHEM_FUZZ_COMMON_H

#include <glog/logging.h>

#include <cstddef>
#include <cstdint>
#include <string>

// glog would otherwise open log files under /tmp; send everything to stderr and keep it quiet.
inline void mayhem_quiet_glog()
{
    FLAGS_logtostderr = true;
    FLAGS_minloglevel = 3;
}

// Unpack the first `nbits` bits of `data` (MSB first) into a string of '0'/'1' characters.
inline std::string mayhem_bits_to_string(const uint8_t* data, size_t nbits)
{
    std::string s(nbits, '0');
    for (size_t i = 0; i < nbits; i++)
        {
            if ((data[i / 8] >> (7 - (i % 8))) & 1)
                {
                    s[i] = '1';
                }
        }
    return s;
}

#endif
