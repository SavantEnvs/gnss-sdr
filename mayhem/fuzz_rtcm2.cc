// Fuzz the RTCM 2 message decoder (RTKLIB derived) in gnss-sdr.
// The input is the sequence of 24-bit data words of one message (parity bits already stripped,
// as input_rtcm2() stores them); the message length is taken from the header word exactly as
// input_rtcm2() derives it.
#include "fuzz_common.h"
#include "rtklib.h"
#include "rtklib_rtcm.h"
#include "rtklib_rtkcmn.h"
#include "rtklib_rtcm2.h"

#include <cstdlib>
#include <cstring>

extern "C" int LLVMFuzzerInitialize(int* /*argc*/, char*** /*argv*/)
{
    mayhem_quiet_glog();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < 6 || size > 99)
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
    std::memcpy(rtcm->buff, data, size);
    rtcm->len = (rtcm->buff[5] >> 3) * 3 + 6;
    decode_rtcm2(rtcm);
    free_rtcm(rtcm);
    free(rtcm);
    return 0;
}
