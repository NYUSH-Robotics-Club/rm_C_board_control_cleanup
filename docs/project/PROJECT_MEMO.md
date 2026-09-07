# RoboMaster Control — Project Memory

每次任务开始先读本文件；代码、配置、架构、硬件假设或验证状态变化后，结束前
更新本文件。它用于防止跨任务遗忘，不替代源码和实车记录。

## 当前任务（2026-09-07）

- 在 Windows x64 新克隆中配置可复现的开发环境；使用 just/Python 统一入口，
  默认保存 infantry_standard、Debug、ST-Link/SWD、允许唯一探针、校验后不复位运行。
- 用户再次明确：**先不调用 RTOS**。全部底层冻结，应用/BSP/驱动也不修改。
- 新增 tools/firmware.py、tools/bootstrap.py、tools/test_firmware.py、justfile，
  合并 VS Code 任务，提供 docs/quickstart.md 和 docs/environment-validation.md。
- 本机工具与探针配置存 .firmware.local.json，编辑器本机路径存被忽略的
  .vscode/settings.json；共享配置不包含个人路径或探针序列号。
- Windows 路径兼容使用临时 subst 映射，构建按主机/路径标识/车型/类型隔离；
  不删除旧构建目录，不覆盖冻结 toolchain、启动汇编和链接脚本。
- ST-Link 排查：对照 RM_Ecat 89a86d88 的 OpenOCD 配置，新增 F407 适配
  tools/openocd/stm32f407-stlink.cfg（保留文件 GPL-2.0-or-later 标记）；
  它不是 USB 驱动安装器，不参与 CubeProgrammer 的 just flash，也未实测 OpenOCD。
- 历史 OpenOCD 烧录任务改为 flash: stlink (CubeProgrammer)，复用统一 just flash；
  默认不复位运行。新增 tools/stlink_usb.py，doctor/flash 枚举失败时显示原始 CLI
  输出和只读 Windows USB 分层诊断，不自动安装/替换驱动或升级探针。
- 早先探针在 17:30 成功绑定官方 WinUSB 2.2.0.0，17:36:54 从 USB 总线移除，
  随后的红灯闪烁阶段仅有已断开历史记录。没有证据判定为驱动版本不兼容。
  系统重新扫描因当前进程没有管理员权限而失败；未更换驱动或重启 USB 控制器。
- 最新已枚举到另一序列号的 ST-Link V2J48M35，Windows 状态正常；用户确认目标是
  DJI 官方 C 板。HOTPLUG 读取到 3.21 V、ID 0x413、F405/407/415/417、1 MBytes。
  修复 validate_target 只接受 KBytes 的误判：MBytes 乘 1024 后仍严格要求 1024 KiB，
  错误容量/家族/未知单位继续拒绝，并在错误中显示实测字段。未烧录/擦除/复位/运行。
- 随后用户实际执行 flash，HOTPLUG 下载报 Sector[0] 失败。本次只读诊断：RDP=AA、
  WRP0..11 全不保护、FLASH_SR=0；CPU locked up，PC=0x20000000（RAM loader），
  CFSR=0x00018200、HFSR=0x40000000。读回 127312 B 与构建 bin 不同，首个差异
  在 0x08001390，固件已部分写入，不得复位运行该不完整镜像。
- 修订下载连接为 NORMAL/SWrst，下载前软件复位并暂停以清理异常状态；身份读取仍
  HOTPLUG。此复位发生在实际 flash 写入阶段，run_after=false 仍禁止校验后复位运行。
  不自动使用 UR、不改 Option Bytes；18 项模拟测试及只读计划通过，尚未实测重烧。
  添加 cube-flash.log 详细日志及 -q；失败提示明确可能已擦除/部分写入，不再笼统声称未烧录。

## 最近架构任务（2026-09-02，历史记录，运行时结论已由下文核正）

- 目标：保持云台上电位置修复，集中应用扩展入口，最大限度降低应用、协议和
  板级代码耦合；直接启用 RTOS；参考指定仓库加入 DM、本末、瓴控驱动；简化
  文档并核对依赖、功能和硬件依据。
- 用户原先冻结全部底层；本次新增明确授权只解除 RTOS 必需范围。因此仅允许
  修改 `Src/main.c`、`Src/stm32f4xx_it.c`、顶层 `CMakeLists.txt`，并新增
  `Middlewares/Third_Party/FreeRTOS-Kernel/`。`Inc/`、`Drivers/`、`.ioc`、
  CubeMX CMake、启动汇编和链接脚本仍未获修改授权。
- 完成本次后恢复底层冻结。以后要改变时钟、引脚、DMA、CAN 波特率、IRQ
  优先级或 CubeMX 生成内容，必须再次取得明确授权。
- 不给未知电机型号、全向/舵轮几何、Jetson 传输或摄像头参数猜默认值。

## 当前事实

- MCU：STM32F407；构建目标：`infantry_standard`、`sentry_swerve`。
- 当前实际启动是裸机循环：main 没有调用 RobotRtos_Start，SVC/PendSV 为空，
  SysTick 只调用 HAL_IncTick。2026-09-07 双车型 ELF 均未链接 RTOS 启动/调度器
  与 FreeRTOS port handler 符号。保留现状，不以构建通过声明 RTOS 已启用。
- 仓库含官方 FreeRTOS Kernel V11.3.0，未启用的运行时代码配置原生 API、1 kHz tick；静态内存，
  `configSUPPORT_DYNAMIC_ALLOCATION=0`。这里的“静态”只指 RTOS 对象；旧
  Quaternion EKF 仍在首次更新时使用 C 库 `malloc` 分配矩阵。
- 未启用的 runtime/rtos 代码定义一个 1 ms 静态控制任务和 FreeRTOS idle task。控制任务依次执行
  IMU 更新、可选应用步进、命令路由、消息派发、电机集中刷新、蜂鸣器和限流
  CAN 日志。
  这些定义没有从当前 main 启动，不代表实际 RTOS 调度。
- 消息中心是仿 ROS2 的固定内存事件总线，不是调度器；当前派发由 main 的裸机循环执行。
- 云台 yaw/pitch 等待真实反馈后同时锁存上电位置，重置 PID，再允许保持电流；
  步兵 pitch 的固定 `3370` 已取消，避免上电突跳。
- `application/cmd/command_router.c` 保存模式策略；`cmd_controller.c` 只收消息、
  调路由、发标准命令。可选应用集中登记在 `application/runtime/app_manifest.c`。
- 遥控 200 ms 无新帧时统一禁用输出；小陀螺 yaw 调整按真实时间差积分。
- CAN manager 启动或整套电机配置校验失败时，`main` 进入安全错误状态，不再
  初始化控制器或启动调度器。
- BSP（Board Support Package，板级支持包）是独立顶层目录，只负责具体板卡
  I/O；协议含义和业务逻辑不放进 BSP。

## 依赖规则

```text
application -> core contracts/interfaces -> services/adapters -> modules -> bsp -> HAL
runtime/rtos -> application + message center + FreeRTOS
```

- 应用层不拼 CAN 帧、不访问 HAL 句柄、不判断厂商协议。
- 厂商适配器只接收 8 字节标准数据帧；同总线的 DM 控制 ID、本末物理地址、
  瓴控广播槽位重复时初始化失败，不发送存在歧义的命令。
- 主题编号属于消息中心；跨层载荷放 `core/contracts/`。
- 运动学放 chassis strategy；电机协议放 motor adapter/protocol codec。
- 消息中心不调用具体业务；电机发送只由 after-dispatch hook 集中刷新。
- 运行时控制环开关放 `MotorConfig_t.control_mode`，调试覆盖走
  `MotorService_SetControlMode()`，不散布编译宏。
- 新应用只通过 `AppModule` 清单、标准主题、服务接口和 BSP 接口扩展。

## 支持矩阵

| 能力 | 代码状态 | 现有车型是否启用 | 仍需资料 |
|---|---|---:|---|
| DJI M3508/M2006/GM6020 | 已有旧驱动并适配 | 是 | 实车参数复核 |
| DM MIT | 编解码、反馈、使能/失能、统一 PID 适配已加入 | 否 | 精确型号、P/V/T 范围、CAN ID |
| 本末 BM1505B | 分组命令、模式/反馈配置、反馈解析已加入 | 否 | 型号、反馈 ID、限幅、波特率 |
| 瓴控广播电流 | 0x280 命令、0x141~0x144 反馈已加入 | 否 | 系列、工具配置、限流、编码器分辨率 |
| 麦轮 | 已用 | 步兵 | 实车方向/参数复核 |
| 现有舵轮 | 已用旧双舵方案 | 哨兵 | 通用四模块几何仍未知 |
| 全向轮 | 安全占位 | 否 | 轮数、安装角、半径、减速比 |
| 旧 USB/Seasky 视觉 | 已用 | 是 | 上位机联调 |
| Jetson 新视觉 | 端口和标准消息已预留 | 否 | 摄像头、传输、坐标、时间戳、帧格式 |

新增厂商适配器“已实现”不等于可直接接未知电机。只有车型配置通过适配器校验
才会发送；当前两套配置没有加入任何 DM、本末或瓴控实例。
云台和哨兵转向仍直接使用 DJI 旧上下文；新厂商目前兼容统一服务以及可配置的
驱动/发射路径，不能只改 vendor 就替换这些特殊轴。

## 参考来源与许可证

- FreeRTOS Kernel V11.3.0，MIT，固定提交 `9b777ae5...`，源码和许可证已随仓库保存。
- HNUYueLuRM/basic_framework，MIT，提交 `6813c72b...`：参考 RTOS 分层、DM MIT
  和瓴控广播帧。
- NYUSH-Robotics-Club/RM_Ecat，LGPL-2.1，提交 `89a86d88...`：协议工作只核对事实和
  BM1505B 字段。2026-09-07 另参考其标注 GPL-2.0-or-later 的 OpenOCD 配置，
  在 tools/openocd 中适配 F407，并保留该文件许可证标记。
- NYUSH-Robotics-Club/Dart，提交 `2048b42b...`：当前仅找到 DJI 驱动，且根部
  未发现许可证，所以没有复制代码。

## 仍需解决的问题

- 尚未测量控制任务最坏执行时间、1 ms 抖动和栈高水位；日志仍在控制任务中，
  实车测量后才能决定是否拆出低优先级任务。
- 旧 `Kalman_Filter_Init()` 在控制任务第一次 IMU 更新时使用 libc heap，且没有
  检查每次分配失败；当前只有一个业务任务，所以没有并发分配，但必须检查链接
  后 heap/RAM 余量。后续静态化需要单独验证算法，不能混入 RTOS 接入改动。
- 消息中心满队列会覆盖最旧消息，暂无丢包计数和每主题优先级。
- CAN/USB/UART 中断尚未使用 RTOS 通知；当前继续用短关中断区和单派发者。
- 本末启动配置一次周期可能发送两帧，需确认目标 CAN 总线负载和实际手册。
- 瓴控广播模式必须先用厂商工具开启；驱动不会擅自修改电机参数。
- DM、本末、瓴控都没有接到现有车型，不能宣称已实车验证。
- 全向轮、通用舵轮和 Jetson 新协议仍缺真实硬件输入。

## 验证记录

- 2026-09-07 容量解析修复：18 项工具测试通过；已捕获的真实 HOTPLUG 输出通过
  validate_target，确认 1 MBytes 与 1024 KBytes 等价。当前探针已识别，之前无探针
  的记录仅代表当时状态；未宣称早先红灯闪烁探针的根因已排除。RTOS 与底层未修改。
- 2026-09-07 ST-Link 修订：17 项工具测试通过；实际 doctor 输出 NO_STLINK_USB；
  infantry_standard Debug 再次构建/ELF 检查通过，SHA-256 未变，RTOS 符号仍未链接。
  VS Code 任务 JSON 与 flash-plan 检查通过；未执行下载、擦除、复位或运行。
- 2026-09-07：Windows 11 x64、CMake 4.2.3、Ninja 1.13.1、Arm 14.3.Rel1，
  两车型 Debug 完整 ARM 编译链接和 ELF EABI5/hard-float/地址/向量/未解析符号检查通过。
  infantry FLASH/RAM 127312/47800 B；sentry 129672/47808 B。RTOS 没有启动/链接。
- 2026-09-07：中文+空格仓库完整构建通过；工具路径含空格；15 项工具层模拟失败测试通过。
  无探针枚举与 flash-plan 已实际执行，未执行任何下载、擦除、复位、运行。
  macOS 两架构仅提供实现，待实机验证；Intel 固定 Arm 14.2.Rel1，Windows ARM64 阻塞。
- 2026-09-02 的历史文字曾声称 RTOS 已启用、另一文档曾列出不同 ELF 容量；
  这些不能作为当前 6c2925f 源码的验证证据，以上述本次检查及 environment-validation.md 为准。

- 2026-09-02：主机测试通过消息中心、底盘策略、电机服务、消息契约、命令路由、
  三类新增电机协议字节、适配器启动/收发以及两套车型配置检查；相关源码通过
  Clang `-Wall -Wextra -Werror` 检查。
- 2026-09-02：Cortex-M4 FreeRTOS port、SVC/PendSV/SysTick 转接和静态运行时
  通过 ARM target 语法检查；CMake 源码路径、现行 Markdown 索引/本地链接和
  文件级职责注释检查通过。冻结的 `Inc/`、`Drivers/`、CubeMX CMake 未改动。
- 2026-09-02：本机没有 `arm-none-eabi-gcc`/STARM 标准库，未完成整固件 ARM
  编译链接；不得据此直接判定可烧录。
- 待完成：两种 `ROBOT_TYPE` 的 ARM 构建、ELF/HEX 生成、静态 RAM/Flash 检查、
  调度器启动观测、1 ms 周期/栈测量、云台无突跳和三类新电机逐型号小电流测试。
