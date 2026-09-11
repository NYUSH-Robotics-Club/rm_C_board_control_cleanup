# Logger 使用指南

本文说明当前固件日志接口和主机端串口工具。实现以
`modules/logger/` 和 `script/logger.py` 为准。

## 当前能力

- 12 个标签：`SYS`、`CMD`、`CHA`、`GIM`、`SHO`、`SEN`、`MOT`、
  `IMU`、`CAN`、`VIS`、`RC`、`DEBUG`。
- 5 个级别：`ERROR`、`WARN`、`INFO`、`DEBUG`、`CSV`。
- 每个标签可编译期开关，并可设置独立的运行时限流间隔。
- 文本格式为 `[TAG][LEVEL] message`，CSV 格式为
  `TAG,timestamp_ms,fields...`。

## 固件端

包含头文件：

```c
#include "logger.h"
```

常用接口：

```c
LOG_ERROR(LOG_TAG_CAN, "CAN start failed: %d", status);
LOG_WARN(LOG_TAG_VIS, "Vision timeout: %lu ms", elapsed_ms);
LOG_INFO(LOG_TAG_SYS, "System ready");
LOG_DEBUG(LOG_TAG_MOT, "motor=%u speed=%.1f", id, speed);
LOG_CSV(LOG_TAG_GIM, "YAW,%.2f,%.2f", target, feedback);
```

编译期开关位于 `modules/logger/logger_config.h`。当前默认只启用
`LOG_ENABLE_GIM`，其余标签为关闭状态。需要某个标签时将对应宏改为 `1`；
大量日志会占用 USB CDC 带宽，调试结束后应关闭无关标签。

初始化和默认限流由 `Src/main.c` 完成。上层模块如需调整，可调用：

```c
Logger_SetRate(LOG_TAG_GIM, 50);  // 最短间隔 50 ms，即最高约 20 Hz
```

间隔为 `0` 表示不进行 Logger 限流，不代表底层传输一定能无损承载全部输出。

## 主机端串口工具

当前输出链路是 `Logger -> Debug_SendString -> BspUsb_Write -> USB CDC`，
需要 C 板自己的 USB 数据口。ST-Link 的 USB 负责 SWD，其自带 VCP 是另一条 UART，
不会自动转发此固件的 USB 日志。2026-09-09 本机仅枚举到 ST-Link VCP
`/dev/ttyACM0`，未枚举到 C 板 CDC，尚不能据此读取板上日志。

先按 [环境指南](../quickstart.md) 安装最新工具和 Python 依赖，再列出串口、
结合设备描述与插拔前后变化确认 C 板端口。不要依赖脚本自动选口：它可能选中 ST-Link。

仓库中的有效脚本名称是 `script/logger.py`，不是旧文档中的
`script/smart_logger.py`。

```bash
source tools/activate.sh
python -m serial.tools.list_ports -v

# 将路径替换为确认属于 C 板 USB CDC 的真实端口
python script/logger.py /dev/serial/by-id/实际C板设备 --tags GIM
python script/logger.py /dev/serial/by-id/实际C板设备 --tags GIM --save gimbal_session.csv

# 查看标签
python script/logger.py --list-tags
```

脚本支持 `--save` 和 `--auto-save`，不支持旧说明中的 `--no-plot`。脚本本身
只负责终端显示和保存，不提供曲线绘图。

`--tags all` 只取消主机过滤，不能开启固件中关闭的日志。当前仅 GIM 编译开启；
补偿路径有 `COMPENSATION` CSV，若干 YAW/ENCODER 日志调用仍被注释。
有日志不等于有所有电机反馈，没有日志也不能单独证明某个回调未执行。

## 查看消息回调和变量（OpenOCD + GDB）

当前是裸机 `MsgCenter_Dispatch()` 调用订阅回调，不是 FreeRTOS 任务。
USB logger 只接收主动输出的日志，不能直接订阅 MCU 内部消息中心，也没有现成的
全量电机反馈流或 RTT 通道。需要观察 `on_gimbal_cmd`、`on_imu_update` 的参数、
调用栈或变量时，可以用工具链自带的 `arm-none-eabi-gdb`。

**断点会暂停控制循环，可能留下电机最后一次输出。先隔离动力输出再调试。**
下面是手动调试示例，不会由 `just doctor` 启动。不要与烧录工具同时占用探针。
先确保目标已完整烧录并校验了与所选 Debug ELF 完全一致的固件；GDB 加载符号
不会自动烧录，也不能证明板上镜像一致。

```bash
source tools/activate.sh
# 替换实际探针序列号；仅监听本机，关闭额外服务
openocd -f tools/openocd/stm32f407-stlink.cfg \
  -c 'fw_select_serial 实际探针序列号' \
  -c 'bindto 127.0.0.1; tcl port disabled; telnet port disabled'
```

另一个激活环境的终端运行 `arm-none-eabi-gdb /本次构建产物的完整路径/固件.elf`，
ELF 路径以 `just build` 输出为准。在 GDB 中：

```text
target extended-remote localhost:3333
monitor halt
break gimbal_controller.c:on_gimbal_cmd
continue
# 命中断点后再执行以下命令
bt
print *ev
```

`continue` 会运行目标；只有消息实际到达，断点才会命中。此流程尚未实机验证，
本轮仅验证了 GDB 能启动及 OpenOCD 的 SWD 身份读取；未暂停、复位或运行目标。
高频断点会改变时序，不适合据此测量正常控制周期。

## 常见问题

1. 没有输出：先确认固件已完整校验并在运行、端口是 C 板 CDC，再检查对应
   `LOG_ENABLE_*` 和调用路径。默认 `just flash` 校验后暂停，不会开始日志输出。
2. 找不到串口：显式传入 `/dev/ttyACM*`、`/dev/cu.usbmodem*` 或 Windows COM 口。
3. 乱码：确认主机端波特率与实际串口配置一致；USB CDC 虚拟串口通常不依赖
   物理 UART 波特率，但工具仍要求一个参数。
4. 丢行或控制变慢：增大 `Logger_SetRate()` 间隔，并关闭无关标签。
5. CSV 没有预期字段：检查调用处的 `LOG_CSV` 格式；脚本只对部分已知标签
   提供表头，其余数据仍会原样记录。

## 历史脚本

`script/deprecated/` 保存旧绘图脚本，仅供参考。它们没有随当前消息契约和目录
结构持续维护，不能作为现行调试入口。
