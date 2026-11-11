#!/usr/bin/env python3
"""Regenerate the seed corpora under mayhem/<target>/testsuite (deterministic).

Run from the repo root: python3 mayhem/gen_seeds.py
GNAV seeds are the real 85-bit strings of upstream's tests/unit-tests/system-parameters/glonass_gnav_nav_message_test.cc;
the other formats are synthesized with the message-type / subframe-id fields set per the ICDs and the
remaining bits pseudo-random, so every decoder branch is reachable from a seed.
"""
import os
import random
import re

ROOT = os.path.dirname(os.path.abspath(__file__))
rnd = random.Random(20261111)


def out(target, name, data):
    d = os.path.join(ROOT, target, "testsuite")
    os.makedirs(d, exist_ok=True)
    with open(os.path.join(d, name), "wb") as f:
        f.write(bytes(data))


class Bits:
    """MSB-first bit stream; `first` is the 1-based ICD position."""

    def __init__(self, n, fill=True):
        self.b = [rnd.getrandbits(1) if fill else 0 for _ in range(n)]

    def set(self, first, length, value):
        for j in range(length):
            self.b[first - 1 + j] = (value >> (length - 1 - j)) & 1

    def bytes(self):
        b = self.b + [0] * (-len(self.b) % 8)
        return bytes(int("".join(map(str, b[i:i + 8])), 2) for i in range(0, len(b), 8))


# ---- GLONASS GNAV: 85-bit strings, 11 bytes each --------------------------------------------
def pack_str(s):
    s = s + "0" * (-len(s) % 8)
    return bytes(int(s[i:i + 8], 2) for i in range(0, len(s), 8))


src = open(os.path.join(ROOT, "..", "tests/unit-tests/system-parameters/glonass_gnav_nav_message_test.cc")).read()
strings = {}
for m in re.finditer(r'std::string (str\d+)\("([01]{85})"\)', src):
    strings.setdefault(m.group(1), m.group(2))
frame = [strings["str%d" % i] for i in range(1, 16) if "str%d" % i in strings]
out("fuzz_glonass_gnav", "frame_15_strings", b"".join(pack_str(s) for s in frame))
out("fuzz_glonass_gnav", "strings_1_to_4", b"".join(pack_str(s) for s in frame[:4]))
out("fuzz_glonass_gnav", "string_1", pack_str(frame[0]))
out("fuzz_glonass_gnav", "strings_5_to_15", b"".join(pack_str(s) for s in frame[4:]))

# ---- GPS LNAV: [sys|prn] + 40-byte subframes (10 LE uint32 words, 30 data bits each) ---------
def lnav_subframe(sf, tow, page=None):
    b = Bits(300)
    b.set(1, 8, 0x8B)          # TLM preamble
    b.set(31, 17, tow)
    b.set(48, 2, 0)
    b.set(50, 3, sf)
    if sf in (4, 5) and page is not None:
        b.set(61, 2, 1)        # data id
        b.set(63, 6, page)     # SV id
    raw = b.b
    words = bytearray()
    for i in range(10):
        w = int("".join(map(str, raw[30 * i:30 * i + 30])), 2)
        words += w.to_bytes(4, "little")
    return bytes(words)


for system in (0, 1):
    tag = "qzss" if system else "gps"
    hdr = bytes([(7 << 1) | system])
    out("fuzz_gps_lnav", f"{tag}_sf1_2_3", hdr + b"".join(lnav_subframe(s, 1000 + s) for s in (1, 2, 3)))
    out("fuzz_gps_lnav", f"{tag}_sf1_to_5", hdr + b"".join(lnav_subframe(s, 2000 + s, 25) for s in (1, 2, 3, 4, 5)))
    out("fuzz_gps_lnav", f"{tag}_sf4_pages", hdr + b"".join(lnav_subframe(4, 3000 + p, p) for p in (1, 18, 25, 52, 55, 56, 63)))
    out("fuzz_gps_lnav", f"{tag}_sf5_pages", hdr + b"".join(lnav_subframe(5, 4000 + p, p) for p in (1, 12, 25, 51)))

# ---- BeiDou D1/D2: [d2] + 300-bit subframes (38 bytes) --------------------------------------
def bds(fra, pnum_first, pnum_len, pnum):
    b = Bits(300)
    b.set(1, 11, 0x712)        # preamble
    b.set(16, 3, fra)
    b.set(19, 8, 100 + fra)    # SOW
    if pnum is not None:
        b.set(pnum_first, pnum_len, pnum)
    return b.bytes()[:38]


d1 = [bds(1, 0, 0, None), bds(2, 0, 0, None), bds(3, 0, 0, None)]
d1 += [bds(4, 44, 7, p) for p in (1, 5, 24)] + [bds(5, 44, 7, p) for p in (1, 7, 8, 9, 10, 11, 24)]
out("fuzz_beidou_dnav", "d1_sf1_to_3", b"\x00" + b"".join(d1[:3]))
out("fuzz_beidou_dnav", "d1_sf4_sf5_pages", b"\x00" + b"".join(d1[3:]))
out("fuzz_beidou_dnav", "d1_all", b"\x00" + b"".join(d1))
d2 = [bds(1, 43, 4, p) for p in range(1, 11)]
d2 += [bds(5, 44, 7, p) for p in (1, 2, 3, 4, 5, 6, 37, 61, 71, 91, 101, 120)]
out("fuzz_beidou_dnav", "d2_sf1_pages", b"\x01" + b"".join(d2[:10]))
out("fuzz_beidou_dnav", "d2_sf5_pages", b"\x01" + b"".join(d2[10:]))
out("fuzz_beidou_dnav", "d2_all", b"\x01" + b"".join(d2))

# ---- GPS CNAV: [qzss] + 300-bit pages -------------------------------------------------------
def cnav(prn, msg, tow):
    b = Bits(300)
    b.set(1, 8, 0x8B)
    b.set(9, 6, prn)
    b.set(15, 6, msg)
    b.set(21, 17, tow)
    return b.bytes()[:38]


for system in (0, 1):
    tag = "qzss" if system else "gps"
    msgs = [10, 11, 30, 31, 32, 33, 34, 35, 36, 37, 12, 13, 14, 15]
    out("fuzz_gps_cnav", f"{tag}_ephemeris_clock", bytes([system]) + b"".join(cnav(5, m, 100 + i) for i, m in enumerate([10, 11, 30])))
    out("fuzz_gps_cnav", f"{tag}_all_types", bytes([system]) + b"".join(cnav(5, m, 200 + i) for i, m in enumerate(msgs)))
    out("fuzz_gps_cnav", f"{tag}_clock_variants", bytes([system]) + b"".join(cnav(9, m, 300 + i) for i, m in enumerate([30, 31, 32, 33, 34, 35, 36, 37])))

# ---- RTCM 3: message body (12-bit type first) ------------------------------------------------
def rtcm3(msg_type, nbits, tweak=None):
    b = Bits(nbits)
    b.set(1, 12, msg_type)
    b.set(13, 12, 7)           # station id
    if tweak:
        tweak(b)
    return b.bytes()


def msm(msg_type, nsat, nsig, per_sat, per_cell):
    nbits = 169 + nsat * nsig + nsat * per_sat + nsat * nsig * per_cell
    def t(b):
        b.set(25, 30, 345600000)   # epoch
        b.set(61, 3, 2)
        b.set(73, 2, 0)
        b.set(75, 2, 0)
        b.set(78, 3, 0)
        b.set(81, 64, ((1 << 63) | (1 << 61) | (1 << 58) | (1 << 40)) if nsat == 4 else (1 << 63))
        b.set(145, 32, (1 << 31) | (1 << 30) if nsig == 2 else (1 << 31))
        for k in range(nsat * nsig):
            b.set(177 + k, 1, 1)
    return rtcm3(msg_type, nbits, t)


payloads = {
    "type1005_station": rtcm3(1005, 152),
    "type1006_station_height": rtcm3(1006, 168),
    "type1007_antenna": rtcm3(1007, 8 * 20, lambda b: b.set(37, 8, 5)),
    "type1008_antenna_serial": rtcm3(1008, 8 * 30, lambda b: (b.set(37, 8, 5), b.set(45 + 40, 8, 4))),
    "type1019_gps_eph": rtcm3(1019, 488),
    "type1020_glo_eph": rtcm3(1020, 360),
    "type1033_receiver": rtcm3(1033, 8 * 40, lambda b: (b.set(37, 8, 4), b.set(45 + 32, 8, 4))),
    "type1042_bds_eph": rtcm3(1042, 499),
    "type1044_qzss_eph": rtcm3(1044, 473),
    "type1045_gal_fnav": rtcm3(1045, 496),
    "type1046_gal_inav": rtcm3(1046, 504),
    "type1002_gps_l1": rtcm3(1002, 64 + 4 * 74, lambda b: b.set(37, 5, 4)),
    "type1004_gps_l1l2": rtcm3(1004, 64 + 4 * 125, lambda b: b.set(37, 5, 4)),
    "type1010_glo_l1": rtcm3(1010, 61 + 4 * 79, lambda b: b.set(37, 5, 4)),
    "type1012_glo_l1l2": rtcm3(1012, 61 + 4 * 130, lambda b: b.set(37, 5, 4)),
    "type1029_text": rtcm3(1029, 8 * 30, lambda b: b.set(65, 8, 8)),
    "type1230_glo_bias": rtcm3(1230, 8 * 12),
    "type1057_ssr_gps_orbit": rtcm3(1057, 8 * 40),
    "type1058_ssr_gps_clock": rtcm3(1058, 8 * 40),
    "type1059_ssr_gps_cbias": rtcm3(1059, 8 * 40),
    "type1060_ssr_gps_orbclk": rtcm3(1060, 8 * 60),
    "type1061_ssr_gps_ura": rtcm3(1061, 8 * 40),
    "type1062_ssr_gps_hrclk": rtcm3(1062, 8 * 40),
    "type1063_ssr_glo_orbit": rtcm3(1063, 8 * 40),
    "type4072_proprietary": rtcm3(4072, 8 * 40),
}
for t in (1074, 1075, 1076, 1077, 1084, 1094, 1104, 1114, 1124):
    payloads[f"msm{t}_4sat_2sig"] = msm(t, 4, 2, 36 if t % 10 in (4, 5) else 36, 42)
    payloads[f"msm{t}_1sat_1sig"] = msm(t, 1, 1, 36, 42)
for k, v in payloads.items():
    out("fuzz_rtcm3", k, v)

# ---- RTCM 2: 24-bit data words of one message (words 1-2 are the header) ---------------------
def rtcm2(msg_type, nwords, staid=7):
    b = Bits(24 * (2 + nwords))
    b.set(1, 8, 0x66)
    b.set(9, 6, msg_type)
    b.set(15, 10, staid)
    b.set(25, 13, 100)         # modified z-count
    b.set(38, 3, 1)
    b.set(41, 5, nwords)
    return b.bytes()


for t, n in ((1, 9), (3, 4), (9, 6), (14, 1), (16, 8), (17, 8), (18, 12), (19, 12), (22, 3), (31, 6), (32, 6), (34, 4), (36, 8), (59, 2)):
    out("fuzz_rtcm2", f"type{t}", rtcm2(t, n))
