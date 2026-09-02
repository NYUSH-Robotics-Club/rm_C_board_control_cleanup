#!/usr/bin/env sh
# Compile and run hardware-independent checks with the host C compiler.
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
test_build=$(mktemp -d "${TMPDIR:-/tmp}/rm-control-tests.XXXXXX")
trap 'rm -rf "$test_build"' EXIT INT TERM

cc=${CC:-clang}
common_flags="-std=c11 -Wall -Wextra -Werror"

"$cc" $common_flags \
  -I"$repo_root/bsp/critical" \
  -I"$repo_root/modules/message_center" \
  "$repo_root/bsp/critical/bsp_critical.c" \
  "$repo_root/modules/message_center/message_center.c" \
  "$repo_root/tests/host/test_message_center.c" \
  -o "$test_build/test_message_center"

"$cc" $common_flags \
  -DROBOT_TYPE_infantry_standard \
  -I"$repo_root/config" \
  -I"$repo_root/config/robots" \
  -I"$repo_root/core/common" \
  -I"$repo_root/core/chassis" \
  "$repo_root/adapters/chassis/mecanum_chassis_strategy.c" \
  "$repo_root/adapters/chassis/swerve_chassis_strategy.c" \
  "$repo_root/adapters/chassis/omni_chassis_strategy.c" \
  "$repo_root/tests/host/test_chassis_strategies.c" \
  -o "$test_build/test_chassis_strategies"

"$cc" $common_flags \
  -DROBOT_TYPE_infantry_standard \
  -I"$repo_root/config" \
  -I"$repo_root/config/robots" \
  -I"$repo_root/core/common" \
  -I"$repo_root/core/contracts" \
  -I"$repo_root/core/motor" \
  -I"$repo_root/services/motor" \
  -I"$repo_root/bsp/critical" \
  -I"$repo_root/modules/message_center" \
  "$repo_root/bsp/critical/bsp_critical.c" \
  "$repo_root/modules/message_center/message_center.c" \
  "$repo_root/services/motor/motor_service.c" \
  "$repo_root/tests/host/test_motor_service.c" \
  -o "$test_build/test_motor_service"

"$cc" $common_flags \
  -I"$repo_root/config" \
  -I"$repo_root/core/contracts" \
  -I"$repo_root/modules/message_center" \
  "$repo_root/tests/host/test_contract_sizes.c" \
  -o "$test_build/test_contract_sizes"

"$cc" $common_flags \
  -I"$repo_root/modules/motor_protocols" \
  "$repo_root/modules/motor_protocols/dm_motor_protocol.c" \
  "$repo_root/modules/motor_protocols/lk_motor_protocol.c" \
  "$repo_root/modules/motor_protocols/benmo_motor_protocol.c" \
  "$repo_root/tests/host/test_motor_protocols.c" \
  -lm \
  -o "$test_build/test_motor_protocols"

"$cc" $common_flags \
  -I"$repo_root/application/cmd" \
  -I"$repo_root/core/common" \
  -I"$repo_root/core/contracts" \
  "$repo_root/application/cmd/command_router.c" \
  "$repo_root/tests/host/test_command_router.c" \
  -lm \
  -o "$test_build/test_command_router"

"$cc" $common_flags \
  -I"$repo_root/adapters/motor" \
  -I"$repo_root/bsp/can" \
  -I"$repo_root/bsp/time" \
  -I"$repo_root/config" \
  -I"$repo_root/core/common" \
  -I"$repo_root/core/contracts" \
  -I"$repo_root/core/motor" \
  -I"$repo_root/modules/algorithm" \
  -I"$repo_root/modules/motor_protocols" \
  "$repo_root/modules/algorithm/pid.c" \
  "$repo_root/modules/motor_protocols/dm_motor_protocol.c" \
  "$repo_root/modules/motor_protocols/lk_motor_protocol.c" \
  "$repo_root/modules/motor_protocols/benmo_motor_protocol.c" \
  "$repo_root/adapters/motor/vendor_control.c" \
  "$repo_root/adapters/motor/dm_motor_adapter.c" \
  "$repo_root/adapters/motor/lk_motor_adapter.c" \
  "$repo_root/adapters/motor/benmo_motor_adapter.c" \
  "$repo_root/tests/host/test_motor_adapters.c" \
  -lm \
  -o "$test_build/test_motor_adapters"

for robot_type in infantry_standard sentry_swerve; do
  "$cc" $common_flags \
    -D"ROBOT_TYPE_${robot_type}" \
    -I"$repo_root/config" \
    -I"$repo_root/config/robots" \
    "$repo_root/config/robot_config.c" \
    "$repo_root/config/robots/infantry_standard.c" \
    "$repo_root/config/robots/sentry_swerve.c" \
    "$repo_root/tests/host/test_robot_config.c" \
    -o "$test_build/test_robot_config_${robot_type}"
done

"$test_build/test_message_center"
"$test_build/test_chassis_strategies"
"$test_build/test_motor_service"
"$test_build/test_contract_sizes"
"$test_build/test_motor_protocols"
"$test_build/test_command_router"
"$test_build/test_motor_adapters"
"$test_build/test_robot_config_infantry_standard"
"$test_build/test_robot_config_sentry_swerve"
echo "host tests: PASS"
