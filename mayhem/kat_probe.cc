// Known-answer probe for test.sh: prints one `name=value` line per check.
#include "glonass_gnav_navigation_message.h"
#include "rtklib_rtkcmn.h"

#include <bitset>
#include <cstdio>
#include <cstring>
#include <string>

int main()
{
    const char* check = "123456789";
    std::printf("crc24q_123456789=%06X\n", rtk_crc24q(reinterpret_cast<const unsigned char*>(check), 9));
    std::printf("crc16_123456789=%04X\n", rtk_crc16(reinterpret_cast<const unsigned char*>(check), 9));

    Glonass_Gnav_Navigation_Message nav;
    std::bitset<GLONASS_GNAV_STRING_BITS> good(std::string("0010100100001100000000000000000000000000110011110001100000000000000001100100011000000"));
    std::bitset<GLONASS_GNAV_STRING_BITS> bad(std::string("0111100100001100000000000000000000000000110011110001100000000000000001100100011000000"));
    std::printf("gnav_crc_good=%d\n", nav.CRC_test(good) ? 1 : 0);
    std::printf("gnav_crc_bad=%d\n", nav.CRC_test(bad) ? 1 : 0);
    return 0;
}
