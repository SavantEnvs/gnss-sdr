#!/usr/bin/env bash
#
# mayhem/test.sh — RUN the functional oracle built by mayhem/build.sh (never compiles).
#  * oracle_tests: gnss-sdr's own system-parameters gtest suites (GLONASS GNAV, BeiDou D1/D2 and
#    CNAV-1/2, GPS CNAV, QZSS LNAV, Galileo RS/ISM, ...), linked against the real decoders.
#  * kat_probe: direct known-answer checks (CRC-24Q/CRC-16 check values, GNAV Hamming check).
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SRC:=/mayhem}"
cd "$SRC"

emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

passed=0
failed=0

for exe in oracle_tests kat_probe; do
  if [ ! -x "$SRC/$exe" ]; then
    echo "FAIL: $SRC/$exe missing (mayhem/build.sh should have built it)" >&2
    failed=$((failed + 1))
  fi
done

# --- gtest suites: parse the summary line; a neutered binary prints nothing, which fails ---
if [ -x "$SRC/oracle_tests" ]; then
  out=$(timeout 600 "$SRC/oracle_tests" 2>&1); rc=$?
  echo "$out" | tail -n 8
  g_pass=$(printf '%s\n' "$out" | sed -n 's/^\[  PASSED  \] \([0-9][0-9]*\) test.*/\1/p' | tail -n 1)
  g_total=$(printf '%s\n' "$out" | sed -n 's/^\[==========\] \([0-9][0-9]*\) tests\{0,1\} from .* ran\..*/\1/p' | tail -n 1)
  g_fail=$(printf '%s\n' "$out" | sed -n 's/^\[  FAILED  \] \([0-9][0-9]*\) test.*, listed below.*/\1/p' | tail -n 1)
  if [ -z "$g_pass" ] || [ -z "$g_total" ] || [ "$g_total" -lt 100 ] || [ "$rc" -ne 0 ] || [ "$g_pass" -ne "$g_total" ]; then
    echo "FAIL: gtest summary missing or incomplete (passed='$g_pass' total='$g_total' rc=$rc)" >&2
    failed=$((failed + ${g_fail:-1}))
    passed=$((passed + ${g_pass:-0}))
  else
    passed=$((passed + g_pass))
  fi
fi

# --- direct known-answer probes ---
if [ -x "$SRC/kat_probe" ]; then
  kat=$(timeout 60 "$SRC/kat_probe" 2>&1)
  echo "$kat"
  check() {
    if printf '%s\n' "$kat" | grep -qx "$1"; then passed=$((passed + 1)); else echo "FAIL: expected '$1'" >&2; failed=$((failed + 1)); fi
  }
  check "crc24q_123456789=CDE703"
  check "crc16_123456789=31C3"
  check "gnav_crc_good=1"
  check "gnav_crc_bad=0"
fi

emit_ctrf "gnss-sdr-system-parameters" "$passed" "$failed" 0
