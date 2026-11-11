// Fuzz the GPS/QZSS LNAV subframe decoder. First input byte: bit 0 selects GPS/QZSS, the rest
// selects the PRN; then a sequence of 40-byte subframes (10 words x 4 bytes) for one decoder.
#include "fuzz_common.h"
#include "gps_navigation_message.h"

extern "C" int LLVMFuzzerInitialize(int* /*argc*/, char*** /*argv*/)
{
    mayhem_quiet_glog();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    constexpr size_t kSubframeBytes = 40;
    if (size < 1 + kSubframeBytes || size > 1 + 16 * kSubframeBytes)
        {
            return -1;
        }
    Gps_Navigation_Message nav((data[0] & 1) ? LnavSystem::QZSS : LnavSystem::GPS);
    nav.set_satellite_PRN(data[0] >> 1);
    for (size_t off = 1; off + kSubframeBytes <= size; off += kSubframeBytes)
        {
            nav.subframe_decoder(reinterpret_cast<const char*>(data + off));
            nav.satellite_validation();
            nav.get_ephemeris();
            nav.get_iono();
            nav.get_almanac();
            nav.get_utc_model();
        }
    return 0;
}
