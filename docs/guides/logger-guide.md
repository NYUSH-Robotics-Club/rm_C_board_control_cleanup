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

仓库中的有效脚本名称是 `script/logger.py`，不是旧文档中的
`script/smart_logger.py`。

```bash
python3 -m pip install -r requirements.txt

# 自动选择串口，交互选择标签
python3 script/logger.py

# 指定标签
python3 script/logger.py --tags GIM
python3 script/logger.py --tags CMD,GIM,IMU

# 指定串口和波特率
python3 script/logger.py /dev/ttyACM0 --baud 115200 --tags all

# 保存 CSV
python3 script/logger.py --tags GIM --save gimbal_session.csv
python3 script/logger.py --tags all --auto-save

# 查看标签
python3 script/logger.py --list-tags
```

脚本支持 `--save` 和 `--auto-save`，不支持旧说明中的 `--no-plot`。脚本本身
只负责终端显示和保存，不提供曲线绘图。

## 常见问题

1. 没有输出：检查对应 `LOG_ENABLE_*` 是否为 `1`，重新编译烧录。
2. 找不到串口：显式传入 `/dev/ttyACM*`、`/dev/cu.usbmodem*` 或 Windows COM 口。
3. 乱码：确认主机端波特率与实际串口配置一致；USB CDC 虚拟串口通常不依赖
   物理 UART 波特率，但工具仍要求一个参数。
4. 丢行或控制变慢：增大 `Logger_SetRate()` 间隔，并关闭无关标签。
5. CSV 没有预期字段：检查调用处的 `LOG_CSV` 格式；脚本只对部分已知标签
   提供表头，其余数据仍会原样记录。

## 历史脚本

`script/deprecated/` 保存旧绘图脚本，仅供参考。它们没有随当前消息契约和目录
结构持续维护，不能作为现行调试入口。
