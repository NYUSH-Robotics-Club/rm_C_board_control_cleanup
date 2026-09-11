/* Real application callbacks: stale feedback and disable must erase old motor commands. */
#include "gimbal_controller.h"
#include "shooter_controller.h"
#include "motor_driver.h"
#include "motor_service.h"
#include "message_center.h"
#include "infantry_standard.h"
#include "logger.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static uint32_t now_ms;
static MotorContext_t motors[16];
static int16_t outputs[16];
static float setpoints[16];
static bool can_armed = true;
bool BspCan_OutputsArmed(void) { return can_armed; }
void ShooterApp_Init(void);
uint32_t BspTime_NowMs(void){return now_ms;}
uint32_t BspTime_NowUs(void){return now_ms*1000U;}
void BspTime_DelayMs(uint32_t ms){now_ms+=ms;}
void Logger_Log(LogTag_t t,LogLevel_t l,const char *f,...){(void)t;(void)l;(void)f;}
void Logger_CSV(LogTag_t t,const char *f,...){(void)t;(void)f;}
void USB_CDC_Printf(const char *f,...){(void)f;}
const MotorConfig_t *MotorService_GetConfig(uint8_t id){
 const RobotConfig_t *r=&g_robot_config_infantry_standard;
 for(uint8_t i=0;i<r->total_motor_count;i++)if(r->motor_configs[i].motor_id==id)return &r->motor_configs[i];
 return NULL;
}
uint8_t MotorService_FindByRole(MotorRole_e role,uint8_t *ids,uint8_t cap){
 uint8_t count=0;
 for(uint8_t id=1;id<10&&count<cap;id++)if(MotorService_GetConfig(id)->role==role)ids[count++]=id;
 return count;
}
MotorContext_t *MotorDriver_GetContext(uint8_t id){return id<16?&motors[id]:NULL;}
/* 使用当前车型的 yaw 电流刻度上限，避免调参后仍只验证旧的限流值。 */
int16_t MotorDriver_GetCommandLimit(uint8_t id){
 return id==5?MotorService_GetConfig(id)->protocol.dji.command_limit:25000;
}
RobotStatus MotorService_CommandCurrent(uint8_t id,int16_t value){assert(id<16);outputs[id]=value;return ROBOT_STATUS_OK;}
RobotStatus MotorService_CommandConfigured(uint8_t id,float target,int16_t value){setpoints[id]=target;return MotorService_CommandCurrent(id,value);}
/* 三台反馈联锁走真实消息回调；依次模拟每台断线和首帧缺失。 */
static void shooter_feedback(uint8_t id, uint32_t tick) {
 MotorFeedbackEvent feedback = {.id=id, .speed=1000, .tick_ms=tick};
 assert(MsgCenter_Publish(TOPIC_MOTOR_FEEDBACK, &feedback, sizeof(feedback)) == 0);
}
static void shooter_command(void) {
 ShootCmd command = {.friction_enabled=true, .feed_enabled=true};
 assert(MsgCenter_Publish(TOPIC_SHOOT_CMD, &command, sizeof(command)) == 0);
 MsgCenter_Dispatch();
}
static void assert_shooter_stopped(void) {
 const ShooterController *controller = ShooterApp_GetController();
 assert(outputs[6] == 0 && outputs[7] == 0 && outputs[9] == 0);
 assert(controller->ramped_turntable == 0 && controller->ramped_shooter1 == 0 && controller->ramped_shooter2 == 0);
 assert(controller->turntable_target == 0 && controller->shooter1_target == 0 && controller->shooter2_target == 0);
 const PID_Controller *pids[] = {&controller->turntable_pid, &controller->shooter1_pid, &controller->shooter2_pid};
 for (unsigned i=0; i<3; ++i) {
  assert(pids[i]->integral == 0 && pids[i]->iout == 0 && pids[i]->dout == 0);
  assert(pids[i]->output == 0 && pids[i]->last_output == 0 && pids[i]->last_dout == 0);
  assert(pids[i]->error[0] == 0 && pids[i]->error[1] == 0 && pids[i]->error[2] == 0);
 }
 assert(!ShooterController_IsRunning(controller));
}
static void test_shooter_interlock(void) {
 MsgEvent queue[32];
 const uint8_t ids[] = {6,7,9};
 can_armed = true;
 for (unsigned missing=0; missing<3; ++missing) {
  now_ms=0;
  MsgCenter_Init(queue,32); ShooterApp_Init();
  for (unsigned i=0; i<3; ++i) if(i!=missing) shooter_feedback(ids[i], now_ms);
  shooter_command(); assert_shooter_stopped();
  assert(ShooterApp_GetController()->feedback_fault);
  /* 时间戳0也是真实首帧；补齐后可从零启动。 */
  shooter_feedback(ids[missing], now_ms); shooter_command();
  assert(setpoints[6]==-500 && setpoints[7]==500 && setpoints[9]==500);
  assert(!ShooterApp_GetController()->feedback_fault);
  now_ms=1000;
  for (unsigned i=0; i<3; ++i) shooter_feedback(ids[i], now_ms);
  for (unsigned i=0; i<15; ++i) shooter_command();
  assert(setpoints[6]==-7500 && setpoints[7]==7500 && setpoints[9]==1000);
  now_ms=1100;
  for (unsigned i=0; i<3; ++i) if(i!=missing) shooter_feedback(ids[i], now_ms);
  shooter_command(); assert(!ShooterApp_GetController()->feedback_fault);
  now_ms=1101; shooter_command(); assert_shooter_stopped();
  assert(ShooterApp_GetController()->feedback_fault);
  for (unsigned i=0; i<20; ++i) shooter_command();
  assert_shooter_stopped();
  now_ms=1102;
  for (unsigned i=0; i<3; ++i) shooter_feedback(ids[i], now_ms);
  shooter_command();
  assert(setpoints[6]==-500 && setpoints[7]==500 && setpoints[9]==500);
  can_armed=false; shooter_command(); assert_shooter_stopped(); can_armed=true;
 }
 /* uint32毫秒回绕不能误判新鲜反馈。 */
 now_ms=UINT32_MAX-50U;
 for(unsigned i=0; i<3; ++i) shooter_feedback(ids[i],now_ms);
 shooter_command(); now_ms=20U; shooter_command();
 assert(!ShooterApp_GetController()->feedback_fault);
 now_ms=50U; shooter_command(); assert_shooter_stopped();
}

static void gimbal_step(uint32_t now,bool enabled,bool fresh){
 now_ms=now;
 if(fresh)motors[5].last_feedback_time=motors[8].last_feedback_time=now;
 GimbalCmd cmd={.enabled=enabled};
 assert(MsgCenter_Publish(TOPIC_GIMBAL_CMD,&cmd,sizeof(cmd))==0);
 /* 普通队列在控制命令发布后溢出，真实云台回调仍须本轮执行并保持失联保护。 */
 for (unsigned i=0;i<256U;i++) assert(MsgCenter_Publish(TOPIC_CAN_RX,&i,sizeof(i))==0);
 MsgCenter_Dispatch();
 assert(MsgCenter_GetDiagnostics()->overwritten_by_topic[TOPIC_GIMBAL_CMD]==0);
}

static void feedforward_step(GimbalCmd command) {
 now_ms += 4U;
 motors[5].last_feedback_time = motors[8].last_feedback_time = now_ms;
 assert(MsgCenter_Publish(TOPIC_GIMBAL_CMD, &command, sizeof(command)) == 0);
 MsgCenter_Dispatch();
}

/* Exercise actual controller callbacks with synthetic gains; production gains stay intact. */
static void test_gimbal_feedforward(MotorConfig_t *yaw_config, YawControlConfig *yaw_control) {
 MotorContext_t *yaw = &motors[5], *pitch = &motors[8];
 const MotorConfig_t *original_pitch = pitch->config;
 MotorConfig_t pitch_config = *original_pitch;
 assert(yaw_config->feedforward.output_max == 0 && pitch_config.feedforward.output_max == 0);
 pitch->config = &pitch_config;
 pitch_config.limits.gm6020.gravity_compensation = 0;
 /* 前馈测试锁存实测位置，独立验证负数初始目标的兼容路径。 */
 pitch_config.limits.gm6020.initial_angle = -1;
 yaw_config->feedforward = (GimbalFeedforwardConfig){10, 3, 100};
 pitch_config.feedforward = (GimbalFeedforwardConfig){4, -7, 100};
 yaw_control->speed_loop_only = true;
 yaw_control->manual_speed_rpm = 10;
 yaw->angle_raw = 4555; pitch->angle_raw = 1971;
 yaw->speed_rpm = pitch->speed_rpm = 0;
 PID_Init(&yaw->pid_inner, 0, 0, 0, 25000, 0);
 PID_Init(&pitch->pid_outer, 1, 0, 0, 600, 0);
 PID_Init(&pitch->pid_inner, 0, 0, 0, 25000, 0);
 pitch->pid_outer.output_lpf_rc = 0;
 gimbal_step(2200, true, true);
 assert(outputs[5] == 0 && outputs[8] == 0); /* Bias cannot bypass startup wait. */
 gimbal_step(2300, true, true);
 assert(outputs[5] == 3 && outputs[8] == -7); /* Explicit bias also exists at rest. */
 GimbalCmd cmd = {.enabled = true, .yaw_rate = 1, .pitch_rate = -0.1f};
 PID_Reset(&pitch->pid_outer);
 feedforward_step(cmd);
 assert(yaw->pid_inner.target == 5 && outputs[5] == 53);
 assert(pitch->pid_inner.target == 6 && outputs[8] == 17); /* direction already in speed target */
 assert(yaw->pid_outer.output == 0); /* Feedforward does not re-enable outer loop. */
 cmd.yaw_rate = -1; cmd.pitch_rate = 0.1f;
 pitch->angle_target = pitch->angle_raw; PID_Reset(&pitch->pid_outer);
 feedforward_step(cmd);
 assert(outputs[5] == -47 && outputs[8] == -31);
 yaw_config->feedforward.output_max = 20;
 pitch_config.feedforward.output_max = 8;
 feedforward_step(cmd);
 assert(outputs[5] == -20 && outputs[8] == -8);

 /* The feedforward uses the limited reference, then adds to the real PID output. */
 yaw_config->feedforward.output_max = 100;
 yaw_control->manual_speed_rpm = 3;
 cmd.yaw_rate = 1; cmd.pitch_rate = 0;
 feedforward_step(cmd);
 assert(yaw->pid_inner.target == 3 && outputs[5] == 33);
 PID_Init(&yaw->pid_inner, 2, 0, 0, 25000, 0);
 yaw->pid_inner.output_lpf_rc = 0;
 feedforward_step(cmd);
 assert(yaw->pid_inner.output == 6 && outputs[5] == 39);
 yaw_control->manual_speed_rpm = 10;
 PID_Init(&yaw->pid_inner, 0, 0, 0, 25000, 0);
 yaw_config->feedforward = (GimbalFeedforwardConfig){0, 50000, 60000};
 pitch_config.feedforward = yaw_config->feedforward;
 feedforward_step(cmd);
 assert(outputs[5] == MotorDriver_GetCommandLimit(5) && outputs[8] == 25000);
 yaw_config->feedforward.bias = pitch_config.feedforward.bias = -50000;
 feedforward_step(cmd);
 assert(outputs[5] == -MotorDriver_GetCommandLimit(5) && outputs[8] == -25000);

 /* Disabling the new term preserves the existing gravity compensation. */
 yaw_config->feedforward = (GimbalFeedforwardConfig){NAN, NAN, 0};
 pitch_config.feedforward = yaw_config->feedforward;
 pitch_config.limits.gm6020.gravity_compensation = 50;
 pitch->angle_raw = 2048;
 feedforward_step(cmd);
 assert(outputs[5] == 0 && outputs[8] == -50);
 pitch_config.limits.gm6020.gravity_compensation = 0;
 yaw_config->feedforward = (GimbalFeedforwardConfig){0, 12, 100};
 pitch_config.feedforward = (GimbalFeedforwardConfig){0, -15, 100};
 gimbal_step(now_ms + 4, false, true);
 assert(outputs[5] == 0 && outputs[8] == 0);
 gimbal_step(now_ms + 4, true, true);
 gimbal_step(now_ms + 100, true, true);
 assert(outputs[5] == 12 && outputs[8] == -15);
 gimbal_step(now_ms + 101, true, false);
 assert(outputs[5] == 0 && outputs[8] == 0);
 gimbal_step(now_ms + 4, true, true);
 gimbal_step(now_ms + 100, true, true);
 assert(outputs[5] == 12 && outputs[8] == -15);

 /* Each invalid enabled term stops BOTH axes and clears their PID histories. */
 for (unsigned i = 0; i < 4; ++i) {
   if (i == 0) yaw_config->feedforward.velocity_gain = NAN;
   if (i == 1) pitch_config.feedforward.output_max = -1;
   if (i == 2) pitch_config.feedforward.bias = INFINITY;
   if (i == 3) yaw_config->feedforward.velocity_gain = FLT_MAX;
   feedforward_step(cmd);
   assert(outputs[5] == 0 && outputs[8] == 0);
   assert(yaw->pid_inner.output == 0 && pitch->pid_inner.output == 0);
   yaw_config->feedforward = (GimbalFeedforwardConfig){0, 12, 100};
   pitch_config.feedforward = (GimbalFeedforwardConfig){0, -15, 100};
   gimbal_step(now_ms + 4, true, true);
   assert(outputs[5] == 0 && outputs[8] == 0);
   gimbal_step(now_ms + 100, true, true);
   assert(outputs[5] == 12 && outputs[8] == -15);
 }
 pitch->config = original_pitch;
 yaw_config->feedforward = (GimbalFeedforwardConfig){0};
}
int main(void){
 MsgEvent queue[32];
 MsgCenter_Init(queue,32);
 assert(MsgCenter_UseLatest(TOPIC_GIMBAL_CMD,MC_LATEST_CONTROL)==0);
 assert(MsgCenter_UseLatest(TOPIC_SHOOT_CMD,MC_LATEST_CONTROL)==0);
 for(uint8_t id=5;id<=8;id+=3){
  MotorContext_t *m=&motors[id];m->config=MotorService_GetConfig(id);m->initialized=true;
  m->type=MOTOR_TYPE_GM6020;m->role=m->config->role;m->angle_raw=id==5?5000:1900;
  PID_Init(&m->pid_outer,1,0,0,600,100);
  PID_Init(&m->pid_inner,1,0,0,25000,100);
 }
 /* 原有回归验证位置环路径；实际步兵配置仍默认为速度调试。 */
 MotorConfig_t yaw_config=*motors[5].config;
 YawControlConfig yaw_control=*yaw_config.yaw_control;
 assert(yaw_control.speed_loop_only);
 yaw_control.speed_loop_only=false;
 yaw_control.manual_rate_deg_s=30; /* 固定测试输入，不依赖实车推杆灵敏度调参。 */
 yaw_config.yaw_control=&yaw_control;
 motors[5].config=&yaw_config;
 GimbalApp_Init(); ShooterApp_Init();
 gimbal_step(100,true,true);assert(outputs[5]==0&&outputs[8]==0);
 gimbal_step(200,true,true);assert(motors[5].angle_target==5000);
 assert(motors[8].angle_target==1971); /* 初始目标不能被1900的反馈覆盖。 */
 motors[5].angle_raw=5100;
 for (unsigned n=0;n<PID_YAW_OUTER_DIVIDER;n++) gimbal_step(210+n,true,true);
 assert(outputs[5]!=0);
 motors[5].pid_inner.iout=999;
 gimbal_step(315,true,false);assert(outputs[5]==0&&outputs[8]==0&&motors[5].pid_inner.iout==0);
 motors[5].angle_raw=1000;gimbal_step(320,true,true);assert(outputs[5]==0);
 gimbal_step(420,true,true);assert(motors[5].angle_target==1000&&outputs[5]==0);
 motors[5].pid_inner.iout=999;gimbal_step(421,false,true);
 assert(outputs[5]==0&&outputs[8]==0&&motors[5].pid_inner.iout==0);
 now_ms=500;
 for(uint8_t id=6;id<=9;id++){
  MotorFeedbackEvent f={.id=id,.speed=1000,.tick_ms=now_ms};
  MsgCenter_Publish(TOPIC_MOTOR_FEEDBACK,&f,sizeof(f));
 }
 MsgCenter_Dispatch();
 ShootCmd shoot={.friction_enabled=true,.feed_enabled=true};
 MsgCenter_Publish(TOPIC_SHOOT_CMD,&shoot,sizeof(shoot));MsgCenter_Dispatch();
 assert(setpoints[6]!=0&&setpoints[7]!=0);
 shoot=(ShootCmd){0};
 MsgCenter_Publish(TOPIC_SHOOT_CMD,&shoot,sizeof(shoot));MsgCenter_Dispatch();
 assert(outputs[6]<0&&outputs[7]<0&&outputs[9]==0); /* Fresh positive RPM requires braking at target zero. */
 assert(setpoints[6]==0&&setpoints[7]==0&&setpoints[9]==0);
 shoot.friction_enabled=true;
 for(unsigned i=0;i<15;i++) {
  MsgCenter_Publish(TOPIC_SHOOT_CMD,&shoot,sizeof(shoot));MsgCenter_Dispatch();
 }
 assert(setpoints[6]==-7500&&setpoints[7]==7500);
 shoot.friction_enabled=false;
 MsgCenter_Publish(TOPIC_SHOOT_CMD,&shoot,sizeof(shoot));MsgCenter_Dispatch();
 assert(setpoints[6]==-7000&&setpoints[7]==7000); /* Restore the original 500 RPM per callback ramp. */
 can_armed=false;
 MsgCenter_Publish(TOPIC_SHOOT_CMD,&shoot,sizeof(shoot));MsgCenter_Dispatch();
 assert(outputs[6]==0&&outputs[7]==0);
 assert(ShooterApp_GetController()->ramped_shooter1==0&&ShooterApp_GetController()->ramped_shooter2==0);
 can_armed=true;now_ms=601;
 MsgCenter_Publish(TOPIC_SHOOT_CMD,&shoot,sizeof(shoot));MsgCenter_Dispatch();
 assert(outputs[6]==0&&outputs[7]==0); /* Stale feedback must never be used to brake. */
 /* 固定测试增益，验证双环出力和失联归零，不把用户调参当作稳定性测试模型。 */
 MotorContext_t *yaw=&motors[5];
 PID_Init(&yaw->pid_outer,1,0,0,600,0);
 PID_Init(&yaw->pid_inner,1,0,0,25000,0);
 gimbal_step(700,true,true);gimbal_step(800,true,true);
 yaw->angle_raw+=20;
 for (unsigned n=0;n<2U*PID_YAW_OUTER_DIVIDER;n++) gimbal_step(810+n,true,true);
 assert(outputs[5]<0&&outputs[5]>=-MotorDriver_GetCommandLimit(5));
 yaw->angle_raw-=40;
 for (unsigned n=0;n<2U*PID_YAW_OUTER_DIVIDER;n++) gimbal_step(820+n,true,true);
 assert(outputs[5]>0&&outputs[5]<=MotorDriver_GetCommandLimit(5));
 motors[8].last_feedback_time=930;
 gimbal_step(930,true,false);
 assert(outputs[5]==0&&outputs[8]==0&&yaw->pid_inner.iout==0);
 /* 超出任一绝对边界均禁止启动双轴；进入范围后采用1971初始目标。 */
 MotorContext_t *pitch=&motors[8];
 pitch->angle_raw=1565;
 gimbal_step(1000,true,true);assert(outputs[5]==0&&outputs[8]==0);
 gimbal_step(1100,true,true);assert(outputs[5]==0&&outputs[8]==0);
 pitch->angle_raw=2206;
 gimbal_step(1104,true,true);assert(outputs[5]==0&&outputs[8]==0);
 pitch->angle_raw=1566;
 gimbal_step(1108,true,true);assert(pitch->angle_target==1971);
 SensorData sensor={0};
 pitch->angle_target=1570;
 for (unsigned i=0;i<10;i++) (void)GimbalController_PitchControl(8,1,&sensor,true);
 assert(pitch->angle_target==1566); /* 连续抬头输入不能累积越过上止点。 */
 (void)GimbalController_PitchControl(8,-1,&sensor,true);
 assert(pitch->angle_target==1626); /* 边界仍允许反向离开。 */
 pitch->angle_raw=2205;pitch->angle_target=2200;
 for (unsigned i=0;i<10;i++) (void)GimbalController_PitchControl(8,-1,&sensor,true);
 assert(pitch->angle_target==2205);
 (void)GimbalController_PitchControl(8,1,&sensor,true);
 assert(pitch->angle_target==2145);
 gimbal_step(1201,true,false);assert(outputs[5]==0&&outputs[8]==0);
 /* 配置中的启动目标也须在范围内，恢复合法配置后可在边界启动。 */
 MotorConfig_t limited_pitch=*pitch->config;
 limited_pitch.limits.gm6020.initial_angle=2206;
 pitch->config=&limited_pitch;
 gimbal_step(1300,true,true);gimbal_step(1400,true,true);
 assert(outputs[5]==0&&outputs[8]==0);
 pitch->config=MotorService_GetConfig(8);
 gimbal_step(1410,true,true);assert(pitch->angle_target==1971);
 /* 速度调试：位置/视觉/spin不能产生额外速度目标；回中仍主动制动。 */
 gimbal_step(1420,false,true);
 yaw_control.speed_loop_only=true;
 PID_Init(&yaw->pid_inner,100,0,0,25000,0);
 gimbal_step(1430,true,true);gimbal_step(1530,true,true);
 GimbalCmd speed_cmd={.enabled=true,.yaw_rate=1.0f};
 now_ms=1534;motors[5].last_feedback_time=motors[8].last_feedback_time=now_ms;
 MsgCenter_Publish(TOPIC_GIMBAL_CMD,&speed_cmd,sizeof(speed_cmd));MsgCenter_Dispatch();
 assert(fabsf(yaw->pid_inner.target-5.0f)<0.001f&&outputs[5]>0);
 assert(yaw->pid_outer.output==0.0f&&yaw->pid_outer.update_divider==0U);
 float held=yaw->angle_target;
 yaw->angle_raw+=100;yaw->speed_rpm=10;
 gimbal_step(1538,true,true);
 assert(yaw->pid_inner.target==0.0f&&outputs[5]<0&&yaw->angle_target==held);
 speed_cmd.vision_valid=true;speed_cmd.vision_yaw_err_rad=1.0f;
 now_ms=1542;motors[5].last_feedback_time=motors[8].last_feedback_time=now_ms;
 MsgCenter_Publish(TOPIC_GIMBAL_CMD,&speed_cmd,sizeof(speed_cmd));MsgCenter_Dispatch();
 assert(yaw->pid_inner.target==0&&yaw->pid_inner.actual==10&&yaw->angle_target==held);
 speed_cmd.vision_valid=false;speed_cmd.yaw_rate_memo=1;speed_cmd.yaw_target_memo=90;
 now_ms=1546;motors[5].last_feedback_time=motors[8].last_feedback_time=now_ms;
 MsgCenter_Publish(TOPIC_GIMBAL_CMD,&speed_cmd,sizeof(speed_cmd));MsgCenter_Dispatch();
 assert(yaw->pid_inner.target==0&&yaw->pid_inner.actual==10);
 /* 最新帧虽新，但丢失超过20ms连续历史，不能猜中间圈数。 */
 gimbal_step(1570,true,true);assert(outputs[5]==0&&outputs[8]==0);
 yaw->speed_rpm=0;yaw->angle_raw=4555;
 gimbal_step(1574,true,true);gimbal_step(1674,true,true);
 assert(yaw->angle_target==4555&&yaw->pid_inner.target==0);
 /* 重新启用位置路径后，跨半圈仍追原目标，而不是改为顺向加速。 */
 gimbal_step(1678,false,true);yaw_control.speed_loop_only=false;
 yaw_control.manual_speed_rpm=10;yaw_control.vision_speed_rpm=7;yaw_control.spin_speed_rpm=8;
 gimbal_step(1682,true,true);gimbal_step(1782,true,true);
 for(unsigned k=1;k<=50;k++) {
   yaw->angle_raw=(uint16_t)((4555U+k*100U)%8192U);
   gimbal_step(1782+4*k,true,true);
 }
 assert(yaw->angle_target==4555&&yaw->pid_outer.error[0]<-4096);
 assert(yaw->pid_inner.target==-10&&outputs[5]<0);
 speed_cmd=(GimbalCmd){.enabled=true,.yaw_rate=1};
 now_ms=1986;motors[5].last_feedback_time=motors[8].last_feedback_time=now_ms;
 MsgCenter_Publish(TOPIC_GIMBAL_CMD,&speed_cmd,sizeof(speed_cmd));MsgCenter_Dispatch();
 assert(yaw->pid_inner.target==-10);
 gimbal_step(1990,true,true);assert(yaw->pid_inner.target==-10);
 /* 视觉与spin按明确模式限速，返回普通模式锁当前连续位置。 */
 speed_cmd.vision_valid=true;speed_cmd.vision_yaw_err_rad=1;
 now_ms=1994;motors[5].last_feedback_time=motors[8].last_feedback_time=now_ms;
 MsgCenter_Publish(TOPIC_GIMBAL_CMD,&speed_cmd,sizeof(speed_cmd));MsgCenter_Dispatch();
 assert(yaw->pid_inner.target==7);
 speed_cmd.vision_valid=false;speed_cmd.yaw_rate_memo=1;speed_cmd.yaw_target_memo=90;
 now_ms=1998;motors[5].last_feedback_time=motors[8].last_feedback_time=now_ms;
 MsgCenter_Publish(TOPIC_GIMBAL_CMD,&speed_cmd,sizeof(speed_cmd));MsgCenter_Dispatch();
 assert(yaw->pid_inner.target==8);
 /* spin世界航向超过半圈时仍保持所选圈数，不逐轮改走另一方向。 */
 sensor.yaw_total_angle=300;
 for(unsigned k=0;k<5;k++) {
   now_ms=2002+4*k;motors[5].last_feedback_time=motors[8].last_feedback_time=now_ms;
   MsgCenter_Publish(TOPIC_IMU_UPDATE,&sensor,sizeof(sensor));
   MsgCenter_Publish(TOPIC_GIMBAL_CMD,&speed_cmd,sizeof(speed_cmd));MsgCenter_Dispatch();
 }
 assert(yaw->pid_outer.error[0]<-4096&&yaw->pid_inner.target==-8);
 gimbal_step(2022,true,true);assert(yaw->angle_target==yaw->angle_raw&&yaw->pid_inner.target==0);
 /* 配置缺失和非有限输入都归零两轴，不沿用旧命令。 */
 yaw_config.yaw_control=NULL;gimbal_step(2026,true,true);assert(outputs[5]==0&&outputs[8]==0);
 yaw_config.yaw_control=&yaw_control;
 gimbal_step(2030,true,true);gimbal_step(2130,true,true);
 speed_cmd=(GimbalCmd){.enabled=true,.yaw_rate=NAN};
 now_ms=2134;motors[5].last_feedback_time=motors[8].last_feedback_time=now_ms;
 MsgCenter_Publish(TOPIC_GIMBAL_CMD,&speed_cmd,sizeof(speed_cmd));MsgCenter_Dispatch();
 assert(outputs[5]==0&&outputs[8]==0);
 test_gimbal_feedforward(&yaw_config, &yaw_control);
 test_shooter_interlock();
 puts("control recovery: PASS (gimbal reseed, friction ramp/brake, fault lock and stale feedback zero)");
}
