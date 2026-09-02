# 文档中心

现行技术说明都放在本目录。建议先读[项目记忆](project/PROJECT_MEMO.md)，再读
[全局说明](architecture/overview.md)。根目录 `README.md` 仅作项目入口；
`AGENTS.md`、`CLAUDE.md` 是工具约束，不是普通技术文档。

## 现行文档清单

| 文档 | 作用 | 来源 |
|---|---|---|
| `project/PROJECT_MEMO.md` | 每次任务必读的边界、决策、未知项、验证记录 | 本轮体系化维护，继承旧 memo |
| `project/COMMENTING_STANDARD.md` | 规定注释写什么、术语和单位如何说明 | 新写 |
| `architecture/README.md` | 架构文档入口 | 新写 |
| `architecture/overview.md` | 全局结构、依赖、扩展接口、问题和调试入口 | 重写旧架构说明 |
| `architecture/rtos-migration.md` | 当前 FreeRTOS 配置、任务和中断关系 | 新写，现已从计划更新为实装说明 |
| `guides/boot-sequence.md` | 上电、云台锁存、调度器启动及 LED 顺序 | 新写 |
| `guides/gimbal-compensation.md` | 云台耦合补偿、上电保持和调参风险 | 继承后简化 |
| `guides/logger-guide.md` | 固件日志接口和主机脚本 | 新写 |
| `guides/vision-diagnostics.md` | 现有视觉链路、Jetson 接入空位和排错 | 新写 |
| `protocols/message-center.md` | 消息中心主题、容量和 RTOS 上下文 | 新写 |
| `protocols/seasky-vision.md` | 当前 USB CDC/Seasky 兼容协议 | 继承后核对 |
| `tutorials/setup-guide.md` | 工具链、双车型构建和烧录 | 继承后简化 |
| `tutorials/can.md` | CAN 基础、DJI/DM/本末/瓴控协议入口 | 继承后简化 |
| `tutorials/github-commands.md` | 通用 Git 命令速查 | 继承原文 |

`archive/` 下五份 Markdown 是继承的历史材料，只用于追溯；其中路径、频率、
接口和 RTOS 结论可能失效。`official-docs/` 是原有 DJI PDF，`assets/` 是原有
教程图片，两者都不是本轮生成。

## 当前核对结论（2026-09-02）

- FreeRTOS Kernel V11.3.0 已进入真实启动链，使用静态任务和静态栈。
- DM MIT、BM1505B、瓴控 0x280 广播电流协议已有驱动和适配器；现有两套
  车型仍只配置 DJI，新增厂商不会在无参数时自动启用。
- 麦轮已用；现有舵轮保留；全向轮仍缺几何参数。
- Jetson 端口仍为空位，摄像头型号、传输和新消息格式仍未知。
- 本地 Markdown 链接、主机测试和两车型 ARM Release 构建已通过；实车调度、
  时序、栈、电机和云台测试仍是烧录后的必做项。

## 维护规则

- 改代码、配置、硬件假设或验证状态时，同步更新 memo 和对应专项文档。
- 新文档进入本索引；旧结论移到 `archive/`，不要与现行说明并列。
- 未知的型号限幅、底盘几何、摄像头和 Jetson 协议必须标“未知”，禁止猜值。
