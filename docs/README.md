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
| `architecture/rtos-migration.md` | 未启用的 FreeRTOS 预留设计和接入边界 | 已核正启动状态 |
| `guides/boot-sequence.md` | 上电、云台锁存、裸机循环及 LED 顺序 | 已核对 main |
| `guides/gimbal-compensation.md` | 云台耦合补偿、上电保持和调参风险 | 继承后简化 |
| `guides/logger-guide.md` | 固件日志接口和主机脚本 | 新写 |
| `guides/vision-diagnostics.md` | 现有视觉链路、Jetson 接入空位和排错 | 新写 |
| `protocols/message-center.md` | 消息中心主题、容量和裸机派发上下文 | 已核对 |
| [quickstart.md](quickstart.md) | 最新工具安装、OpenOCD 烧录入口 | 当前推荐 |
| [environment-validation.md](environment-validation.md) | 实机连接、构建验证及尚未验证项 | 当前证据 |
| `protocols/seasky-vision.md` | 当前 USB CDC/Seasky 兼容协议 | 继承后核对 |
| `tutorials/setup-guide.md` | 工具链、双车型构建和烧录 | 继承后简化 |
| `tutorials/can.md` | CAN 基础、DJI/DM/本末/瓴控协议入口 | 继承后简化 |
| `can-recovery.md` | Bus-Off 受控恢复、上电回中解锁与停机验证边界 | 2026-09-08 新增 |
| `omni-chassis.md` | CAN1 四全向轮运动学、3/2/1/4 轮位、已确认几何与验证状态 | 2026-09-08 新增 |
| `tutorials/github-commands.md` | 通用 Git 命令速查 | 继承原文 |

`archive/` 下五份 Markdown 是继承的历史材料，只用于追溯；其中路径、频率、
接口和 RTOS 结论可能失效。`official-docs/` 是原有 DJI PDF，`assets/` 是原有
教程图片，两者都不是本轮生成。

## 当前核对结论（2026-09-09）

- 当前裸机启动，FreeRTOS 预留代码未从 main 启用；消息回调由裸机派发执行。
- 新装开发工具选择最新发行，OpenOCD 首选，CubeProgrammer 不推荐；版本表仅为快照。
- DM MIT、BM1505B、瓴控 0x280 广播电流协议已有驱动和适配器；现有两套
  车型仍只配置 DJI，新增厂商不会在无参数时自动启用。
- 2026-09-08：步兵已选择参数化全向轮；已确认轮位左前3/右前2/右后1/左后4，
  X形±45°、轮距/轴距0.54m、轮半径0.07m、P19减速比已填入；方向待架空验证。
  麦轮策略和哨兵舵轮保留，详见全向轮说明。
- Jetson 端口仍为空位，摄像头型号、传输和新消息格式仍未知。
- 两车型 ARM Debug 构建与主机测试通过，步兵已通过真实 OpenOCD 擦写/校验，
  校验后保持暂停；C 板 USB 日志尚未连接，程序运行和机器人功能仍待验证。

## 维护规则

- 改代码、配置、硬件假设或验证状态时，同步更新 memo 和对应专项文档。
- 新文档进入本索引；旧结论移到 `archive/`，不要与现行说明并列。
- 未知的型号限幅、底盘几何、摄像头和 Jetson 协议必须标“未知”，禁止猜值。
