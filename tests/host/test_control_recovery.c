/* Real application callbacks: stale feedback and disable must erase old motor commands. */
#include "gimbal_controller.h"
#include "shooter_controller.h"
#include "motor_driver.h"
#include "motor_service.h"
#include "message_center.h"
#include "infantry_standard.h"
#include "logger.h"
#include <assert.h>
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
static void gimbal_step(uint32_t now,bool enabled,bool fresh){
 now_ms=now;
 if(fresh)motors[5].last_feedback_time=motors[8].last_feedback_time=now;
 GimbalCmd cmd={.enabled=enabled};
 assert(MsgCenter_Publish(TOPIC_GIMBAL_CMD,&cmd,sizeof(cmd))==0);
 MsgCenter_Dispatch();
}
int main(void){
 MsgEvent queue[32];
 MsgCenter_Init(queue,32);
 for(uint8_t id=5;id<=8;id+=3){
  MotorContext_t *m=&motors[id];m->config=MotorService_GetConfig(id);m->initialized=true;
  m->type=MOTOR_TYPE_GM6020;m->role=m->config->role;m->angle_raw=id==5?5000:2500;
  PID_Init(&m->pid_outer,1,0,0,600,100);
  PID_Init(&m->pid_inner,1,0,0,25000,100);
 }
 GimbalApp_Init(); ShooterApp_Init();
 gimbal_step(100,true,true);assert(outputs[5]==0&&outputs[8]==0);
 gimbal_step(200,true,true);assert(motors[5].angle_target==5000);
 motors[5].angle_raw=5100;gimbal_step(210,true,true);assert(outputs[5]!=0);
 motors[5].pid_inner.iout=999;
 gimbal_step(311,true,false);assert(outputs[5]==0&&outputs[8]==0&&motors[5].pid_inner.iout==0);
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
 assert(outputs[6]==0&&outputs[7]==0&&setpoints[6]==0&&setpoints[7]==0);
 can_armed=true;now_ms=601;
 MsgCenter_Publish(TOPIC_SHOOT_CMD,&shoot,sizeof(shoot));MsgCenter_Dispatch();
 assert(outputs[6]==0&&outputs[7]==0); /* Stale feedback must never be used to brake. */
 /* Exercise the configured current-mode PID after feedback alignment, then lose yaw only. */
 MotorContext_t *yaw=&motors[5];
 const MotorConfig_t *cfg=yaw->config;
 PID_Init(&yaw->pid_outer,cfg->pid_outer.kp,cfg->pid_outer.ki,cfg->pid_outer.kd,
          cfg->pid_outer.output_max,cfg->pid_outer.integral_max);
 PID_Init(&yaw->pid_inner,cfg->pid_inner.kp,cfg->pid_inner.ki,cfg->pid_inner.kd,
          cfg->pid_inner.output_max,cfg->pid_inner.integral_max);
 gimbal_step(700,true,true);gimbal_step(800,true,true);
 yaw->angle_raw+=20;
 gimbal_step(810,true,true);
 assert(outputs[5]<0&&outputs[5]>=-MotorDriver_GetCommandLimit(5));
 yaw->angle_raw-=40;
 gimbal_step(820,true,true);
 assert(outputs[5]>0&&outputs[5]<=MotorDriver_GetCommandLimit(5));
 motors[8].last_feedback_time=921;
 gimbal_step(921,true,false);
 assert(outputs[5]==0&&outputs[8]==0&&yaw->pid_inner.iout==0);
 /* 重装后的5398不在旧限位内；关闭限位仍须等待反馈稳定，并锁存实测位置。 */
 MotorContext_t *pitch=&motors[8];
 pitch->angle_raw=5398;
 gimbal_step(1000,true,true);assert(outputs[5]==0&&outputs[8]==0);
 gimbal_step(1100,true,true);assert(pitch->angle_target==5398);
 SensorData sensor={0};
 pitch->angle_target=5398;
 (void)GimbalController_PitchControl(8,-1,&sensor,true);
 assert(pitch->angle_target==5458); /* 不再被4000裁剪。 */
 pitch->angle_raw=8190;pitch->angle_target=8190;
 (void)GimbalController_PitchControl(8,-1,&sensor,true);
 assert(pitch->angle_target==58); /* 跨零保留60刻度的增量。 */
 pitch->angle_raw=2;pitch->angle_target=2;
 (void)GimbalController_PitchControl(8,1,&sensor,true);
 assert(pitch->angle_target==8134);
 gimbal_step(1201,true,false);assert(outputs[5]==0&&outputs[8]==0);
 /* 默认开启的限位仍挡住越界启动，校准完成后可恢复原有保护。 */
 MotorConfig_t limited_pitch=*pitch->config;
 limited_pitch.limits.gm6020.angle_limits_disabled=false;
 pitch->config=&limited_pitch;pitch->angle_raw=5398;
 gimbal_step(1300,true,true);gimbal_step(1400,true,true);
 assert(outputs[5]==0&&outputs[8]==0);
 pitch->angle_raw=2500;
 gimbal_step(1410,true,true);assert(pitch->angle_target==2500);
 pitch->angle_target=3990;
 (void)GimbalController_PitchControl(8,-1,&sensor,true);
 assert(pitch->angle_target==4000);
 pitch->config=MotorService_GetConfig(8);
 puts("control recovery: PASS (gimbal reseed, friction ramp/brake, fault lock and stale feedback zero)");
}
