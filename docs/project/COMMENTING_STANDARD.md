# 程序注释规范

本规范适用于项目程序。`Inc/`、`Src/`、`Drivers/`、`Middlewares/`、CubeMX
文件、启动文件和链接脚本默认冻结；即使某次任务为功能获准修改，也不得仅为
补注释扩大底层改动范围。

## 基本要求

1. 每个源码和头文件开头用 1～3 句话说明“本文件负责什么”和“不负责什么”。
2. 对外函数在头文件说明输入、输出、单位和失败结果；实现文件不重复整段说明。
3. 对状态、限幅、坐标、角度、时间等容易误用的数据明确写出单位和有效范围。
4. 注释重点解释“为什么这样做”和“异常时如何处理”，不要逐行翻译代码。
5. 使用常用词。必须使用 PID、CAN、CRC、ISR 等缩写时，在首次出现处说明含义。
6. 占位实现必须写明缺少什么资料，并明确返回“不支持”，不能让读者误以为可用。
7. 代码变化后同步修改相邻注释；失效注释比没有注释更危险。

## 推荐示例

```c
/*
 * Convert a chassis command into four wheel speeds.
 * Inputs are normalized to -1..1; output uses motor RPM.
 * Invalid geometry returns ROBOT_STATUS_INVALID_ARGUMENT and sends no command.
 */
```

## 不推荐示例

```c
i++;  // i 加一
MotorService_Flush();  // 调用电机服务刷新函数
```

这类注释没有补充目的、单位或失败行为，只增加阅读负担。
