# 环境验证记录

## 2026-09-09 · 用户授权后的首次 OpenOCD 实际烧录通过

- 执行 `source tools/activate.sh`、`just flash`，当前车型 infantry_standard、Debug。
  构建与 ELF 检查通过，FLASH/RAM 127040/47800 B；目标 ID 0x413、1024 KiB。
- 在同一探针会话中完成身份检查、reset halt、再次身份检查、Flash 擦写和
  verify_image；退出码 0，并输出 FIRMWARE_VERIFY_OK 与 FIRMWARE_FLASH_OK。
- ELF：`build/linux-aarch64-306202d6/inf-debug/NYUSH_Infantry.elf`；SHA-256：
  `a387ca0150ff359f2dac2e4c0a32f687e3697ba32b56847567449a43bfa7b231`。
  详细日志：同目录 `openocd-flash.log`，后续同车型烧录可能覆盖该日志。
- run_after=false，校验后未复位运行或 resume，目标保持暂停。旧 Flash 对应扇区已被
  新固件替换，本次未备份旧镜像；新镜像完整校验成功。未改保护位或升级探针。
- 本节更新下方早期“写入未验证”的状态；没有测试程序启动、USB 日志、实际消息回调
  或机器人功能，哨兵也尚未实机烧录。文档 diff 检查通过，冻结底层和业务代码未改。

## 2026-09-09 · SWD 实机身份检查与日志入口核对

- OpenOCD 通过当前 ST-LINK/V2.1 建立 SWD 连接：目标电压 3.232718 V，
  DPIDR 0x2ba01477，Cortex-M4 r0p1，设备 ID 0x413，Flash 1024 KiB。
  探针 V2J37M26；请求 2000 kHz，探针实际选用 1800 kHz，检查退出码 0。
  家族 ID 不独立证明精确型号/封装，板卡仍按用户确认的 DJI C 板处理。
- 本次手动诊断清空 examine-end 回调后执行 init、fw_check_target、shutdown，
  未发送 halt/reset/erase/program/resume；没有进行写入、校验或机器人功能试验。
  建立 SWD 调试连接不能称为完全无调试副作用。以下早期“SWD 未连接”结论已被本节更新。
- OS 只枚举到 ST-Link 自带 VCP `/dev/ttyACM0`（by-id 以 STM32_STLink 开头），
  没有 C 板自身 USB CDC。源码日志走 C 板 USB，并不走 ST-Link UART。
  当前只能确认读取日志的软件可用，尚未读到目标日志或实际回调数据。
- 工具链 GDB 可启动；新增手动回调断点说明，但未实机连接 GDB、暂停或运行目标。
  默认仅 GIM 标签开启，不存在自动导出的全量消息/电机反馈流。
- 修正文档中 FreeRTOS 已启用的旧结论：当前 main 裸机派发，预留运行时未启动。
  本轮仅更新文档；不修改冻结底层、日志开关或业务代码。

## 2026-09-09 · 最新工具与 OpenOCD 默认流程

按用户要求，日常推荐 OpenOCD，CubeProgrammer 不推荐且不再作为依赖。
旧文档版本 pin 已取消；下面 2026-09-08/07 内容仅保留历史证据。

- Ubuntu 22.04.5 / aarch64：just 1.58.0、CMake 4.4.3、Ninja 1.13.2、Arm GNU
  Toolchain 15.3.Rel1 (GCC 15.3.1) 及 xPack OpenOCD 0.12.0-7 用户级安装完成。
  GitHub latest/Arm release branches 提供发行版本，全部新工具归档摘要校验通过。
  包来源与 SHA-256 保存于本地配置 tool_sources，并随构建写入 manifest。
- OpenOCD banner：0.12.0+dev-02228-ge5888bda3-dirty (2025-10-04-22:42)。这是最新
  xPack 发布包自带标识，并非本轮改动源码；不把它称作纯上游稳定版 0.12.0。
- uv 管理 Python 3.14.7；requirements.in 解析为 12 个最新稳定依赖快照，包含
  numpy 2.5.3 / matplotlib 3.11.1 / pyserial 3.5。pip check、logger help、Agg
  无桌面绘图通过。旧 .venv 保留为 .venv-python313-backup，系统 Python 未替换。
- 两车型 Debug 各完成 113 步 ARM 构建；ELF EABI5/hard-float、向量、内存范围、
  未解析符号检查通过，RTOS 启动/调度符号均未链接。

| 车型 | FLASH / 1 MiB | RAM / 128 KiB | CCMRAM / 64 KiB |
|---|---:|---:|---:|
| infantry_standard | 127040 B | 47800 B | 0 B |
| sentry_swerve | 129408 B | 47808 B | 0 B |

产物位于 build/linux-aarch64-306202d6/inf-debug 和 sen-debug，含 ELF/map/bin/hex/
inspection/manifest。旧工具与旧构建保留；本次编译器更新导致容量变化，不沿用历史 SHA。

- 16 项工具编排测试、10 项环境/USB 测试、6 项 OpenOCD Tcl 测试全部通过。
  Tcl 测试使用新安装的真实解释器，但以替身覆盖所有目标操作且不加载接口；覆盖
  芯片/容量不符、读取失败、复位后身份变化、复位/写入/校验失败不运行、显式运行选择、
  路径中的中文/空格/Tcl 特殊字符。这些结果不是实际烧录成功证据。
- 9 个 GCC 主机测试程序通过，使用 -Wall -Wextra -Werror。
- doctor 实际使用 OpenOCD noinit 解析 ST-Link/F407 配置通过；Linux USB 元数据
  枚举到一个 ST-LINK/V2.1。未打开 SWD；权限、接线、芯片身份和实际写入仍待确认。
  flash-plan 成功输出 OpenOCD 命令，硬件命令执行数为零。
- VS Code 任务和 Linux PATH 已更新；GUI 点击未实测。Windows/macOS 新 OpenOCD
  流程待实机。未执行烧录、擦除、复位、运行，未修改冻结底层或任何机器人业务代码。

## 2026-09-08 · Jetson Ubuntu 22.04.5 / aarch64

编译、主机测试和 Python 工具环境已就绪；烧录环境尚缺官方原生 CubeProgrammer。
下面 2026-09-07 的 Windows 芯片读取/部分写入记录不是本次 Linux 操作。

- 用户目录独立安装 just 1.46.0、CMake 4.2.3、Ninja 1.13.1、Arm 14.3.Rel1
  （GCC 14.3.1 / arm-none-eabi），均验证官方归档 SHA-256；系统工具保留。
- uv 0.12.10 管理 CPython 3.13.7，项目 `.venv` 内安装全部 12 项固定依赖。
  `uv pip check`、logger `--help`、serial/numpy/matplotlib Agg 绘图通过。
- 两车型 Debug 各完成 113 个编译/归档/链接步骤；ELF32 ARM EABI5 hard-float、
  向量地址和强处理器、内存范围、无未解析符号检查通过，生成 ELF/map/bin/hex/
  inspection/manifest。步兵 FLASH/RAM 127328/47800 B，哨兵 129688/47808 B，
  CCMRAM 均为 0；本次数字相对历史 Windows 构建均多 16 B Flash，以本机 manifest 为准。
  产物位于 `build/linux-aarch64-*/inf-debug/` 和 `sen-debug/`。
- 两车型 RTOS 启动、调度器与 port handler 符号仍未链接；源码 main 无 RTOS/
  AppRuntime_Step 调用。已纠正 architecture/overview 的过时启动描述。
- 18 项工具安全测试、4 项环境回归测试和 9 个 GCC 主机测试程序通过；
  主机 C 检查启用 `-Wall -Wextra -Werror`。Linux x86_64 包选择仅模拟验证。
- `just doctor` 报 BUILD ENVIRONMENT READY / FLASH NOT READY；`flash-plan` 通过。
  `source tools/activate.sh` 后 just 使用 Python 3.13.7；VS Code Linux 配置和任务
  JSON 已校验，GUI/客户端任务点击尚未实测。默认车型步兵 Debug，校验后不运行。
- USB 已枚举 ST-LINK/V2.1（0483:374b），有可访问节点及已有 udev 规则。
  未进行芯片连接、擦写、复位、运行或机器人功能验证，未改 udev/用户组。
- ST 官方说明 Linux ARM64 CLI 从 2.23 开始，仓库现行 2.21 pin 不适用该平台。
  本机官方产品页访问失败（HTTP/2 错误、HTTP/1.1 超时）；未取得安装包，也未安装
  非官方镜像。需通过官方渠道取得 ARM64 包，核对 CLI 并增加平台 pin 后继续配置。
  这是当时的阻塞记录；当前已采用 [OpenOCD 配置](quickstart.md)，无需 CubeProgrammer。

## 2026-09-07 · Windows 历史记录

最新状态：用户后续实际下载报 Sector[0] 失败；只读检查确认 Flash 已部分写入，
CPU 处于 locked up 状态。脚本已将下载阶段由 HOTPLUG 改为 NORMAL/SWrst（复位后暂停），
身份检查仍为 HOTPLUG。18 项模拟测试/flash-plan 通过；修订后的真实重烧仍待验证，
不能宣称已修复硬件下载或把 99% 当成功。详情见下方失败诊断。

此前状态：用户确认使用 DJI 官方 C 板。当前 ST-Link 已被 Windows/CubeProgrammer
识别；HOTPLUG 读取到 3.21 V、设备 ID 0x413、F405/407/415/417 系列、`1 MBytes`。
已修复脚本只接受 `1024 KBytes` 的容量单位误判，真实输出回放校验与 18 项工具
测试通过。下面无探针记录是早先检查状态；仍未执行下载、擦除、复位或运行。

源码来自 `NYUSH-Robotics-Club/rm_C_board_control_cleanup`，main 提交
`6c2925f6e72461f17292be268c0c1b296b051838`。本次使用新目录完整 Git clone，
非浅克隆，包含两次提交；仓库没有额外子模块。工具层改动保留在工作区，未推送远端。

## 主机与依赖

- Windows 11 家庭版中文版 10.0.26200，x64；本次原生 PowerShell 7.6.5。
- Python 3.13.7；VS Code 1.129.1 x64；Git 2.51.0.windows.1。
- CMake 4.2.3；Ninja 1.13.1；Arm GNU Toolchain 14.3.Rel1，GCC 14.3.1，target `arm-none-eabi`。
- STM32CubeProgrammer 2.21.0 在标准 ST 安装目录，CLI 能运行，Authenticode 为 Valid。
- Driver Store 已有 ST-Link 调试/VCP/bridge 驱动 2.2.0.0；未重复安装、未移除旧版本。
- 缺少的 just 1.46.0 已用户级安装，官方 x86_64 ZIP 的 SHA-256 比对通过：
  `f0acf3f8ccbcf360b481baae9cae4c921774c89d5d932012481d3e0bda78ab39`。
- 其他所需工具和 C/C++、CMake Tools 扩展已有，直接复用。未安装 ROS、CUDA、完整嵌入式 IDE 或额外 GDB 环境。

发现本机 Python 3.10、3.13、WindowsApps alias 并存，用户 PATH 还有 MSYS/MinGW
相关工具；VS Code 全局默认终端原为 MSYS2。项目本地配置选择原生 PowerShell，
并为任务/新终端提供选定工具目录。构建子进程只优先使用配置中的 ARM 编译器，
不继承其他项目的 CC/CXX/CFLAGS/CXXFLAGS/ASMFLAGS/LDFLAGS。

具体可执行文件完整路径、CPU 信息和已有 PATH 记录于交付目录外层的
`environment-report.local.json`；用户名、路径和探针配置没有写进共享项目配置。

## 当前 ARM 产物（Debug）

| 车型 | FLASH / 1 MiB | RAM / 128 KiB | CCMRAM / 64 KiB | Reset entry |
|---|---:|---:|---:|---|
| infantry_standard | 127312 B (12.14%) | 47800 B (36.47%) | 0 B | 0x0800c6d9 |
| sentry_swerve | 129672 B (12.37%) | 47808 B (36.47%) | 0 B | 0x0800c6e9 |

两车型均完成 113 个 ARM 编译/归档/链接步骤。ELF 都是 little-endian ELF32 ARM
EABI5、Cortex-M4/v7E-M、Thumb、VFPv4-D16 hard-float；向量表位于 0x08000000，
初始 MSP 为 0x20020000。所有分配段和加载段位于冻结链接脚本的合法内存区域；
无未解析符号，SVC/PendSV/SysTick 向量绑定当前强符号处理器。

每个目录均生成 ELF、map、bin、hex、inspection 和 manifest；根据 CMake 缓存确认
车型及 Debug 构建。最新完整路径、SHA-256、Git dirty 和工具链 banner 以各自
`build/w64-*/inf-debug/firmware-manifest.json`、
`build/w64-*/sen-debug/firmware-manifest.json` 为准。
历史目录保留，不将旧产物作为构建失败时的回退。Debug 路径/工具映射会影响 ELF
的调试信息和 SHA，容量相同并不表示 ELF 每个字节相同。

## RTOS 核查与明确边界

当前 main 在初始化后进入裸机循环，没有调用 `RobotRtos_Start()`。
`Src/stm32f4xx_it.c` 的 SVC/PendSV 函数为空，SysTick 只调用 `HAL_IncTick()`。
两车型 ELF 中 `RobotRtos_Start`、`vTaskStartScheduler`、`xPortPendSVHandler`、
`vPortSVCHandler`、`xPortSysTickHandler` 均未链接。

运行时代码和 FreeRTOS 静态库存在，但没有进入有效启动链。原 PROJECT_MEMO、
README 和 setup-guide 的部分“已启用 RTOS”表述与当前源码不符，已在本次文档
中纠正。用户明确要求先不调用 RTOS，本次没有添加任何调用或中断转接。

## 工具层验证

- `just doctor`：编译环境就绪；实际 `-l stlink-only` 枚举结果为无 ST-Link。
  用户回复已连接后再次检查，Windows 仍未出现 ST-Link/VID_0483；只读诊断返回
  `NO_STLINK_USB`，13 个当前 USB 设备错误码均为 0。摄像头的 DFU 接口不是 STM32。
  本机已有 ST 官方驱动，尚不能证明线缆/探针硬件正常，未重复安装驱动。
- 参考 RM_Ecat 89a86d88 新增可选 F407 OpenOCD 配置，未安装或实测 OpenOCD。
  日常入口继续使用 CubeProgrammer。修订后标准步兵再次构建、ELF 检查通过，
  SHA-256 为 `7549f026c60f7736b2cd4ea15ac04120f75d1a9d6a1c10381fc6835eecdccb11`，
  RTOS 仍未链接；任务 JSON 可解析，冻结底层 diff 为空。
- `just flash-plan`：生成只读计划；未调用设备连接、下载、擦除、复位或运行命令。
- 17 项 Python 测试：无配置不选车型、显式车型覆盖不改默认、无探针/多探针/
  指定探针缺失、芯片或容量错误、编译失败、外部进程非零、下载失败、校验失败、
  仅明确选择后复位、计划不启动外部进程、错误 ELF、含空格/中文参数均通过。
  涉及硬件的返回值均为测试替身，不能标为真实烧录通过。
  ST-Link 修订新增 USB 状态分层判断、CLI 枚举失败不进入连接的测试。
- 将仓库复制到包含“中文 空格工程”的路径，并将 just 放在“工具 空格”路径，
  在仓库以外目录通过 `just --justfile ... build infantry_standard` 完整构建通过。
- 初始 Windows 过长构建目录暴露 MinGW 路径限制，已用较短且可区分的构建目录
  修复；中文路径的 DSP 库链接失败已用临时 subst 映射修复，未改链接脚本或库。
- 首次配置读写成功；本机路径/复位选项保存后可重读；bootstrap 重跑复用全部固定版本。
- 新增 Firmware: Doctor/Build/Flash。默认 Build 改为只构建，旧 DFU 任务不再占用
  默认构建快捷键。ST-Link 修订将旧 OpenOCD 烧录任务替换为统一 CubeProgrammer
  入口，标签为 flash: stlink (CubeProgrammer)，其余历史任务保留；没有自动烧录任务。
- VS Code 真实终端/任务验证状态将在完成工作区信任后补记；此前测试窗口因
  Restricted Mode 忽略终端配置，不能把该次尝试声明为通过。
- 冻结范围以及 BSP、应用、电机驱动、协议、控制参数没有修改；顶层 CMake、
  原 toolchain、CMakePresets 保留。

## 仍未验证

已执行一次 HOTPLUG SWD 身份读取，没有执行烧录、擦除、复位、运行，没有机器人
功能或实车安全验证。探针已枚举，目标参考电压读数为 3.21 V；用户确认 DJI 官方 C 板，
尚未独立核查芯片丝印/完整接线。早先断开探针与当前探针序列号不同，不能据此宣称
早先红灯闪烁的具体根因已查明。
Release 分支提供配置入口，本次容量记录为 Debug。
主机 Unix shell 测试未作为 Windows 依赖运行，不代替 ARM 检查。

macOS Apple Silicon / Intel 仅提供安装和统一脚本实现，均待对应系统实机验证。
Intel Mac 固定 Arm 14.2.Rel1；Windows ARM64 因 Arm host/USB 驱动支持未确认而阻塞。

## Sector[0] 下载失败诊断

用户提供日志显示擦除扇区 [0,4] 后下载失败，属于实际写入尝试，不能用早先环境配置
“未烧录”的记录描述该次操作。agent 本轮仅执行 HOTPLUG 读取，未重复擦写或复位。

- RDP=0xAA（无读保护），WRP0..11 全部不保护；目标电压 3.21 V。
- FLASH_SR=0，CPU 状态 locked up；PC=0x20000000，MSP=0x20000500，
  RAM 首条指令包含 BKPT。CFSR=0x00018200，HFSR=0x40000000，表明存在故障状态；
  尚不能仅凭这些寄存器证明最初故障触发原因。
- 读取 0x08000000 起 127312 B，与本次 bin 比较：121586 字节不同，首个差异
  0x08001390，末个差异 0x0801F14F，确认镜像不完整。
- 读回 SHA-256：`f4da4c40e4d663cf376f6c8d6702373b0bc73f97915cc8e487f16db0b5250170`；
  期望 bin：`9b3b9cc98973e3ccdd6f0f9cd9cb587f97715b182dd192227c768d148fcd21cc`。
- 本机证据保存在被忽略的 build/flash-failure-readonly.log、flash-failure-registers.log、
  flash-failure-readback.bin；修订后的计划在 build/flash-recovery-plan.log。
- 18 项工具测试验证身份检查不复位、写入前 NORMAL/SWrst、失败不自动重试/运行，
  以及原有芯片/容量/车型/校验检查。真实重烧尚未执行，RTOS 和冻结源码未修改。
