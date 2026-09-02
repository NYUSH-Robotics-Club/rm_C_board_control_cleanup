# 环境、构建与烧录

## 需要的工具

- CMake 3.22 或更高；
- Ninja；
- ARM GNU Toolchain（`arm-none-eabi-gcc`）；
- STM32CubeProgrammer；
- 可选：VS Code、CMake Tools、C/C++ Extension Pack、STM32CubeMX。

当前这台 Apple Silicon Mac 使用用户级安装，不依赖 Homebrew：CMake 4.4.3 和
Ninja 1.13.2 位于 Python 用户目录，Arm GNU Toolchain 14.3.Rel1 位于
`~/.local/opt/`，PATH 已写入 `~/.zprofile`。CubeProgrammer 仍需在实际烧录前
从 ST 官方安装。新终端中确认：

```bash
cmake --version
ninja --version
arm-none-eabi-gcc --version
```

## 构建

工程只开放两个 `ROBOT_TYPE`：

```bash
# 步兵麦轮
cmake -S . -B build/infantry \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake \
  -DCMAKE_BUILD_TYPE=Debug \
  -DROBOT_TYPE=infantry_standard
cmake --build build/infantry --parallel

# 当前哨兵舵轮
cmake -S . -B build/sentry \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake \
  -DCMAKE_BUILD_TYPE=Debug \
  -DROBOT_TYPE=sentry_swerve
cmake --build build/sentry --parallel
```

也可用 `cmake --preset Debug`，默认车型是 `infantry_standard`。切车型应使用独立
build 目录，避免旧缓存混入。

预期产物是 `NYUSH_Infantry.elf`，链接阶段还会生成 map。当前验证产物位于
`build/infantry_arm/` 和 `build/sentry_arm/`；本次另用 `arm-none-eabi-objcopy`
从最新 ELF 生成了可供其他烧录工具使用的 `.bin` 和 `.hex`。CMake
当前不会自动重生这两种文件。只有两个目标都完整编译链接，才能发现 FreeRTOS
端口、静态库顺序和车型条件编译问题。

## 当前 ELF 静态检查结果（2026-09-02）

| 车型 | FLASH | RAM | 结果 |
|---|---:|---:|---|
| `infantry_standard` | 83,544 B / 1 MiB（7.97%） | 52,392 B / 128 KiB（39.97%） | 合格 |
| `sentry_swerve` | 84,752 B / 1 MiB（8.08%） | 52,392 B / 128 KiB（39.97%） | 合格 |

两份 ELF 都是 ARM EABI5、Thumb-2、VFPv4-D16 hard-float，向量表从
`0x08000000` 开始，初始 MSP 为 `0x20020000`，Reset 入口带 Thumb 位；
SVC/PendSV/SysTick 指向本项目的 FreeRTOS 转接，且没有未解析符号。
这里的“合格”只表示编译、链接、格式、地址和静态容量检查通过，不表示已在板上
验证调度器、CAN、电机或云台。

## 主机测试

```bash
./tests/host/run_tests.sh
```

它覆盖消息中心、底盘策略、电机服务、消息尺寸和 DM/本末/瓴控协议字节布局，
不覆盖 ARM 指令、中断、链接脚本、HAL 和真实硬件。

## 使用 STM32CubeProgrammer 烧录

1. 断开电机主电源，保留控制板供电和可靠急停。
2. 用 ST-Link 连接 SWDIO、SWCLK、GND 和参考电压。
3. CubeProgrammer 选择 ST-Link，连接芯片并确认识别为 STM32F407。
4. 选择刚构建的 ELF，执行下载和校验，然后复位运行。
5. 观察 LED 启动顺序和 USB 日志；第一次 RTOS 固件先不接新增厂商电机。

若用 USB DFU，必须确认板卡 Boot0/DFU 接线和目标地址；本项目不应假定所有 C 板
都已配置同一 DFU 流程。VS Code 下载任务本质上仍需指向正确 ELF 和烧录工具。

## 烧录前清单

- `ROBOT_TYPE` 与实车一致；
- 两车型 ARM 构建、主机测试都通过；
- map 中 RAM/Flash 未超限，FreeRTOS 静态栈可解释；
- CAN 波特率、终端、电机 ID、方向和限幅已核对；
- 新电机配置从 `DISABLED` 开始；
- 云台可自由转动，首帧反馈和上电锁存日志可见；
- 准备物理急停，首次只做架空、小电流测试。

## 常见问题

- 找不到编译器：把 ARM 工具链 `bin` 加入 PATH，再删掉失败的 build 目录重配。
- 切车型没有生效：不要复用同一 CMake 缓存。
- ELF 有但无法运行：检查目标芯片、浮点 ABI、链接脚本、向量表和 SVC/PendSV。
- 绿色 LED 后无控制：确认调度器是否启动、SysTick 是否进入 FreeRTOS、任务是否
  栈溢出或进入 HardFault。
- USB 有日志但电机不动：先看 CAN RX 计数、配置校验返回和控制模式，不要直接
  增大电流。
