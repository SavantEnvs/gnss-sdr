// Fuzz the RTCM 3 message decoder (RTKLIB derived) in gnss-sdr.
// The input is the message body (12-bit message type first). The 3-byte frame header and the
// CRC24Q are not part of the input: decode_rtcm3() reads neither, it only needs rtcm->len.
#include "fuzz_common.h"
#include "rtklib.h"
#include "rtklib_rtcm.h"
#include "rtklib_rtkcmn.h"
#include "rtklib_rtcm3.h"

#include <cstdlib>
#include <cstring>

extern "C" int LLVMFuzzerInitialize(int* /*argc*/, char*** /*argv*/)
{
    mayhem_quiet_glog();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < 2 || size > 1023)
        {
            return -1;
        }
    auto* rtcm = static_cast<rtcm_t*>(calloc(1, sizeof(rtcm_t)));
    if (rtcm == nullptr || init_rtcm(rtcm) == 0)
        {
            free(rtcm);
            return -1;
        }
    rtcm->time = gpst2time(2300, 0.0);
    std::memcpy(rtcm->buff + 3, data, size);
    rtcm->len = static_cast<int>(size) + 3;
    decode_rtcm3(rtcm);
    free_rtcm(rtcm);
    free(rtcm);
    return 0;
}
