// Functional oracle: a gtest runner over gnss-sdr's own system-parameters unit tests (nav-message
// decoders, GNAV/BeiDou/GPS CNAV/QZSS, RS and LDPC codes) and the rtklib helpers they use.
// Upstream aggregates these test files into one binary the same way (tests/unit_tests_system_parameters.cc)
// but needs the whole receiver; this runner links only the standalone libraries.
#include "rtklib.h"
#include "rtklib_conversions.h"
#include "rtklib_rtcm.h"
#include "rtklib_rtkcmn.h"

#include <glog/logging.h>
#include <gtest/gtest.h>

#include "unit-tests/system-parameters/beidou_bdgim_test.cc"
#include "unit-tests/system-parameters/beidou_cnav1_ldpc_test.cc"
#include "unit-tests/system-parameters/beidou_cnav1_navigation_message_test.cc"
#include "unit-tests/system-parameters/beidou_cnav2_ldpc_test.cc"
#include "unit-tests/system-parameters/beidou_cnav2_navigation_message_test.cc"
#include "unit-tests/system-parameters/beidou_dnav_navigation_message_test.cc"
#include "unit-tests/system-parameters/galileo_e1b_reed_solomon_test.cc"
#include "unit-tests/system-parameters/galileo_e6b_reed_solomon_test.cc"
#include "unit-tests/system-parameters/galileo_ism_test.cc"
#include "unit-tests/system-parameters/glonass_gnav_crc_test.cc"
#include "unit-tests/system-parameters/glonass_gnav_ephemeris_test.cc"
#include "unit-tests/system-parameters/glonass_gnav_nav_message_test.cc"
#include "unit-tests/system-parameters/gnss_ephemeris_posvel_test.cc"
#include "unit-tests/system-parameters/gps_cnav_navigation_message_test.cc"
#include "unit-tests/system-parameters/qzss_lnav_navigation_message_test.cc"

int main(int argc, char** argv)
{
    FLAGS_logtostderr = true;
    FLAGS_minloglevel = 3;
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
