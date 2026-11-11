// Fuzz the GPS/QZSS CNAV page decoder. First input byte: bit 0 selects GPS/QZSS; then a sequence
// of 300-bit pages (38 bytes each, the trailing 4 bits ignored) for one decoder.
#include "fuzz_common.h"
#include "gps_cnav_navigation_message.h"

#include <bitset>

extern "C" int LLVMFuzzerInitialize(int* /*argc*/, char*** /*argv*/)
{
    mayhem_quiet_glog();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    constexpr size_t kPageBytes = 38;
    if (size < 1 + kPageBytes || size > 1 + 24 * kPageBytes)
        {
            return -1;
        }
    Gps_CNAV_Navigation_Message nav((data[0] & 1) ? CnavSystem::QZSS : CnavSystem::GPS);
    for (size_t off = 1; off + kPageBytes <= size; off += kPageBytes)
        {
            const std::bitset<GPS_CNAV_DATA_PAGE_BITS> page(mayhem_bits_to_string(data + off, GPS_CNAV_DATA_PAGE_BITS));
            nav.decode_page(page);
            nav.get_ephemeris();
            nav.get_iono();
            nav.get_utc_model();
            nav.get_eop();
        }
    return 0;
}
