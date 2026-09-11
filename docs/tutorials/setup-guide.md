# 环境、构建与烧录

当前 Windows/macOS/Linux 安装与日常命令已统一到
[快速开始](../quickstart.md)。请从那里完成首次车型选择和本机工具路径配置。

```text
just doctor
just build
just flash-plan
```

默认推荐 OpenOCD + ST-Link/SWD；CubeProgrammer 不推荐用于日常开发且不是依赖。
bootstrap 安装最新发布版，构建记录实际版本，不再锁定历史文档版本。
只有明确决定执行实际设备写入时才运行 `just flash`；校验后
不复位运行。VS Code 任务使用 Firmware: Doctor / Build / Flash。

两车型的当前 ARM 构建、静态容量与验证边界见
[环境验证记录](../environment-validation.md)。不要用同名 NYUSH_Infantry.elf
判断车型，要核对独立构建目录、CMake cache 和 manifest。

2026-09-07 核实当前 main 未调用 RobotRtos_Start，SVC/PendSV 为空、SysTick 仅更新
HAL tick。按用户要求保持裸机启动，先不调用 RTOS。原文“RTOS 已启用”及旧产物数字
与当前提交不一致，不能当成本机验证结果。成功编译不等于调度器/机器人功能通过。

原有 CMakePresets.json 和 cmake/gcc-arm-none-eabi.cmake 保留供需要时手动使用；
just 自动采用原 toolchain 并明确传递车型、构建类型和 Ninja 路径，不改 MCU、ABI
或链接脚本。主机测试 tests/host/run_tests.sh 需要适合的 Unix 主机环境，
未作为 Windows 原生构建依赖，也不替代 ARM 链接检查。
