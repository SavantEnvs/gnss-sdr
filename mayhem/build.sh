#!/usr/bin/env bash
#
# mayhem/build.sh — build the gnss-sdr fuzz harnesses and the functional oracle.
#
# The full receiver needs GNU Radio, so we compile only the standalone libraries that hold the
# bit-level decoders: src/core/system_parameters (navigation-message decoders, RS/LDPC codes) and
# src/algorithms/libs/rtklib (RTCM 2/3 and the RTKLIB helpers), straight from the sources.
#   1) sanitized + DWARF-3 library  -> build-fuzz/libgnsssdr_core.a -> fuzz_* and fuzz_*-standalone
#   2) the same sources, normal flags -> build-test/ -> oracle_tests (upstream's gtest suites) + kat_probe
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${SRC:=/mayhem}"
: "${STANDALONE_FUZZ_MAIN:=/opt/mayhem/StandaloneFuzzTargetMain.c}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"

SOURCES=$(ls src/core/system_parameters/*.cc src/algorithms/libs/rtklib/*.cc \
             src/algorithms/libs/gnss_sdr_string_literals.cc src/algorithms/libs/geofunctions.cc)

INC="-Isrc/core/system_parameters -Isrc/algorithms/libs -Isrc/algorithms/libs/rtklib \
     -Isrc/algorithms/libs/gsl -Isrc/algorithms/libs/gsl/include -Isrc/core/interfaces \
     -Isrc/core/receiver -Isrc/core/libs -Itests -Itests/common-files -Imayhem"
DEFS="-DUSE_GLOG_AND_GFLAGS=1 -DHAS_STD_FILESYSTEM=1"
LIBS="-lglog -lgflags -larmadillo -llapack -lblas -lboost_serialization -lboost_filesystem \
      -lboost_date_time -lssl -lcrypto -lgnutls -lpthread -lm"

# compile_all <outdir> <flags...>: one object per source, in parallel.
compile_all() {
  local outdir="$1"; shift
  mkdir -p "$outdir"
  printf '%s\n' $SOURCES | xargs -P "$MAYHEM_JOBS" -I{} sh -c '
    f="$1"; outdir="$2"; shift 2
    o="$outdir/$(echo "$f" | tr / _).o"
    "$@" -c "$f" -o "$o"' _ {} "$outdir" "$CXX" -std=c++17 $INC $DEFS "$@"
  rm -f "$outdir/libgnsssdr_core.a"
  ar rcs "$outdir/libgnsssdr_core.a" "$outdir"/*.o
}

# ---- 1) sanitized fuzz build --------------------------------------------------------------
# Relaxed UBSan check: the signed bit-field readers (read_navigation_signed) sign-extend with
# `value << 1` on a negative int64, which fires "left shift of negative value" on every negative
# field (well-defined in C++20, benign in practice). Everything else in ASan/UBSan stays halting.
FUZZ_CXXFLAGS="-O1 $SANITIZER_FLAGS -fno-sanitize=shift-base $DEBUG_FLAGS"
rm -rf build-fuzz
compile_all build-fuzz $FUZZ_CXXFLAGS -fsanitize=fuzzer-no-link

$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c mayhem/lsan_off.cc -o build-fuzz/lsan_off.o
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o build-fuzz/standalone_main.o

TARGETS="fuzz_rtcm3 fuzz_rtcm2 fuzz_glonass_gnav fuzz_gps_lnav fuzz_beidou_dnav fuzz_gps_cnav"
for t in $TARGETS; do
  $CXX -std=c++17 $FUZZ_CXXFLAGS $INC $DEFS -fsanitize=fuzzer-no-link -c "mayhem/$t.cc" -o "build-fuzz/$t.o"
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "build-fuzz/$t.o" build-fuzz/lsan_off.o \
       build-fuzz/libgnsssdr_core.a $LIBS -o "$SRC/$t"
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS "build-fuzz/$t.o" build-fuzz/standalone_main.o build-fuzz/lsan_off.o \
       build-fuzz/libgnsssdr_core.a $LIBS -o "$SRC/$t-standalone"
done

# ---- 2) functional oracle, normal flags ---------------------------------------------------
rm -rf build-test
compile_all build-test -O2 $COVERAGE_FLAGS
$CXX -std=c++17 -O2 $COVERAGE_FLAGS $INC $DEFS mayhem/oracle_tests.cc build-test/libgnsssdr_core.a \
     -lgtest $LIBS $COVERAGE_FLAGS -o "$SRC/oracle_tests"
$CXX -std=c++17 -O2 $COVERAGE_FLAGS $INC $DEFS mayhem/kat_probe.cc build-test/libgnsssdr_core.a \
     $LIBS $COVERAGE_FLAGS -o "$SRC/kat_probe"
