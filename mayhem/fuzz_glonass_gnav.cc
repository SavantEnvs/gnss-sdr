// Fuzz the GLONASS GNAV string decoder. The input is a sequence of 85-bit strings (11 bytes
// each, the trailing 3 bits ignored) fed to one fresh decoder, as strings of one frame would be.
#include "fuzz_common.h"
#include "glonass_gnav_navigation_message.h"

extern "C" int LLVMFuzzerInitialize(int* /*argc*/, char*** /*argv*/)
{
    mayhem_quiet_glog();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    constexpr size_t kStringBytes = 11;
    if (size < kStringBytes || size > 32 * kStringBytes)
        {
            return -1;
        }
    Glonass_Gnav_Navigation_Message nav;
    for (size_t off = 0; off + kStringBytes <= size; off += kStringBytes)
        {
            nav.string_decoder(mayhem_bits_to_string(data + off, GLONASS_GNAV_STRING_BITS));
            nav.get_ephemeris();
            nav.get_utc_model();
            nav.get_almanac(1 + (off / kStringBytes) % 24);
        }
    return 0;
}
