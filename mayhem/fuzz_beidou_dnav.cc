// Fuzz the BeiDou D1/D2 NAV subframe decoders. First input byte: bit 0 selects D1 or D2; then a
// sequence of 300-bit subframes (38 bytes each, the trailing 4 bits ignored) for one decoder.
#include "beidou_dnav_navigation_message.h"
#include "fuzz_common.h"

extern "C" int LLVMFuzzerInitialize(int* /*argc*/, char*** /*argv*/)
{
    mayhem_quiet_glog();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    constexpr size_t kSubframeBytes = 38;
    if (size < 1 + kSubframeBytes || size > 1 + 24 * kSubframeBytes)
        {
            return -1;
        }
    const bool d2 = (data[0] & 1) != 0;
    Beidou_Dnav_Navigation_Message nav;
    for (size_t off = 1; off + kSubframeBytes <= size; off += kSubframeBytes)
        {
            const std::string bits = mayhem_bits_to_string(data + off, 300);
            if (d2)
                {
                    nav.d2_subframe_decoder(bits);
                }
            else
                {
                    nav.d1_subframe_decoder(bits);
                }
            nav.get_ephemeris();
            nav.get_iono();
            nav.get_utc_model();
            nav.get_almanac();
            nav.get_differential_corrections();
        }
    return 0;
}
