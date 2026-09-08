# RoboMaster Control — Project Memory

每次任务开始先读本文件；代码、配置、架构、硬件假设或验证状态变化后，结束前
更新本文件。它用于防止跨任务遗忘，不替代源码和实车记录。

## 当前任务（2026-09-07）

- 2026-09-09 终端电阻说明：电压/电流模式不改变CAN终端规则；GM6020第4拨码ON启用
  内部终端，是否启用取决于它是否位于主干物理末端且该端没有其他终端。高速CAN主干
  两端各120Ω，中间节点关闭；全部断电后完整总线CAN_H-L通常约60Ω，但读数不能证明
  电阻位置正确。C板/C620现有终端配置与实物拓扑尚未确认，不能直接要求yaw一律ON或OFF。
  说明以本地GM6020手册与TI终端文档为依据，本轮无硬件或软件修改。

- 2026-09-09 按用户要求直接查本地docs/official-docs/RM_GM6020_Docs.pdf，实际是英文
  User Guide v1.4/2023.10，共13页；PDF技能、pypdf文本与Poppler渲染页6/7/8完整视觉核对。
  印刷页6/PDF7明确电压模式：1FF对应ID1–4、2FF对应ID5–7，ID5为DATA0/1，高字节在前，
  标准数据帧DLC8，范围±25000；与当前yaw5配置一致。印刷页5/PDF6：橙常亮=电流模式
  收到电压指令；第4拨码是CAN终端电阻开关，不是电流环开关。前三位ID5为Bit2:0=101。
  PDF7说明电机固件v1.0.11.2及以上配RoboMaster Assistant v2.7及以上，参数中的Current
  Ring On/Off Switch启用电流模式；只能确认板发电压，未读取电机内部开关实际设置。
  PDF8电流指令1FE/2FE、范围±16384对应±3A；该英文版2FE表Motor ID列印为1/2/3，
  与分组电机ID语义存在疑点，不能无证据将表中文字当作5/6/7直接引用，当前电压确认不受影响。
  原PDF未编辑，不修改任何固件或摩擦轮逻辑。渲染缓存位于work/gm6020-manual/。

- 2026-09-09 yaw指令模式复核：当前配置GM6020_COMMAND_VOLTAGE、CAN1 TX2FF slot0、
  硬件ID5/RX209、限幅±25000原始电压刻度。实机邮箱TIR5FE00000确为2FF，8字节数据
  全0、TXRQ=0，表示采样邮箱内容为零电压，不能据此声称已送达yaw。没有修改模式，
  摩擦轮逻辑未动；记录build/yaw-command-mode-latest.log。

- 2026-09-09 最新CAN检查（再次改变）：用户已重编译/烧录，Flash137240B匹配当前本地
  BINfbb598d27e346004ae612272297fb33b1549735bed7d0dd36afdd45d3b385635；已核对ELF中
  RAM地址仍相同才解码。底盘四轮实机Kp均2、Ki0，ID1/4 Kd0.1、ID2/3 Kd0；上一轮
  “Kp为0”不再适用。两次tick69803→89872，CAN1 ESR00F00003→00600001（采样BOFF=0，
  先错误被动、后错误警告），但faults/recoveries291/291→390/390，约20秒新增99次，
  恢复超时0。RX268977→346344，提交12032→16060、失败1728→2318，持续新故障仍在。
  CAN2 ESR0、fault/recovery0，RX279088→359882、提交13023→17362、提交失败7→9，
  少量提交失败不等同于CAN2硬件Bus-Off。四轮及CAN2四电机反馈新鲜；yaw反馈时间0，
  本次启动从未收到。遥控在线零杆、armed=0、四底盘输出0，当前控制受CAN故障联锁。
  本轮未确认是否重新接yaw、换线或调整终端，不可把早先隔离后稳定泛化为当前稳定，
  也不可凭yaw无反馈直接确定接线状态。未改任何控制逻辑/参数、未烧录复位，摩擦轮
  用户冻结要求继续有效。build/can-check-latest-a/b.log/json及RAM、a-flash为本轮证据。

- 2026-09-09 用户确认摩擦轮已恢复，并明确要求以后不要再修改摩擦轮逻辑；将此作为
  持续约束，后续底盘/yaw工作不得顺带更改摩擦轮控制。当前请求仅诊断底盘不能动。
  两次HOTPLUG只读Flash137240B匹配95f9d633 ELF/962baf2f BIN；tick77225→118567，
  CAN1/2 ESR均0、faults/recoveries均0、TX提交失败0，CAN1 RX308891→475300，提交
  14532→23804；CAN2 RX308881→475292，提交13523→22234。四轮反馈新鲜、seen全1，
  全局armed=1，遥控online=1但两次零杆，chassiscmd/targets/running/outputs均0，CAN1
  mailbox0 ID200全0。不能把本次不动归因为CAN Bus-Off、缺轮反馈或未解除全局联锁。
  明确配置问题：源码与实机底盘PID一致，ID1/4 Kp=Ki=0、Kd=0.1，ID2/3 Kp=Ki=Kd=0。
  PID微分基于测量(last_measure-actual)，静止测量无变化时也为0，故目标变化不会凭微分
  产生起步驱动力；前两台全零PID始终无控制输出。需恢复仅底盘ID1–4的有效速度环增益，
  本轮只诊断，未改PID/控制代码、烧录、复位或要求推动摇杆进行运动测试。
  记录build/chassis-no-motion-a/b.log/json及RAM、a-flash.bin；目前零杆采样不证明已实测
  非零杆量运动链路，但全零P/I缺陷已由源码与实机PID直接确认。

- 2026-09-09 按用户“修改成最开始的摩擦轮逻辑”恢复正常下档减速/零速闭环：
  shooter_controller不再因friction_enabled=false立即清摩擦轮ramp/PID或将电流强制0，
  而恢复每次回调500RPM降至0、反馈新鲜时持续运行速度PID。运行目标±7500和PID不变。
  CAN未解锁时仍清摩擦轮ramp/PID并输出0，反馈超时清PID/零输出；保留上电先下档保护、
  CAN恢复、消息派发上限、云台保护及当前拨弹逻辑。不是回退全部工程或解除故障保护。
  专项测试验证正常下档7500→7000斜坡、零速目标下实际正转会得到负制动电流、CAN锁定
  立即零/清目标、陈旧反馈不制动，已通过。全套初次在底盘测试失败：用户最新实际底盘
  kp全改0（ID2/3的kd也0），原测试依赖非零现场增益。保持用户参数不变，仅在该测试
  拷贝的四底盘motorconfig中设置固定测试kp10/ki0/kd0，完整host suite随后PASS。
  两车型Debug ARM通过，RTOS仍未链接，diff --check通过；Infantry FLASH137240/RAM48384，
  ELF95f9d633fea01f309c7cb470b68da76ca66da22bfa53949a5a77358e0a57d61d，
  BIN962baf2fd4eebec71adb856129f296ae656be52c6a6293dde67b37f3056f8e51；Sentry138632/48384。
  infantry已通过tools/firmware.py下载校验，run_after=false，没有自动启动或实车制动测试。
  下载时摩擦轮专项/ARM已通过，全套底盘fixture修正后的复测在下载后完成，仅测试文件变化。
  日志build/friction-restore-host/arm/sentry/flash.log。用户需机械安全后人工RESET生效。
  新代码/ELF可能使后续符号变化，读取RAM应重新核对地址与Flash哈希，不直接套旧解码器。

- 2026-09-09 用户报告摩擦轮右拨杆下仍不停：本轮先提示切断摩擦轮动力，再只读。
  用户已另行重新构建/烧录，Flash137208B匹配本地最新BIN7025f9a9852a4b40d55ae35c2b02cd01e98445f64101091424c26277705640ff，
  ELF5df1de80842ba5fdbec58adb9ca33716b9f2e6c80af76c89d6ee4fe06667105f，manifest本地时间
  2026-09-09 00:07:52。与上轮ad77f9f3 Flash仅16字节配置数据不同，代码和已核对的
  RAM符号地址相同，故在确认新ELF符号后才解码，未直接套旧固件结论。
  两次tick123346→173876，remote_online=0、RC计数2237未更新，拨杆残留[2,2]，
  shootcmd=[0,0]、ramp与四输出全0、armed=0。所有电机反馈停在约31.8秒，CAN2
  RX125693固定、摩擦轮残留转速-2704/+2624RPM是约142秒前的旧反馈，不代表现在仍在转。
  CAN1/2 ESR均00F80057并反复恢复（faults2774→4314），当前可能已断电，不能用这些
  断电后读数确定事发时总线故障。当前邮箱采样仅有1FF零帧，未采到200帧，不能声称
  已证实200零帧送达电调。源码rc.s[0]来自nyush switch_right，DOWN=2，路由条件正确。
  明确上轮变更语义：shooter禁用立即输出零电流，不做零速PID制动；这表示自由滑行，
  不保证立即停转。已向用户说明此前未明确这个区别，询问是持续高速/加速还是逐渐减速。
  用户确认“逐渐减速，但很久才停”，与零电流自由滑行相符；据此解释停机行为，不再把
  用户现象描述为持续加速或右拨杆无效。要缩短停止时间需另行实现反馈有效时的限流零速
  制动，低速后再零电流，不能直接输出大反向电流或在反馈离线时闭环制动。
  此轮未改代码/烧录/复位/施加输出。
  记录build/friction-stop-a/b.log/json及RAM、a-flash.bin。

- 2026-09-09 yaw支路隔离实验有明确结果：用户按要求断开CAN1 yaw及支线、仅留四个
  底盘C620后回复“已接好”。Flash仍匹配ad77f9f3/c2f2d631版。两次tick185251→201459，
  CAN1与CAN2 ESR均0，CAN1故障/恢复604/604固定、TX失败3320固定，提交29746→33624，
  RX771960→837209；CAN2故障/恢复457/457固定、TX失败2742固定、提交27939→31670，
  RX677510→742756。中间操作期间两路计数都增长，不能把累计数当成隔离后新增故障；
  比较连续16.2秒窗口没有新故障，最新距最后恢复约65.4秒。四轮和CAN2电机反馈持续
  更新，yaw反馈停120962，与用户物理断开一致。armed=1、零杆底盘与摩擦轮输出0；
  云台cmd enabled=1但startup=false（yaw不新鲜），两轴受新保护停机。
  实测把持续故障缩小到yaw电机/支线/接头/终端配置所在支路；不能据此判定电机损坏，
  也不能称yaw已修复。已询问是否有可靠备用CAN线可替换曾被扯动的yaw支线做进一步
  隔离。没有改ID、波特率或重新下载。证据build/can-chassis-only-a/b.log/json及RAM、a-flash。

- 2026-09-09 最新ad77f9f3固件已人工RESET并完成两次只读实测，Flash匹配c2f2d631...BIN。
  CPU running/Fault=0，tick43977→80395，CAN1 BOFF采样均0：ESR00EF0003（TEC239/error
  passive）→00440000（TEC68）；CAN2 ESR均0。CAN1 RX210244→385458，TX提交3232→6601，
  失败176→362；CAN2 RX175091→321692，提交2821→5897、失败0。九个电机反馈均持续更新。
  关键：恢复状态机已正常调度，CAN1 faults/recoveries45/45→94/94、timeouts0，说明恢复
  软件已起作用，但仍不断产生新的CAN1总线故障，电气/初始协议根因未解决；CAN2故障0。
  CAN1最近firstESR00F80007，last_error_esr018F0023（LEC2格式错误），前次LEC1填充错误；
  不能宣称整车CAN已正常或只需再次RESET。实机PD0/PD1 MODER低4位A、AFRL低字节99
  仍正确，GPIO配置没有被其他外设覆盖。新msgcenter dispatch6200→11735、budget_hits
  6198→11734，所有hook/主循环/TX持续推进；overwritten402785→716876显示仍有严重消息
  吞吐积压，64条限额修复饿死但不等于吞吐与全部消息无丢失，机械控制性能仍未验证。
  保护实测armed=0、gimbalcmd=0、摩擦轮ramp/PID输出与底盘输出均0，遥控在线两拨杆下。
  build/can-final-a/b.log/json与RAM、a-flash.bin保留证据。已询问用户断电后暂时移除CAN1
  yaw及其支线、仅留四台底盘C620，再保持遥控安全位置上电，排除yaw支路；等待物理隔离结果。
  后续无固件改动或继续烧录计划，只读定位持续新故障。软件恢复/停机缺陷已修复并实测，
  当前硬件通信故障尚未排除，最终报告须区分这两点。

- 2026-09-09 实机验证触发补充修复（覆盖下条“只读不再烧录”的阶段性安排）：用户第一次
  RESET后c120939e固件Flash匹配，CAN1 BOFF确已清除，ESR0→03000000，CAN2 0；但
  tick100770→115010、CAN1 RX491757→561696，BSP恢复状态phase3/phase_since22097、
  两路last_tx22095均长期停住，armed=0、所有输出0。PC08010740位于消息回调链dm_on_can_frame。
  源MsgCenter_Dispatch为无上限for(;;)，持续CAN生产者使队列不空则永远不能返回到主循环
  或执行发送刷新hook；与之前反馈增长但控制/TX停更吻合。恢复请求已让硬件BOFF退出，
  状态机本身因未获调度停在phase3，不能称恢复及控制流程已全部正常。
  新增每次派发最多64条消息，之后始终调用flush hooks并返回，提供MsgCenterDiagnostics
  dispatches/events/budget_hits/overwritten。测试回调持续补充1000条消息，验证每次仅64条
  且刷新hook执行。全host suite再次PASS、两车型Debug ARM PASS、RTOS仍无链接。
  最新Infantry FLASH137208/RAM48384B，ELF ad77f9f349c861e972dd96dddc4adc5017d431711209026f99e1d27689409838，
  BIN c2f2d6315b868b2cd22eacf3f8f677b576a4d7ee79bfd9d8b71f06afa38b06e8；Sentry138592/48384B，
  ELF c65baabad3e9378bef062586560e6cb546d4fada6fee28cac13b77abf39ae33d。
  第二次下载/校验已成功（build/can-recovery-bounded-flash.log），run_after=false。已解释
  新发现的调度缺陷并请用户再次RESET；等待最终实机只读验证。新mc_diagnostics2000B514，
  recovery2000B524，armed2000B55E、healthy2000B55F/560、abortpending2000B564，其余应用
  地址不变；work/read_can_recovery_snapshot.py已改为仅对应ad77f9f3 ELF。
  第一版实测证据build/can-recovery-live-a/b.log/json及RAM、a-flash仍保留，勿用新版解码器重解。

- 2026-09-08 用户要求修复持续CAN Bus-Off：已实现并下载校验，等待人工RESET实测。
  bsp/can非阻塞恢复状态机由CmdController_Task每次调用：检测BOFF锁定两路输出并取消
  两路硬件邮箱，确认取消完成后INRQ置位等待INAK，再清INRQ等待BOFF清除；超时至少
  等1秒重试。保留原ABOM关闭/波特率，不改冻结Src/Inc/Drivers/CubeMX或RTOS。
  故障/恢复统计见BspCan_GetRecovery：phase、fault/recovery/timeout及first/last ESR。
  BspCan_Write在锁定时拒绝非零报文，CAN_Manager_FlushTx清除旧软件缓冲电机值；
  两路健康至少500ms后，遥控在线、两拨杆下位且五通道±3内再持续500ms才解锁，
  解锁当次仍发布停机。startup也默认锁定。命令路由摩擦轮新增上电/遥控重连先下档条件。
  云台每次命令要求两轴反馈<=100ms且稳定100ms，失联/禁用清PID和旧目标，重连重采
  当前角度；修复先前已证实的yaw陈旧反馈近满输出缺口。摩擦轮/拨弹禁用立即清ramp/PID
  并输出0，反馈超时清PID。没有修改运行转速、PID增益、机械行程或ID。
  真实BSP硬件模型测试通过取消前禁止初始化、握手、BOFF等待、健康窗口、取消失败重试；
  真实应用回调测试通过云台陈旧反馈停机/重对齐、摩擦轮禁用立即零；遥控链集成测试
  覆盖故障后中位杆禁止解锁、回中500ms且解锁当次仍禁用，路由覆盖上电高档/重连高档
  禁止发射。全host suite PASS；两车型Debug ARM PASS、RTOS均未链接、git diff --check通过。
  Infantry FLASH137112/RAM48368B，ELF c120939ebe5781f5a4c0dabbc5f2e06af6a2ab730c6675dfd13c8146be88bc6f，
  BIN 4a92d198b10c143da8114313d81735012ce9c0709175f1408d2f287bfa9c381e。
  Sentry FLASH138496/RAM48368B，ELF61207fa1...。使用tools/firmware.py flash infantry_standard
  已下载/校验成功，保持run_after=false；SN53FF6F067187485514522487，未自动运行。
  下载前CAN1 ESR仍00FB0007、CAN2 0，日志build/can-recovery-before/flash/host/arm/sentry.log。
  已请用户在机械区域安全、两拨杆下且杆回中时人工RESET，后续仅只读，不继续烧录。
  本次RAM布局改变，旧read_yaw_chassis_snapshot.py不能再用；新work/read_can_recovery_snapshot.py
  对应本ELF，须先验证Flash哈希。新motorcontexts20009AD8，BSP recovery2000B514，
  armed2000B54E，s_input仍20006BCC，shootctrl20006D44、gimbalcmd20006F00、chassisctrl20006FF8。
  实际电气根因和实机恢复效果尚待RESET后读取，不能把编译/下载成功称为CAN已恢复。

- 2026-09-08 用户报告“刚才有一段时间疯转”，补充为摩擦轮、发生在重新烧录后，
  不得沿用yaw超时旧目标原因直接解释。初始两次探针不可见；用户接回后新SN为
  53FF6F067187485514522487、FW V2J38S7。本轮HOTPLUG Flash仍匹配73a319c2...BIN，
  CPU running/Fault=0；仅只读，未复位/下载/写寄存器/启动电机，已提示先切断电机动力。
  两次tick224689→246079，CAN1 ESR01FB0017→03FB0027（BOFF保持，LEC先填充错误1、
  后格式错误2），RX1078162→1181343；CAN2 ESR0，RX899187→985295，九台反馈均更新。
  两路TX统计固定：CAN1提交成功94/失败319，CAN2成功412/失败0，last_tx_time155218。
  遥控两拨杆均下[2,2]、online=1、零杆；shoot s_last_cmd(0,0)、s_ctrl.enabled=0，
  摩擦轮ramp目标/PID输出/四槽应用输出全0；CAN2 mailbox0 ID200数据全0、无TXRQ。
  摩擦轮反馈RPM首次0/0、后次-3/0，当前并非高速转动。s_ctrl地址20006D3C、shootcmd
  20006CE4，与ELF尺寸444B/2B相符。日志build/friction-incident-a/b.log/json及RAM、a-flash。
  源码确认启动风险：application/cmd/command_router.c:95直接按s[0]电平使能，中/上档
  friction_enabled=true，上档feed_enabled=true；无上电先下档再主动解锁条件。
  shooter_controller.c:110/111使能时直接目标-7500/+7500RPM，.h:19/20定义7500及每次
  命令回调增加500RPM（15次即可到满目标，非按dt限加速度）。可解释上电/重启时拨杆
  已在中/上档即自动起转，但事故时拨杆未确认，不能认定该次根因或测得过7500RPM。
  用户回答不确定事故时发射拨杆位置；当前下档不能证明当时也下档。摩擦轮配置CAN2，不能直接
  归因CAN1 Bus-Off。后续安全修复应增加启动/重连回下档解锁和停机清状态；本轮未改代码。

- 2026-09-08 Bus-Off原因核查：新只读build/can-busoff-cause.log再次CAN1 ESR00FB0007，
  CAN2 ESR0；CAN1三个邮箱TIR均5FE00001（2FF且TXRQ=1），CPU运行。源Src/can.c:48
  AutoBusOff=DISABLE，实机MCR00010010的ABOM=0；BSP仅启用RX FIFO0通知，无Bus-Off
  错误回调/运行期恢复流程。可确认恢复机制缺失会使故障保持，不能由此判定初始触发原因。
  LEC目前0不保留初始ACK/位/填充错误，不能断言是电机断线或重复ID造成。特别注意tx_ok
  来自HAL_CAN_AddTxMessage成功（入邮箱），不是总线发送完成/ACK计数，先前“发送成功”
  用语应更正为“提交成功”。ST RM0090说明Bus-Off由TEC超过255触发、ABOM关闭需软件
  请求恢复；其他同类ST bxCAN参考手册称Bus-Off不能收发，当前BOFF置位同时RX与反馈
  持续增长存在待解释的不一致，不应把“Bus-Off仍正常接收”当作已证实的正常硬件行为。
  本次未复位、改寄存器、下载或启用自动恢复；初始错误需在故障发生时捕获LEC/TEC/REC。

- 2026-09-08 紧接上轮的CAN再检测：Flash仍匹配73a319c2...，CPU running/Fault=0。
  本轮tick88549→104442，CAN1 ESR两次00FB0007，Bus-Off仍置位；RX425462→502106。
  CAN2 ESR0，RX354508→418488。九个逻辑电机反馈时间均持续更新，左前ID3已恢复，
  四轮feedback_seen全1，不能继续称ID3离线或四轮缺帧联锁正在触发。遥控在线且零杆，
  底盘enabled=false、四输出0；pitch3993、云台startup/enabled均true。
  新异常：两路TX统计在本轮约15.9秒期间均不变，CAN1成功86/失败14，CAN2成功100/失败0，
  两路last_tx_time均停48548ms。CAN2接收与错误状态正常，但本轮不能称发送正常增长；
  TX停更具体原因未在本次状态读取中定位。上轮TX失败持续增长的现象已不适用于本轮。
  仅HOTPLUG读取，无暂停/复位/下载/控制输出。build/can-status-next-a/b.log/json及RAM、
  next-a-flash.bin为本轮证据，RAM为非原子顺序采样。

- 2026-09-08 最新CAN状态复测：Flash仍匹配73a319c2...BIN，CPU running/Fault=0。
  新启动周期两次tick74932→93437，CAN1 ESR均00FC0007（BOFF/EPVF/EWGF置位，TEC252），
  TX提交成功固定86、失败4355→5695，三个发送邮箱均有TXRQ且是2FF；与此同时CAN1
  RX289826→361911仍增长，不能把Bus-Off概括为当前完全无回调，也不能把RX增长当成发送正常。
  CAN2 ESR均0、RX299046→373548、TX提交4441→5781且失败0。
  轮子ID1/2/4及yaw5反馈时间两次持续更新，ID4相对上轮已恢复；唯独左前ID3反馈仍0。
  四轮feedback_seen为[0,1,1,1]（LF/RF/RR/LR），缺ID3会触发四轮联锁；采样零杆、底盘
  enabled=false、输出全0。遥控online=1，pitch约3992、云台startup=true、enabled=true。
  需区分当前两个故障：CAN1发送Bus-Off/邮箱堵塞，以及左前ID3无反馈；具体物理根因
  未定位。用户上轮确认电调灯号正确，无重复ID证据。仅HOTPLUG读取，未复位/烧录/暂停。
  记录build/can-status-now-a/b.log/json及RAM、a-flash.bin；RAM顺序读取非原子快照。

- 2026-09-08 CAN ID 重复专项检查：infantry_standard 的9个全局motor_id、同总线RX ID、
  同总线TX ID与slot组合均无重复。CAN1轮子ID1–4反馈201–204，yaw硬件ID5反馈209、
  电压TX2FF slot0；CAN2摩擦轮/拨弹反馈201/202/203，pitch硬件ID4反馈208。
  分隔的两条总线复用201–203不冲突；轮子共用TX200的四个独立槽位符合分组协议。
  本轮HOTPLUG读取Flash仍匹配73a319c2...BIN，CAN1/2 ESR均0；tick319858时CAN1 RX836132，
  轮子ID1/2反馈新鲜，ID3/4仍从未收到。yaw最后反馈245089，已陈旧约74.8秒；pitch3991、
  startup=true，此时remote online=0、gimbal enabled=0，PID内存残留output约+1263不能
  单凭该值认定正在发送非零电压。本轮未读取发送邮箱，先前已证实的运行期超时保护缺口
  仍未修复。记录build/can-id-duplicate-live.log/json及Flash/RAM快照。
  已查本地官方C620手册PDF第10/11/13页：正常绿灯闪数表示ID，橙灯每秒两次表示同总线
  ID重复且重复电调切断输出。询问左前/右前/右后/左后应为3/2/1/4，用户答“闪灯正确”。
  因此配置与用户核对灯号均未发现ID重复；缺少203/204反馈的根因尚未定位，不能把缺帧
  直接判成重复ID。此次只读诊断与文档记录，无修改固件、烧录、复位或控制输出注入。

- 2026-09-08 CAN1“无回调”专项新实测：当前已恢复回调，不能沿用上一轮Bus-Off结论。
  Flash仍匹配73a319c2...BIN，CPU正常。两次tick63782→90678，CAN1 rx_frames188346→
  267704，约2951帧/s，ESR均0、TX提交失败0；ID1/2/yaw5反馈时间持续更新，ID3/4仍0。
  实机IER=2（FIFO0消息中断使能）；NVIC ISER0=00101040含IRQ20使能，VTOR08000000、
  向量08000090=0800C441匹配当前ELF CAN1_RX0_IRQHandler(0800C440 Thumb)。GPIOD
  MODER低4位A、AFRL低字节99正确，CAN时钟开启；滤波bank0/14启用，bank0掩码全0。
  首次两路FIFO0=1B（3帧/满/溢出），NVIC有pending；后次FIFO=0且接收数增长，说明
  确有处理而非中断没有进入。曾有积压/溢出，单次寄存器不能归因为持续回调阻塞。
  当前链路HAL IRQ→HAL_CAN_RxFifo0MsgPendingCallback→CAN_Manager_GlobalCallback→
  CAN_Manager_ProcessCallback已由计数与反馈增长证明运行；缺的是底盘ID3/4对应的反馈。
  底盘四输出仍0；yaw已在线但pitch4773/4774超过1000–4000启动范围，startup=false，
  CAN1实际200/2FF数据全0。此轮只HOTPLUG读取，无halt/reset/下载/改控制代码。
  记录build/can1-irq-audit-1/2.log、can1-irq-audit-1/2.json及对应Flash/RAM快照。

- 2026-09-08 用户接回探针后当前CAN状态再次变化：SN0673FF323447523043172630可用，
  Flash134640B读回仍匹配最新BIN 73a319c2...；CPU running/Fault=0。第一次tick47871，
  CAN1 ESR=FFF80017（Bus-Off，LEC1/stuff error）、RX=0、TX提交成功42/失败14439。
  CAN2 ESR=0/RX190779，遥控在线。底盘1–4及yaw5反馈时间/初始化角标志全0，四底盘
  电流及yaw输出0，startup=false，pitch4773超过启动限4000。上轮ID1/2/yaw在线已不代表
  当前状态，本次CAN1完全无接收；不能继续只归因ID3/4或pitch启动门。
  第二次读取探针短暂消失未生成快照，第三次恢复后tick28091低于前次，期间发生重启，
  非本agent复位。此次CAN1仍FFF80017/RX0/成功49/失败6969，五台反馈仍0；CAN2 ESR0/
  RX111151正常。不能把两个启动周期计数相减算速率。MCR00010010的ABOM未开，
  Src/can.c AutoBusOff=DISABLE，Bus-Off不会自动恢复，重启后再次进入错误说明须定位
  CAN1主干/供电/接线等初始故障；具体故障部件未查明。CAN1恢复后pitch4773仍会阻塞
  云台启动，且先前发现的yaw运行期反馈超时保护缺口尚未修复。
  本次只有HOTPLUG读取，无halt/复位/下载/控制输出注入。有效记录build/reconnected-can-1/3.log、
  reconnected-can-1/3.json及RAM；reconnected-can-2.log为No debug probe detected失败记录。

- 2026-09-08 再次要求检查CAN回调：本次CubeProgrammer枚举为空，HOTPLUG返回
  No debug probe detected，未产生新的Flash/RAM读取数据（build/can-callback-latest-1.log）。
  已请求接回ST-Link。当前只能引用上一轮ID1/2/yaw在线、ID3/4缺帧、pitch4772导致
  两轴启动阻塞的历史实测；不能称这些是本次实时状态。源码复查确认CAN回调分发至
  电机反馈主题、底盘四轮新鲜度门和云台pitch启动行程门仍在，本次未改代码或烧录。

- 2026-09-08 最新在线复查：新启动tick61417→79731ms，Flash读回仍匹配981947a1 ELF
  对应BIN，CPU运行且Fault寄存器0。CAN1 ESR两次均0，RX181827→236372。
  底盘ID1/2与yaw ID5反馈时间从61414更新至79959/79963/79967ms（顺序RAM读取晚于
  tick采样约0.24s，属非原子快照），确认三台持续在线；ID3/4反馈时间及seen仍全0。
  遥控在线、采样零杆，底盘四输出0。yaw已在线但startup=false，因为pitch角度4772
  超过配置启动范围1000–4000，故两轴控制受启动门阻塞；CAN1 200与2FF采样数据全0、
  yaw内环输出0。上轮掉线近满电压不代表此次新启动状态；超时保护缺陷仍未修复。
  本次仅HOTPLUG读取，无复位/暂停/下载/控制输出注入。日志build/online-recheck-1/2.log、
  online-recheck-1/2.json及对应RAM快照。

- 用户补充yaw开机大幅转动把CAN线扯掉：物理断线是用户报告那次运动的结果，不能将
  后续陈旧反馈高输出倒置为最初开机转动的已证实原因。已抓到的后续高电压可严格解释：
  遥控在线时云台enabled=true，松杆保持旧目标；断线未清目标/PID/启动锁存，目标7984.40
  与最后angle537的环绕误差-744.60刻度，外环/应用限制后目标-500RPM，旧反馈-55RPM，
  内环误差-445RPM，P项52.5*(-445)=-23362.5，加积分约-1273得到约-24635电压刻度。
  初次96s采样yaw从未有反馈、startup=false，与用户所述转动的确切启动阶段尚未对齐，
  不可凭后续快照宣称证明开机猛转。底盘不是全离线：右后ID1/右前ID2有反馈；左前3/
  左后4未见反馈。若两左轮位于被扯掉接头后的支路，共同断点可解释，但实际拓扑未知，
  也未排除实体C620 ID设置或供电；代码注册映射与CAN1接收全通滤波无ID3/4专门禁用。

- 2026-09-08 实机yaw/底盘不能动及突然转动诊断（覆盖上轮无探针状态）：当前换了
  ST-Link SN0673FF323447523043172630，旧保存SN066EFF...会报Serial number not found。
  本次显式选择新探针，只HOTPLUG读取，无halt/reset/download/输出注入。
  Flash读回134640B与最新yaw电压+全向轮BIN逐字节哈希一致（BIN SHA256
  73a319c216d11e8c33914c79d71d657994aecc0403ced183e5d96bf6a784fc97，ELF仍981947a1...）。
  CPU运行且CFSR/HFSR=0。三次同一启动tick96323→178642→230398ms，遥控帧
  6845→12743→16440，online=true；采样时五通道均0，普通/跟随档，无小陀螺或视觉。
  底盘config_valid=true、几何ID顺序3/2/1/4正确，但ID3和4在驱动及控制器中的反馈
  时间持续0、feedback_seen=false，ID1/2有更新。故零杆时enabled=false符合路由，
  即使有杆量也会被全向轮四轮反馈联锁置零，CAN1 TX200实际8字节全0。不能将它说成
  另缺C620使能帧；尚未定位ID3/4是实体ID、供电、接线还是其他丢帧原因。
  CAN1 40次离散FIFO/回调lastID采样仅201/202/209（各采样独立非总线全量抓包），
  CAN1 ESR00170000→00050000→0，均非Bus-Off；CAN2 ESR=0，不能沿用历史CAN1全断。
  yaw状态发生变化：第一帧反馈从未收到、startup=false；第二次209在线、startup=true、
  angle7792/target7788.83/电压约-452；第三次yaw反馈时间停207588ms，而tick230398，
  已失联约22.8秒，陈旧angle537/speed-55；target7984.40，外环饱和-600RPM，应用限
  -500RPM，内环仍输出-24635。CAN1 mailbox1 TIR5FE00000=ID2FF，数据9F D2 00...，
  实际电压命令-24622，与控制器近满输出一致。底盘mailbox0 ID200仍全0。
  明确软件缺陷：gimbal启动标志永久锁存，只要求曾收到反馈；之后应用/服务/驱动均
  没有反馈超时停机，持续用旧位置计算且保留积分/目标，重新连通可能突然运动。
  已即时告知用户先切断yaw动力，避免恢复通信时冲击；上轮恢复旧电压PID增益但未补
  此保护是遗漏。当前手动/保持允许300/500RPM且无输出软启动，可能放大突变；未抓到
  用户所述开机瞬间，不能宣称开机大转已被精确复现/证明根因。首次采样yaw从未见帧，
  用户报告动作的具体启动/上电时间仍待核实，已问排查期间是否动过yaw接线/供电。
  后续应先加云台实时反馈新鲜度门、超时零输出并清PID/旧目标、重连稳定后重新锁存
  当前角度，再以受限速度/输出验证；不能删除底盘四轮联锁或盲目加大PID。
  原始记录build/yaw-chassis-read-1/2/3.log、yaw-chassis-ram-1/2/3.bin、
  yaw-chassis-can1-rx-samples.log、yaw-chassis-snapshot-3.json。本次仅诊断与记录，
  未改固件/测试注入；RAM为运行中顺序读取非原子快照，细小时差不可用来断言超时。

- 2026-09-08 yaw再次改回电压：用户确认已重新调整GM6020电机固件，现在与pitch一样
  使用电压指令。当前工程仍配置电流2FE，构成模式不匹配；本次将infantry yaw改为
  CAN1硬件ID5、RX209、TX2FF/slot0、GM6020_COMMAND_VOLTAGE、原始限幅25000。
  恢复8911563电压PID增益：外环1.5/0.03/0、上限600RPM/积分450；内环52.5/0.12/1.8、
  积分6000，输出由旧30000修正为协议25000。保留yaw位置→速度串级与启动位置锁存，
  不复制pitch重力补偿/行程到yaw；新电机固件下速度环响应待实测。pitch及四轮几何/
  CAN ID、遥控与冻结底层/RTOS均未改。不要再沿用上方历史yaw电流模式为当前配置。
  更新真实DJI适配器+CAN聚合测试：两轴电压分组、正负限幅、零输出/旧入口、错误模式/
  槽位/限幅拒绝；独立夹具保留2FE/1FE电流和混合模式覆盖。完整host测试通过；两车型
  ARM Debug/ELF检查通过，无RTOS入口。infantry FLASH/RAM=134640/48280B，ELF SHA256=
  981947a17dcc9a05c1f6c307881e022e199fc3e0e40c98825a42ad7548a5557e；sentry保持原SHA。
  日志build/yaw-voltage-host-tests.log、yaw-voltage-build.log、yaw-voltage-sentry-build.log；
  模式说明docs/gm6020-control-modes.md已更新。只读连接提示No debug probe detected，
  记录build/yaw-voltage-before.log，已请求接回ST-Link。本次尚未下载/复位/注入输出或
  修改电机内部设置；电机电压模式来自用户确认，非从CAN反馈读到。后续烧录会包含
  已完成的全向轮配置，仍遵守校验后不自动复位运行，实车成功不得提前宣称。

- 2026-09-08 底盘CAN ID专项复查：当前源码左前3/RX203/slot2、右前2/RX202/slot1、
  右后1/RX201/slot0、左后4/RX204/slot3，均CAN1、TX200，符合C620 ID1–4协议。
  几何轮序3/2/1/4经逻辑ID映射，发送仍按slot打包，不会随数组重排而错发；同总线
  yaw RX209/TX2FE无冲突，CAN2相同数字ID为独立总线。当前配置测试重新严格编译/
  执行通过；此次未修改控制代码、未烧录、未读取电调实际ID或在线状态。源码配置正确
  不代表已确认板上固件/实体电调设置一致。

- 2026-09-08 四轮全向底盘：用户确认 CAN1 四台 M3508/C620 ID1–4、遥控已通；
  俯视车头朝前，左前3、右前2、右后1、左后4。HEAD d7f91eb 已加入硬件手册。
  本次将 omni 占位改为参数化逆运动学：车体 +x前/+y左/+yaw逆时针，轮心位置与驱动
  单位向量投影，再按半径/减速比换算转子 RPM；输入平移矢量限幅与四轮等比例降速，
  拒绝缺参/NaN/非单位向量/重复ID/秩不足布局，失败清空输出。
  OmniChassisConfig 新增每轮 motor_id/x/y/驱动向量/半径/减速比与配置开关、速度上限；
  ChassisKinematicsInput 增加几何指针，RobotConfig 增加 omni 指针。步兵选择 OMNI，
  用户随后确认X形±45°、左右与前后轮中心距均54cm、轮半径7cm、M3508配套P19。
  已按3/2/1/4轮位填入(±0.27m,±0.27m)、驱动向量(1,±1)/sqrt(2)、半径0.07m及
  精确减速比3591/187，并将configured=1。减速比来自DJI官方P19手册v1.0(2017.08)
  PDF第7页，来源链接见docs/omni-chassis.md；不再有未填几何字段。
  旧 motor.direction ID1–4=-1/+1/+1/-1 沿用，实际安装正反向待首次架空核对；初调上限
  0.30m/s、0.60rad/s。C620手册PDF14–17页确认0x200分组、转子RPM及±16384=±20A；
  仓库M3508 v1.0(2025.10)为裸电机，因此另查官方P19手册，没有混用裸电机转速与轮速。
  控制器按几何ID找真实电机，不再把配置数组顺序当轮位；RPM上限同时尊重四电机限幅。
  disabled/无效配置直接零电流并PID_Reset；新增反馈已见标志（tick0也有效），全向轮
  任一轮>100ms无反馈则四轮零电流；恢复后重新计算。错误映射保留真实底盘ID停机，
  不给其他角色发零。云台/遥控接收/底层/RTOS未改，遥控普通档映射保持原样；原跟随/
  小陀螺坐标转换未校准，首测用普通档。哨兵控制实现保持，公共结构随构建重新编译。
  验证：完整host通过，新增运动学基向量/组合/限幅/半径减速/轮序/非法参数测试；
  新真实消息→控制器→PID→CAN1聚合测试确认0x200 ID1–4字节顺序、方向、禁用/缺反馈/
  单轮失联/无效映射零输出（电机服务转发、BSP、时间/日志为桩）。加入实际生产几何
  回归：0.30m/s前进约555.72转子RPM，0.60rad/s自转约600.17RPM及符号/CAN槽位。
  两车型Debug构建/
  ELF检查通过，无RTOS入口。infantry FLASH/RAM=134640/48280B，SHA256=
  9f306c0ef74cc7b2de641b53f17879927830195c4af29ad1079ee1fd3556ca4e；
  sentry=136024/48280B。日志build/omni-host-tests.log、omni-infantry-build.log、
  omni-sentry-build.log。说明docs/omni-chassis.md。本次完成编写与编译验证，未烧录、
  未注入电机输出，不能宣称已实车运动；后续仍需核对轮子正反向与实车速度环表现。

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
| 全向轮 | 参数化逆运动学和控制器已实现 | 步兵已选择并填入实际几何 | 3/2/1/4，X形±45°，轴/轮距54cm，轮半径7cm，P19；方向/速度环待实车验证 |
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
- 全向轮CAN1 ID/轮位与几何已确认，实际正反转方向与速度环效果待实车验证；通用舵轮和Jetson仍缺硬件输入。

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
