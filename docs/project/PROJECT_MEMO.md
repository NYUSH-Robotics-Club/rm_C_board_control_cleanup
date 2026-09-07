# RoboMaster Control — Project Memory

每次任务开始先读本文件；代码、配置、架构、硬件假设或验证状态变化后，结束前
更新本文件。它用于防止跨任务遗忘，不替代源码和实车记录。

## 当前任务（2026-09-07）

- 2026-09-08 用户明确要求将yaw改为电流指令，已在8911563基础实现并烧录：
  infantry yaw硬件ID5保持RX209，TX改2FE/slot0；protocol.dji显式电流模式，原始刻度
  限幅4096=0.75A。位置环输出上限60RPM，P=1.5/I=D=0；速度环输出电流刻度，
  P=64/I=D=0、上限4096。这是首次调试初值，非实车调优、非旧电压PID等价转换。
  pitch配置/电压协议1FF/slot3与哨兵车型保持原样；电机内部电流环设置未修改。
  新dji_motor_protocol.h检查GM6020模式/反馈ID/发送组/槽位及限幅，零模式兼容电压；
  CAN发送组由3扩至5（增加1FE/2FE），适配器、驱动和云台使用统一原始命令限幅，
  旧硬件ID发送入口也查配置，避免再次误发电压。冻结底层/遥控/RTOS未改。
  新主机集成测试使用真实DJI适配器、CAN聚合/注册表及配置，捕获BSP报文：覆盖两轴
  混合电压/电流、正负限幅、停机零指令、旧入口、未使用组不发送、ID/槽位/限幅错误
  拒绝及1FE组全量程。初次测试桩误用BSP通道下标，修正桩后完整host测试通过；
  两车型ARM构建/ELF检查通过，无RTOS入口链接。infantry FLASH/RAM=131720/48264B，
  最终烧录ELF SHA256=de4717c93f00fe9bd3ff39844a2a64597433dd28f7d9daf0053a66b35f722899；
  sentry=134080/48280B。日志build/yaw-current-host-tests.log、yaw-current-build.log、
  yaw-current-sentry-build.log。按持续授权烧录infantry并校验成功，日志yaw-current-flash.log；
  未自动复位运行，已请求用户RESET及橙灯状态，实机行为待验证。说明docs/gm6020-control-modes.md。
  新诊断地址：can1_manager=20000534/can2_manager=20000594，各96B，tx_ok偏移72；
  yaw context=20009F40、pitch=2000A1F8（各反馈时间偏移16）；startup标志20006F71、
  gimbal命令20006EF8、uwTick2000052C、rc_frame_count2000A95C。不得再用旧地址解码。
  下载完成后尝试只读启动状态时提示No debug probe detected（yaw-current-start-state.log），
  因此尚不能确认新固件运行报文或橙灯解除；需接回探针并在下载结束后用户RESET再验证。

- 2026-09-08 重要纠错/最新yaw橙灯原因：用户截图提示“电流控制模式下接收到电压指令”，
  已找到DJI官方GM6020使用说明v1.4（2023.10）第5页明确包含这条橙灯常亮定义。
  先前仅引用v1.2、称橙灯常亮未定义并要求视频/怀疑PWM等，是资料版本遗漏，已向用户
  纠正。官方新版URL：https://rm-static.djicdn.com/tem/17348/RoboMaster%20GM6020%E7%9B%B4%E6%B5%81%E6%97%A0%E5%88%B7%E7%94%B5%E6%9C%BA%E4%BD%BF%E7%94%A8%E8%AF%B4%E6%98%8E20231013.pdf
  当前源码yaw TX2FF/slot0，实机yaw-mode-check.log再确认TIR=5FE00000（2FF）、DLC8，
  这明确是电压指令，变量名current不等于GM6020电流协议；CAN反馈209/温度27℃/ESR0。
  与用户确认的橙色常亮组合，强烈支持yaw内部电流环开启而主控发电压报文的模式不匹配。
  未直接读取电机参数开关：公开209反馈无模式位，用户确认电机未接三芯调参线，本机
  仅蓝牙串口和STLink COM9，未见独立USB串口适配器。不能宣称已读取/修改电机内部配置。
  修复优先用RoboMaster Assistant >=2.7将yaw电流环开关关闭，匹配现有电压控制/PID；
  或后续明确适配电流模式：硬件ID5 TX2FE，量程±16384对应±3A，还需发送组支持、
  限幅、控制参数与测试，不能只把2FF改2FE或照搬±25000。pitch无需随yaw改变。
  电机固件>=1.0.11.2才支持该电流环选项，无需盲目升级。本次仅只读检查及记录，
  未更改固件、发送类型或电机内部设置。

- 2026-09-08 用户确认pitch已能动、yaw仍橙灯常亮，本次yaw-orange-1/2.log实测：
  startup_position_captured=true、gimbal enabled=true、两路ESR=0，CAN1反馈209持续。
  CAN1 TX mailbox1 ID2FF/DLC8/数据9E 58 00 00 00 00 00 00，即yaw电压给定-25000
  （当前程序限幅；源码历史变量名current但GM6020协议实际是电压给定）。不能再归因于
  启动门置零或ID不匹配。yaw反馈从angle3624/speed0到angle3238/speed-14rpm，
  RX寄存器209采样0C 99 FF F0 03 63 1B 00：angle3225、speed-16rpm、温度27℃。
  数据证明有位置/速度变化，尚不能区分手动转动或电机驱动，也不能据此宣称橙灯修复。
  用户随后确认未接PWM三芯线且CAN1仅这一台GM6020，PWM外部输入和同总线另一台
  GM6020重复ID基本排除；橙灯常亮仍非官方v1.2定义，须视频核对或官方Assistant诊断。
  电机输出接近限幅，若机构卡住应先断电再检查；下一步隔离PWM/重复ID并用电机整机
  断电重启或USB转串口/官方Assistant定位自身异常，不能盲目增大PID或改ID。仅只读，
  无新控制输出、固件变更或烧录。

- 2026-09-08 云台不动原因已读到：本次tick0x14752→0x19BAE为新的启动运行，
  两GM6020反馈持续且CAN1/2 ESR=0，remote_online=true、GimbalCmd.enabled=true，
  遥控帧0x173E→0x1D45；第二次pitch_rate浮点原始BEC75EA4（约-0.389），证明摇杆
  命令已到云台。s_startup_position_captured却持续false，因为pitch编码器4018>
  配置上限4000（下限1000）。capture_startup_position将两轴一起阻塞，与底盘离线
  无直接联锁关系；MotorService_CommandCurrent也没有全底盘在线门槛。CAN1 mailbox1
  ID2FF、CAN2 mailbox1 ID1FF均DLC8且数据全0，与未通过启动检查分支一致。
  上一轮startup=true已不代表这次启动状态；需按实际机械行程校准pitch限位或先在
  已确认安全的允许范围内启动，不能仅凭超出18刻度任意放宽机械限位。日志
  build/gimbal-not-moving-1/2.log；本次仅检查和记录，无固件修改或测试输出注入。

- 2026-09-08 最新连接验证成功（build/motor-connection-round8-1/2.log）：CAN1两次
  last_rx_id=0x209，RX0x23AE4→0x2689D；yaw软件5反馈时间持续更新，已与配置匹配。
  CAN2 RX0x8FA31→0x9B239，pitch软件8反馈持续更新；两路ESR=0、TX提交失败0。
  s_startup_position_captured现为true，原先缺yaw反馈导致的启动阻塞已解除。
  yaw/pitch两次速度均0，不能宣称完成运动测试；pitch当前编码器4012，启动标志只
  证明此前捕获完成，并非持续行程安全检查。本次只读，无固件修改、烧录或命令注入。

- 2026-09-08 用户报告yaw GM6020橙灯常亮：核对DJI GM6020使用说明v1.2印刷第5页
  （PDF第7页），手册没有定义橙色常亮，不能据此判为ID冲突。橙色每秒1/2/3/4次分别
  为>100℃、CAN同ID、PWM无法识别、温度传感器异常；橙色快闪为PWM行程校准失败。
  当前软件yaw仍要求CAN1硬件ID5（拨码1/2/3=ON/OFF/ON、反馈209），上次实际观察205
  若来自yaw则是硬件ID1，与软件不匹配；修正映射不能保证同时解除未知橙灯警告。
  建议断电核对ID与PWM连接，再单台隔离/官方Assistant读取异常；第4拨码为终端电阻
  不属于ID。此次仅查官方资料和记录，无固件更改或硬件操作。

- 第七轮用户复位后复查（build/can-after-reset-round7-1/2.log）：两路ESR均持续0，
  TX提交失败均0；CAN1 RX0x5A4C→0x8C4F，两次last_rx_id=0x205；CAN2 RX0x16B74→
  0x234C7，软件6/7/8/9反馈新鲜。CAN1软件1–5仍无匹配反馈，当前0x205与yaw配置
  0x209不匹配，物理通信恢复但角色映射问题仍在。此前中断的第六轮单次读取仍是ACK
  错误，不代表本次复位后状态。本次只读，未改配置、烧录或执行软件复位。

- 第五轮只读复查（build/can-status-round5-1/2.log）：本次启动tick=0x4FCA→0x8361，
  CAN1 ESR持续00800033（ACK错误、Error Passive、未Bus-Off），RX始终0，上一轮
  的0x205本次未出现，软件1–5反馈时间全0。CAN2 ESR=0，RX0x13998→0x20950，
  软件6/7/8/9反馈新鲜。两路TX提交失败均0不代表CAN1已收到ACK。未改固件、未复位。

- 用户再次RESET后第四轮状态已恢复：build/can-after-user-reset-round4-1/2.log，
  tick24954→41004ms，两路ESR持续0、TX提交失败均0；CAN1 RX24473→40526，
  两次last_rx_id均0x205，约1k帧/秒；CAN2 RX98502→163120，软件6/7/8/9反馈新鲜。
  但CAN1软件1–5反馈时间仍0，云台startup_position_captured仍false。当前yaw配置
  CAN1 RX0x209，与观测0x205不一致；若当前CAN1设备是GM6020，则0x205对应硬件ID1，
  不是配置的硬件ID5。不能仅凭报文ID断言电机型号或擅自修改映射，需确认当前接入
  设备与其角色。通信恢复不等于yaw已被识别。本次只读，无烧录、软件复位或输出注入。

- 最新第三轮复查（build/can-status-round3-1/2.log）状态变化：本次启动tick42342→
  54098ms，CAN1 ESR=FFF80057、RX=0；CAN2 ESR=00F80007，BOFF也置位，不能再称
  CAN2发送正常。两路TX接受提交计数均停19，提交失败CAN1 13020→18182、CAN2
  13020→18184；CAN2 RX133152→180479且软件6/7/8/9时间戳仍新鲜，表明当前有反馈
  更新但发送路径异常。CAN1软件1–5反馈仍全0。本次CPU running，只读无复位、烧录
  或电机命令注入。以上覆盖上一轮CAN2 ESR=0的当前状态。

- 随后同次启动复查（build/can-status-round2-1/2.log）：tick=0x247BD→0x27D0F，
  CAN1 ESR仍00F80057/RX=0，TX提交失败0xE763→0xFD5F；CAN2 ESR仍0、RX从0x92616
  增至0x9FCC6、TX提交失败0，软件6/7/8/9反馈新鲜，1–5仍全0。状态与上一轮一致，
  本次只读，无固件或设备状态修改。

- 再次CAN状态检查（本条覆盖上述历史状态）：采样恰逢新启动，tick=1558ms时两路ESR=0、
  TX提交均0，CAN1 RX=0而CAN2 RX=4325；tick=27487ms时CAN1 ESR=00F80057，
  RX仍0、TX提交失败7085，CAN2 ESR=0、RX=108703、TX提交失败0。说明本次CAN1在启动
  后再次进入Bus-Off，不能将启动初期ESR=0当持续正常。当前CAN2软件6/7/8/9均有新鲜
  反馈（两摩擦轮、pitch GM6020、拨弹），CAN1软件1–5时间戳均0（四底盘和yaw）。
  相比上一轮，CAN2 pitch已恢复反馈。中断前一次旧读取曾见两路Bus-Off，已不代表当前。
  本次只读，无复位/烧录/输出注入；日志build/can-status-recheck-1/2.log。

- 最新CAN信号复查覆盖上次正常状态：CPU running，tick7456→24406ms，CAN1原始RX
  两次均0，ESR=00800033（LEC=3/ACK错误、TEC=128、Error Passive，BOFF=0）；
  不是之前的Bit dominant/Bus-Off状态。CAN2 ESR=0，RX21096→72393，约3026帧/秒；
  软件6/7/9（RX201/202/203）有新鲜反馈，软件1–5和8反馈时间均0，当前两台GM6020
  均未进入角色反馈。上次CAN1收到的0x208在本次启动后未再收到，实际接线/供电是否
  改动尚未知，不能沿用此前CAN1已正常的结论或据此断言某部件损坏。TX提交计数增加
  不代表收到总线ACK。本次只读，无固件修改、烧录、软件复位或电机测试输出；
  原始日志build/can-status-latest-1/2.log。

- CAN1换线隔离测试成功：用户将原CAN2硬件ID4 GM6020直接接到CAN1。最初未复位时
  仍是旧Bus-Off（ESR=FFF80057/RX=0）；用户随后确认RESET，实测tick5750→20074ms，
  CAN1原始RX从5270→19594，差值14324帧/14324ms，约1000帧/秒，last_rx_id=0x208，
  CAN1 ESR=0、TX提交失败0；CAN2 ESR亦0。证明现有固件与C板CAN1路径可与这台电机
  通信，不能继续将CAN1固定归为板载收发器故障。原CAN1电机/线束/连接和终端组合需逐一
  隔离，具体故障件未确定。固件无自动Bus-Off恢复，换线后须清除旧错误才能有效对照。
  此测试只验证通信：配置仍将pitch的0x208绑定CAN2，CAN1未注册0x208，原始回调能收到
  但电机角色解码不会接管，不能以此宣称换线后pitch已可控制。日志build/can1-swap-test-1/2/3.log；
  本次仅只读检测及备忘录更新，无固件更改、烧录、软件复位或输出注入。

- CAN1再次排查：复位前CAN1 ESR=00F80057、RX=0，CAN2也曾出现ESR=00F80007和
  TX提交失败持续增长（其RX仍更新，不能以有反馈推定发送正常）。用户确认刚按RESET后，
  tick降至0x6527，CAN2 ESR恢复0、TX提交失败0、pitch新鲜反馈/角度3763；CAN1重新
  出现ESR=00F80057、RX=0、yaw反馈时间0、TX提交失败6063。启动门仍未通过。
  CAN1 LEC=5为Bit dominant error，不能误称单纯ACK缺失，也不能仅凭此断言收发器损坏。
  CAN1/2 BTR=00180002，APB1=36MHz对应1Mbps；PD0/PD1 AF9、滤波bank0/14分区正常。
  两路MCR ABOM=0，Src/can.c AutoBusOff=DISABLE，BSP只开RX通知，未发现软件Bus-Off
  恢复；这是持续锁定的代码缺口，但CAN1复位后复发证明还需定位最初通信错误。
  tx_ok只统计HAL_CAN_AddTxMessage接受提交，并非总线已ACK成功，修正此前统计表述。
  下一步用已验证CAN2线束/单个电机隔离CAN1物理路径并观察原始RX，不盲目更改ID、
  波特率或取消云台联锁。日志build/can1-recheck-1/2/3.log和can1-recheck-after-reset.log；
  本次仅只读检查与备忘录更新，未修改固件、烧录、软件复位或注入输出。

- CAN2 GM6020 不动的只读诊断：CPU running，tick=0xB6CE5；软件ID8的pitch反馈时间
  0xB6CE4（相差1ms）、编码器0x1709=5897，CAN2 ESR=0、发送成功计数非零且发送错误0。
  软件ID5的CAN1 yaw反馈时间仍0，CAN1 ESR=00F80057（Bus-Off）。云台命令enabled=true，
  但s_startup_position_captured=false。gimbal_controller.c的capture_startup_position要求
  两轴均收到反馈，且pitch位于配置1000–4000内；当前yaw缺反馈首先阻塞，pitch当前读数
  也不满足后续范围检查。on_gimbal_cmd未就绪分支将两轴输出置0，因此CAN2通信正常
  不能保证pitch获准输出。需恢复yaw反馈并核实pitch实际机械行程/编码器范围，不能直接
  放宽到全量程或删除联锁。原始读取build/can2-gimbal-gate-read.log；本次未改固件、
  未烧录/复位/注入电机命令，仅更新诊断记录。

- 遥控链路已恢复（本条覆盖此前零帧的当前状态）：用户发现DR16绿灯闪烁，查询DJI
  DT7/DR16官方手册确认这表示检测到遥控信号但未连接，正常为绿灯常亮；用户重新对频后，
  未修改/重烧/复位固件，仅HOTPLUG读取：RxEvent和DMA完成及有效帧同步从3602→5959，
  tick472315→505300ms，接收约71.46帧/秒；每帧18字节，UART错误/非BUSY启动失败均0。
  原先HAL_BUSY累计38238不再增长，说明当前正常逐帧重装。CmdController remote_seen=true、
  remote_online=true，last_remote_ms持续更新；s0/s1从2/1变3/3，五摇杆通道两次均0。
  CPU运行正常、CFSR/HFSR=0。已实测USART→DMA→原版回调/解码→消息→命令输入贯通。
  同一固件在重新对频后由零帧恢复，说明此前正常运行仍零帧的关键原因是无线链接未建立，
  不能再继续将当前问题归为HAL/时钟不兼容；之前“已对频”口头确认未核实灯态造成误判。
  独立问题仍有CAN1 ESR=00F80057（Bus-Off），CAN2 ESR=0，不能以遥控恢复宣称所有电机正常。
  原始记录build/remote-after-link-1/2.log。本次仅读取设备并更新备忘录。

- 原因分析新增实机证据：用户确认上次RESET之后又烧录过，最新cube-flash.log为22:37:32，
  晚于正常运行采样22:36:22。再次读取CPU实际停HardFault：PC=0x0800C3CC、LR=FFFFFFF9、
  MSP=20000BCC、HFSR=80000000(DEBUGEVT)，RCC/USART/DMA/NVIC尚未初始化，旧诊断地址
  是下载器RAM代码而非应用变量。HardFault函数push r7，异常硬件栈从MSP+4开始：堆栈PC=
  0x20000000，该处半字BE00(BKPT #0)，与RAM下载器退出断点吻合。烧录成功不代表应用启动；
  本机run_after=false，tools/firmware.py仅下载前-halt，无校验后reset/run。flash-plan中的
  “remain halted”并非已保证的行为，说明不准确。此次未改脚本或重新烧录。
- 用户随后再次确认RESET；只读验证PC回到正常应用、MSP接近20020000、CFSR/HFSR=0，
  但有效帧/接收事件/错误仍0。将当前“烧录后未启动”与原先“正常运行仍零帧”分开，
  前者已由用户复位解除，不能以此宣称后者根因已解决。
  正常运行时128次离散采样：PC11全部高（GPIOC其他位有变化），USART3 SR均C0，
  DMA NDTR均18；AHB1ENR=007000CF、APB1ENR=16840004（相关时钟已开）、DMA LISR=0，
  NVIC接收中断已使能且无pending，PC11 AF7/模式正确，BRR=168hex/CR1=351C/CR3=41。
  因此当前没有看到UART→DMA接收进展，单纯解码/消息映射故障解释不了该前端现象。
  离散采样不能证明完全无脉冲，也不是示波器波形；不得据此推断硬件损坏。HAL_BUSY为
  已挂接接收未结束的结果；原版离线重试只调用ReceiveToIdle，BUSY路径不重置DMA。
  仍需用已验证固件或仅UART/DMA的最小固件做同硬件对照，隔离整机初始化/时钟/HAL差异。
  本次日志build/remote-reason-core-state.log、remote-reason-exception-stack.log、
  remote-reason-after-reset.log、remote-reason-running-samples.log；第一次passive-samples
  采集发生在HardFault状态，不能用于遥控物理输入结论。本次仅更新备忘录，无固件修改。

- 用户改为“直接照搬nyush-rm-control里面的遥控器代码链路，再次尝试”，本次以新仓库
  2b7ca720e856f1efda9f6de6259faea42654720d为基准，不再沿用Dart恢复实现。
  克隆在work/nyush-rm-control-reference；原先本机代码和ELF备份work/pre-nyush-remote/before.zip。
  third_party/nyush_remote下复制原始USART服务、remote、daemon、CRC16共8个源码/头文件
  及MIT许可证；逐字节对比通过，source.json记录来源/提交/哈希，原始文件未改动。
  用bsp/remote/nyush_usart.c及modules/remote/nyush_remote/nyush_daemon/nyush_crc16.c
  编译包装原文件。HAL回调改名以保留WT61C分支；接收API观测包装只记录结果，不改变
  原版重试行为；CRC符号加前缀避免视觉协议冲突；原版IRQ日志改为计数，避免阻塞。
  daemon未使用的bsp_dwt/buzzer头由仅用于包装源文件的兼容头满足，无RTOS依赖。
- 接收实际链路：RemoteControlInit(huart3)→USARTRegister→ReceiveToIdle_DMA(18)→
  HAL IRQ→原版注册表回调→原版DBUS解码/按键状态/DaemonReload→清缓冲→重装接收。
  原版离线每10ms调USARTServiceInit，HAL_BUSY时不Abort；已删除Dart强制Abort/200ms恢复。
  初始化提前到main建立消息中心后（云台对齐/校准前），初始化时短暂关IRQ，保证原版
  分配的串口和daemon实例指针就绪后才回调。现有bare-metal dispatch每10ms调原版DaemonTask。
  原版decoder也处理部分长度事件，保持其源码行为；桥接层仅将18字节完整事件发布给
  当前机器人，保留200ms命令失联关闭。原版RC_ctrl_t通过独立快照适配当前消息结构。
  USART3/DMA配置与参考一致；整机72MHz时钟、HAL1.8.5及其他外设未替换，不能宣称整固件相同。
- 验证：真实复制USART注册器+解码器+daemon→当前消息中心→命令控制器的主机集成测试
  通过（仅HAL/time/log伪造），覆盖TC/IDLE、快照、BUSY无Abort、键位/通道、超时、UART
  错误和重连。替代旧Dart BSP专项测试并接入tests/host/run_tests.sh。两车型ARM构建/ELF
  检查通过，无RTOS符号；infantry FLASH/RAM=130744/48216 B，ELF SHA256=
  4170a69c52b30419ea0fbc91b6533819fa07fe0cd04f669b5991bbcfba8dbe4c。
  已实际烧录infantry Debug并校验成功，日志build/nyush-remote-flash.log；未自动复位运行。
  用户在下载结束后确认RESET；两次实测程序正常运行、无HardFault，rx/idle/TC/字节/
  有效帧/UART错误均0；初次HAL启动成功，离线重试HAL_BUSY从4600→7120，非BUSY失败0。
  接收实例0x2000B660的缓冲全0，DMA NDTR=18、M0AR正确指向该实例缓冲；daemon实例
  0x2000B770 reload=10/temp=0，GPIO PC11 AF7生效。此版确实执行了原仓库链路，但实机
  遥控仍未恢复；不能把HAL_BUSY当新硬件错误，也不能宣称照搬后问题已解决。
  日志build/nyush-remote-after-reset-1/2.log，用户RESET后只读，没有再次下载。
  新诊断s_diagnostics=0x2000B4C8，52字节；rc_frame_count=0x2000A92C，last_sbus_frame=
  0x2000A930，rc_usart_instance=0x2000A9F0，rc_daemon_instance=0x2000A9F4，protocol=0x2000A9F8。

- 对齐范围复核：用户确认Dart可用是在同一块C板、同一个DR16/遥控器/线束上，只换固件。
  后续优先定位工程配置/初始化/驱动差异，不以零帧判硬件损坏。用户问为何Dart可用：
  尚未获得可复现的根因，不得将下述差异直接当成结论。
  纠正先前说法：参考提交2048b42b的Src/main.c实际也是裸机while，DaemonTask每10ms
  运行，不依赖RTOS。此前“未移植Dart RTOS”的说法错误，已向用户明确纠正。
  Dart时钟SYSCLK168/APB1 42MHz、HAL1.8.1；本项目72/36MHz、HAL1.8.5，均HSE12MHz。
  当前BRR360对应100kbaud，主频不同本身不证明串口波特率错误。Dart接收在RobotInit
  中初始化，本项目在BMI/云台对齐/校准后才初始化，恢复调用挂在消息dispatch之后。
  当前为Dart接收生命周期的项目适配，并非整个固件/驱动/运行环境完全相同。
  UART关键函数差异已核对：新版去除部分锁、检查DMA启动返回值、记录RxEventType；
  未发现可直接解释当前零事件的确定错误，不盲目降级整套HAL。
  最新只读HAL状态ReceptionType=TOIDLE(1)、RxState=BUSY_RX(0x22)、DMAState=BUSY(2)、
  ErrorCode=0，仍零接收事件；日志build/dart-parity-audit-live/hal-state.log。
  NDTR18是会被每200ms恢复重装的瞬时值，不能证明整个期间从未收到任何字节。
  本次未改固件或下载；已询问用户实际可用Dart镜像路径，用于后续同硬件基准对照。

- 用户进一步明确“完全按照dart中遥控器路径实现”，本次授权覆盖遥控接入必需的
  Src/main.c、Src/stm32f4xx_it.c、Src/usart.c、Src/dma.c 及 .ioc；不扩展其他底层范围。
  参考 Dart 2048b42bd0d1b04d81484dc826c214e81f975156，本次已替换上一版兼容路径：
  USART3 100K/9B+EVEN、PC11 AF7、DMA1 Stream1 CH4 NORMAL BYTE/LOW，DMA/UART IRQ均5。
  UART IRQ只交给HAL；main的RxEvent增加USART3分发，保留USART1 WT61C分支。
  BSP统一ReceiveToIdle_DMA(18)，回调先解码/发布，再清已收字节，最后重新接收；
  正常重装不Abort，HAL_BUSY/ERROR才Abort UART+DMA、清SR/DR与ErrorCode并重试一次；
  ErrorCallback立即强制恢复。关闭HT，已删除普通RxCplt和手动IDLE实现。
  .ioc同步DMA优先级、中断优先级并补USART3 NVIC使能，避免重新生成丢失配置。
- 裸机适配：保留现有消息中心和命令数据接口，用after-dispatch每10ms执行一次Dart
  daemon相同倒计数（初始100，有效18字节帧重装10，计数为0后的下一次检查清零数据）。
  离线强制重启限频200ms；错误恢复不等待该限频。数据解码对齐五通道±660外置0，
  DBUS mouse.z=0，保留当前键位bitmask与控制映射。Dart的VTM协议、组合键计数结构未移植；本项目命令控制层200ms失联禁用策略保持。这些是项目接入差异，
  不再将“参考部分机制”表述为整个Dart模块逐字相同。裸机恢复临时屏蔽接收两个IRQ，
  保留SysTick及原IRQ使能状态；遥控状态清零通过既有短临界区避免与解码并发。
- 验证：Windows GCC -Wall -Wextra -Werror两项遥控测试通过：TC/IDLE处理顺序、
  正常无Abort、立即错误恢复、BUSY重试、最终失败、IRQ状态；真实解码→消息→命令、
  初始等待、100ms倒计数与200ms重启限频、残帧不喂狗、通道限幅、失联及重新连接。
  infantry/sentry Debug ARM构建及ELF检查通过，均无RTOS符号。infantry FLASH/RAM=
  128168/47808 B，ELF SHA256=35493a14ce25000a26a10728e020a86570dae66ec186b4ce99487e9e3fd5344f。
  infantry固件已对SN066EFF535548877187253144实际下载并校验成功，未自动复位运行；
  日志build/dart-rxevent-flash.log。用户随后确认RESET；只读三次观测程序正常运行、
  CFSR/HFSR=0，接收事件/字节/有效帧/UART错误均0，强制恢复210→411→546且HAL失败0。
  DMA NDTR18，CR=0x08000417，USART3 CR1=0x351C/CR3=0x41/BRR=360；PC11复用AF7，
  单次引脚采样为高；NVIC DMA1 Stream1及USART3均使能、优先级字节均0x50，无待处理中断。
  当前没有观察到USART3向DMA交付字节；不能以此断言接收机或线束损坏，遥控实机尚未恢复。
  CAN1仍ESR=0x00F80057/Bus-Off，CAN2 ESR=0，是独立未解决项。日志为
  build/dart-rxevent-after-reset-1/2/3.log；用户RESET后没有再次下载或软件复位。
  新符号：diagnostics=0x2000B33C（56B），rc_frame_count=0x2000A94C，last_sbus_frame=
  0x2000A950；watchdog_count=0x2000A93E，lost_restart_count=0x2000A948，uwTick=0x2000052C。
  先前两文件接线草案已落实并由本次完整修改取代。之前264帧/9错误是旧固件状态。

- 最新反思/链路复查：Flash读回128216 B与当前a5a68f12 ELF派生BIN完全一致。
  状态已经变化：DMA完成和已解码帧均264，remote_seen=true；最后解码五通道均0、
  s0=2/s1=1，CmdController保存的数据与接收快照一致。因此实机接收→解码→消息派发
  曾经贯通，不能再沿用“从未收到帧/回调不通”的结论。连续采样帧数264不再增加，
  uptime=221410ms，last_remote_ms=41481ms，已失联179929ms，remote_online=false。
  UART错误累计9，last_uart_error=0x4（HAL帧错误）；恢复次数1147→1288、启动失败0、
  NDTR=18。当前故障边界为持续串口输入中断或接收条件异常，尚无同步波形证明根因。
  CAN1仍ESR=0x00F80057/Bus-Off，CAN2 ESR=0，是独立故障，勿混成遥控问题。
  记录 build/remote-reflection-state.log；本次只读设备、仅更新备忘录。

- 按用户要求参考 Dart，已克隆至工作区 work/Dart-reference，提交
  `2048b42bd0d1b04d81484dc826c214e81f975156`。对照 remote_control.c、bsp_usart.c、
  usart.c 和 robot_config_select.h：默认 USART3/DBUS/18字节/100K/9B+EVEN；
  Dart 使用 HAL ReceiveToIdle DMA、错误恢复和离线后200ms限频重启。未复制整模块。
- 当前接收实现更新：用 HAL_UART_Receive_DMA 接收18字节，DMA完成后重装接收并
  发布栈上快照；原USART3 IDLE入口负责丢弃残帧并重新对齐。采用标准RxCplt/Error
  回调以兼容冻结main中已有的WT61C RxEventCallback，不覆盖其他串口。
  USART错误延后到裸机dispatch结束时恢复；200ms无完整帧时中止并重启接收，
  HAL失败不会按主循环频率忙重试。取消手动DBM，不启用RTOS、不修改冻结文件。
  DMA完成抢先于UART错误IRQ时检查错误标志并丢包；看门狗避免IRQ更新时间稍晚于
  主循环采样时间造成无符号下溢误重启。新增完成/重启/失败/HAL状态/错误诊断字段。
  去掉遥控IRQ日志，保留现有消息载荷、键位及200ms失联输出策略。
- 本次 Windows 主机测试通过 HAL完成、快照所有权、残帧/空IDLE、错误/200ms恢复、
  HAL_BUSY限频重试、IRQ使能状态、并发时间戳及DMA优先错误丢包；真实解码→消息中心→
  控制器集成测试通过并检查恢复钩子。ARM infantry Debug构建与实际下载校验通过，
  FLASH/RAM=128216/47840 B，ELF SHA256
  `a5a68f1226a57610d033b38df55bd858f4e34fc6c793109151f2ec5fe7f9c91a`。
  保留校验后不自动复位运行。首次用户复位可能与最后一次下载重叠，读取仍为下载器
  RAM残留和HardFault。再次按RESET后运行正常，无HardFault：CR1=0x351C，CR3=0x41，
  DMA CR=0x08030417，NDTR=18。连续采样重启次数175→457、HAL失败数0、HAL_OK，
  但idle/接收字节/DMA完成/有效遥控帧均0，remote_seen=false；接收服务持续运行，
  新恢复机制已实测生效，遥控输入仍未恢复，不能宣称整条链路修复成功。
  复位后原始记录 build/dart-remote-after-reset-2/3.log。下一步需能区分DR16输出、
  板载反相后PC11信号和MCU接收的波形证据，当前不能仅凭无帧断言硬件损坏。
  最新诊断地址0x2000B364（40字节），last_receive_ms=0x2000B38C，fault_pending=
  0x2000B390；有效帧仍0x2000A974。不要把旧版本字段偏移用于新固件。

- 本机 DR16 链路排查与接收修复：读取 Flash 127320 B 与 966a36c 本机 BIN 完全一致，
  确認 USART3 CR1=0x341C（9B+EVEN）已在板上生效；APB1=36MHz、BRR=360，
  PC11/AF7、DMA1 Stream1 channel4、DMAR/IDLEIE/NVIC 和 USART3 中断向量均对应正确。
  用户确认 DBUS 接线；采样起初 callback_count/rc_frame_count 均0，后有一次非18字节回调，
  缓冲开头为00 FE 00，其后未持续收到帧。此证据不能断言硬件损坏，也不能说遥控已恢复。
- 修复 bsp/remote/bsp_rc.c：IDLE 时先停止 DMA 再取长度，保留尚在 DR 的 RXNE 帧尾字节，
  不再直接返回并丢字节；PE/FE/NE/ORE 错误包不发布，重装缓冲继续接收。
  新增 BspRc_GetDiagnostics 只读诊断视图（idle事件/字节数/错误包/最后状态及长度），
  字段由ISR更新，不作为控制输入。未修改冻结 Src/Inc/.ioc 或启用 RTOS。
  966a36c 更新已覆盖之前 CAN 滤波器补丁，本次恢复两路 CAN2SB=14；CAN1 Bus-Off
  仍需实测，不能以滤波修复声明消除总线故障。
- 新增 host remote BSP 回归测试，验证双缓冲、17字节+RXNE补齐、四类UART错误丢弃、
  空包、恢复和非IDLE不消费数据；Windows UCRT GCC 严格编译/运行通过，接入 run_tests.sh。
  infantry Debug ARM构建通过，FLASH/RAM=127336/47824 B，ELF SHA256
  `62702170983de51e5e1f020289c20e72e72521a80d2701a5d847901e9cbadfaf`。
  按用户此前重烧授权执行 tools/firmware.py flash，下载及校验成功，未执行校验后复位运行。
  HOTPLUG后实测PC=0x0800C3B2、XPSR=0x21000003（HardFault），不能宣称保持halt或
  新固件已经正常启动。随后用户确认按RESET，新固件正常运行，CFSR/HFSR均0；
  tick从52920至147189期间，新诊断idle事件/接收字节/错误包计数一直0，NDTR36，
  rc_frame_count=0、remote_seen=false。因此接收异常路径修复尚未恢复实机遥控，
  不能据此断言硬件损坏；尚需确认实际输入信号为何没有触发USART3接收。
  当前 ELF 诊断地址0x2000B36C，有效帧0x2000A974，解码回调计数0x2000A98C；
  每次固件变化必须重新核对符号。原始记录 build/dr16-diagnosis-before/latest.log。
  复位后记录 build/dr16-after-reset-1/2.log；新增 test_remote_chain 使用真实解码、
  消息中心及命令控制器，仅替换UART传输和日志，在主机注入18字节帧后命令正确产生，
  17字节帧被拒绝、200ms失联关闭命令测试通过；无硬件命令注入。两项测试均接入host入口。

- 用户明确要求修复 DR16 串口格式，授权本次修改冻结的 `Src/usart.c` 与 `.ioc`：
  USART3 从 100K 8N1 改为 STM32 HAL 的 9B+EVEN，即 DBUS 要求的 8 数据位、偶校验、
  1 停止位；DMA 字节对齐和 18 字节解码不变。额外审查文档及索引已按用户反馈撤回。
  Arm GCC 13.3.0 的 Cortex-M4 语法检查及配置一致性检查通过。其余 CAN 滤波、云台算法
  与限位发现未在本次修复，未烧录或实车验证。
- 用户要求将本次 CAN 配置修复与备忘录提交并推送至 origin/py；推送前 fetch 确认远程
  仍为 77073ad，无新增提交。沿用刚完成的双车型配置测试、9台映射检查和 ARM 语法检查；
  本次提交不包含工具安装、本机配置、构建产物或底层改动。
- 已核对 py 77073ad 原 motor_id 与用户硬件编号完全对应，并按用户要求修复步兵配置：
  底盘 CAN1 硬件 ID1–4、yaw CAN1 ID5、左右摩擦轮 CAN2 ID1/2、pitch CAN2 ID4、拨弹 CAN2 ID3。
  yaw RX209/TX2FF/slot0；摩擦轮 RX201/202、TX200、slot0/1；pitch RX208/TX1FF/slot3；
  拨弹 RX203/TX200/slot2（报文 ID 均为十六进制、slot 从0计）；底盘原收发配置正确并保留。
  为满足现有接口的全局唯一要求，软件 motor_id 为底盘1–4、yaw5、左/右摩擦轮6/7、pitch8、拨弹9。
  更改仅涉及 config/robots/infantry_standard.c 的逻辑 ID、收发 ID、槽位及相关注释；
  接口结构、角色顺序、PID、方向、限幅和底层均未改。逐条9台映射、RX/TX槽位冲突、
  非ID参数不变检查通过；现有 GCC 双车型配置测试和 Arm GCC 13.3.0 Cortex-M4 语法检查通过。
  未安装工具、未完成固定版本整固件构建、未烧录或实车验证。
  四底盘 ID 对应实际轮位、拨弹实际是否 M3508（当前类型）仍未由用户确认。
- DT7 逻辑核查（py 77073ad）：ch0/ch1 控制云台 yaw/pitch，ch2/ch3 控制底盘平移，ch4 控制
  底盘旋转；s0（右拨杆）下=停发射、中=摩擦轮、上=摩擦轮+连续供弹；s1（左拨杆）
  下=普通、中=云台参考坐标平移、上=固定 0.33 归一化角速度小陀螺，非急停开关。
  物理左右按相邻 nyush-rm-control 的同一 DBUS 位域命名交叉核对，未连接实物验证。
  有效视觉自动优先控制 yaw，pitch 始终手动；200 ms 无遥控更新后路由输出清零。
  清零不等于整机立即零电流：底盘仍作零速控制，发射速度斜坡下降；云台请求零电流。
  已用现有 GCC 严格编译并通过 test_command_router；未安装工具、未改控制代码或烧录。
  核查时 py 的重复 motor_id 使 MotorService_Init 校验失败；现已由上方配置修复消除重复，仍待整机验证。
- 本机配置查找与撤回：未在已搜索的项目父目录、桌面、文档和 D 盘找到旧配置；
  当前原有 MSYS2 Arm GCC 为 13.3.0、just 为 1.47.1。bootstrap 曾新增 just 1.46.0、
  CMake 4.2.3、Ninja 1.13.1、Arm GCC 14.3.1；用户随后要求撤回，已删除本次新建的
  `Local/Programs/FirmwareTools` 目录、四个下载包及 `.firmware.local.json`。
  安装进程已结束；未执行 configure、固件构建或烧录，未改系统 PATH、VS Code 设置或原有工具。
  后续暂停安装，先根据用户指定的现有环境继续；历史环境记录不证明当前电脑已配置。
- 已按用户要求 fetch 并切换到跟踪 `origin/py` 的本地 `py`，HEAD 为 `77073ad`
  （motor id config）；保留本地 CAN 手册备忘录改动，main 仍在 `976f100`。
  远程仅修改 `config/robots/infantry_standard.c`。GCC 严格编译配置测试通过，但运行
  `tests/host/test_robot_config.c:13` 当时断言失败：motor_id 1/2/3/4 重复，后续本地修复见上方。
  `python tools/firmware.py build infantry_standard` 因当前缺少本机保存配置而停止，
  本次未完成 ARM 构建；HEAD 与 origin/py 一致且 diff --check 通过，未烧录。
  进一步核对：缺失的是仓库根目录被 Git 忽略的 `.firmware.local.json`；脚本在工具版本
  检查前即停止，不能据此判定编译器未安装。当前 PATH 能找到 Python、Arm GCC 和 just；
  CMake/Ninja 的其他安装位置及整套固定版本尚未核验。
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
- 2026-09-07 复核当前 main：未检查 CAN_Manager_Start 返回值，也未提前调用 MotorService_Init；
  电机服务在命令入口校验整套配置，失败则拒绝命令。旧“main 初始化失败即安全退出”描述不符合当前源码。
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

- 2026-09-07 用户提供仓库外的《RoboMaster  开发板 C 型用户手册 (2).pdf》（桌面
  `Robomaster/用户手册/`）；仓库 `docs/official-docs/` 仍仅有 GM6020 与 C620 PDF。
  已提取并目视核对手册印刷第 11–12 页：J22/J23 共用 CAN1（2-pin，1=CANL、2=CANH）；
  J20/J21 共用 CAN2（4-pin，1=5V、2=GND、3=CANH、4=CANL）。两路独立总线须分别接
  CAN1 与 CAN2；CAN2 转两针通信线应取 H/L，不能将 5V 接入电机 CAN 信号口。
  本次未改代码或接口，无需固件构建；实际线束与电机型号仍待确认。
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
