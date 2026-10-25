#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_dir=${TMPDIR:-/tmp/opencode}/charybdis-diag-host-tests
mkdir -p "$build_dir"

cc=${CC:-cc}
common_flags="-std=c11 -Wall -Wextra -Werror -pedantic"

"$cc" $common_flags -I"$root/src" \
  "$root/src/diag_stats.c" "$root/tests/test_diag_stats.c" \
  -o "$build_dir/test_diag_stats"
"$build_dir/test_diag_stats"

"$cc" $common_flags -pthread \
  -DCHARYBDIS_DIAG_CONFIG_REV=\"host-config\" \
  -DCHARYBDIS_DIAG_ZMK_REV=\"host-zmk\" \
  -DCHARYBDIS_DIAG_ZEPHYR_REV=\"host-zephyr\" \
  -DCHARYBDIS_DIAG_BUILD_TYPE=\"host-test\" \
  -DCHARYBDIS_DIAG_COMPILER=\"host-cc\" \
  -I"$root/tests/stubs" -I"$root/src" \
  "$root/src/diag_stats.c" "$root/tests/test_wrappers.c" \
  -o "$build_dir/test_wrappers"
"$build_dir/test_wrappers"

"$cc" $common_flags -fno-inline -no-pie -I"$root/tests/native_wrap" \
  "$root/tests/native_wrap/api.c" \
  "$root/tests/native_wrap/producers.c" \
  "$root/tests/native_wrap/wrappers.c" \
  "$root/tests/native_wrap/main.c" \
  -Wl,--wrap=input_report -Wl,--wrap=zmk_hog_send_mouse_report \
  -o "$build_dir/test_postlink_elf"
"$build_dir/test_postlink_elf"
python3 "$root/scripts/verify_postlink.py" \
  --elf "$build_dir/test_postlink_elf" \
  --nm "$(command -v nm)" \
  --objdump "$(command -v objdump)"

cmake -S "$root/tests/cmake_postbuild" -B "$build_dir/cmake-postbuild"
grep -R -F "$root/scripts/verify_postlink.py" "$build_dir/cmake-postbuild" >/dev/null
grep -R -F "$build_dir/cmake-postbuild/zephyr_final" "$build_dir/cmake-postbuild" >/dev/null

cmake -S "$root/tests/cmake_disabled" -B "$build_dir/cmake-disabled"
cmake --build "$build_dir/cmake-disabled" --target zephyr_final
if nm "$build_dir/cmake-disabled/zephyr_final" | grep -E '__wrap_(input_report|zmk_hog_send_mouse_report)'; then
  echo "disabled diagnostic target unexpectedly contains wrapper symbols" >&2
  exit 1
fi

PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -v \
  -s "$root/tests" -p 'test_postlink.py'
