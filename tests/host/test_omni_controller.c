/*
 * Exercise message -> omni strategy -> speed PID -> CAN1 0x200 packing.
 * The motor-service shim forwards to real CAN aggregation; time/log/BSP are fake.
 * Geometry is synthetic and never installed on the real board.
 */
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "chassis_controller.h"
#include "chassis_strategy.h"
#include "robot_config.h"
#include "motor_service.h"
#include "control_messages.h"
#include "message_center.h"
#include "logger.h"
#include "can_manager.h"
#include "bsp_can.h"

CAN_Manager_t can1_manager, can2_manager;
static CAN_HandleTypeDef handles[2];
static MotorRegistry_t registry;
static RobotConfig_t robot;
static OmniChassisConfig geometry;
static uint32_t now;
static float commanded_rpm[16];
static int16_t commanded_current[16];
static uint8_t last_frame[8];
static unsigned writes;
static MsgEvent queue[32];
const RobotConfig_t *RobotConfig_Get(void) { return &robot; }
uint32_t BspTime_NowMs(void) { return now; }
uint32_t BspTime_NowUs(void) { return now*1000U; }
void Logger_Log(LogTag_t t, LogLevel_t l, const char *f, ...) {(void)t;(void)l;(void)f;}
bool BspCan_Start(BspCanChannel c) {(void)c;return true;}
bool BspCan_OutputsArmed(void) {return true;}
bool BspCan_Read(BspCanChannel c, BspCanFrame *f) {(void)c;(void)f;return false;}
bool BspCan_MatchesNativeHandle(BspCanChannel c, const void *h)
{
    return c==BSP_CAN_CHANNEL_1 && h==&handles[0];
}
bool BspCan_Write(BspCanChannel c, uint16_t id, const uint8_t *d, uint8_t n)
{
    assert(c==BSP_CAN_CHANNEL_1 && id==0x200 && n==8);
    memcpy(last_frame,d,8); ++writes; return true;
}
const MotorConfig_t *MotorService_GetConfig(uint8_t id)
{
    for (uint8_t i=0;i<robot.total_motor_count;++i)
        if (robot.motor_configs[i].motor_id==id) return &robot.motor_configs[i];
    return NULL;
}
uint8_t MotorService_FindByRole(MotorRole_e role, uint8_t *ids, uint8_t cap)
{
    uint8_t count=0;
    for (uint8_t i=0;i<robot.total_motor_count && count<cap;++i)
        if (robot.motor_configs[i].role==role) ids[count++]=robot.motor_configs[i].motor_id;
    return count;
}
RobotStatus MotorService_CommandCurrent(uint8_t id, int16_t value)
{
    assert(id>=1 && id<=4); /* A bad geometry must never stop yaw or another role. */
    commanded_current[id]=value;
    assert(CAN_Manager_SendMotorCurrent(&can1_manager,id,value)==HAL_OK);
    return ROBOT_STATUS_OK;
}
RobotStatus MotorService_CommandConfigured(uint8_t id, float rpm, int16_t value)
{
    commanded_rpm[id]=rpm;
    return MotorService_CommandCurrent(id,value);
}
static void setup(void)
{
    static MotorConfig_t test_motors[16];
    const float c=.70710678118655f;
    robot=g_robot_config_infantry_standard;
    assert(robot.total_motor_count <= 16);
    memcpy(test_motors, robot.motor_configs, robot.total_motor_count * sizeof(test_motors[0]));
    robot.motor_configs = test_motors;
    /* Verify controller/slot behavior independently of the user's live PID tuning. */
    for (uint8_t i=0; i<robot.total_motor_count; ++i) {
        if (test_motors[i].role == MOTOR_ROLE_CHASSIS_DRIVE) {
            test_motors[i].pid_outer.kp = 10.0f;
            test_motors[i].pid_outer.ki = 0.0f;
            test_motors[i].pid_outer.kd = 0.0f;
        }
    }
    geometry=(OmniChassisConfig){
        .configured=1,.max_translation_mps=1,.max_rotation_radps=1,
        .wheels={
            {3,.2f,.3f,c,-c,.05f,10},
            {2,.2f,-.3f,c,c,.05f,10},
            {1,-.2f,-.3f,c,-c,.05f,10},
            {4,-.2f,.3f,c,c,.05f,10}
        }
    };
    robot.omni=&geometry;
    MsgCenter_Init(queue,32);
    assert(CAN_Manager_Init(&can1_manager,CAN_CHANNEL_1,&handles[0],&robot,&registry)==HAL_OK);
    ChassisApp_Init();
    memset(commanded_rpm,0,sizeof(commanded_rpm));
    memset(commanded_current,0,sizeof(commanded_current));
    now=0; writes=0;
}
static void feedback(uint8_t id, uint32_t tick)
{
    MotorFeedbackEvent m={.id=id,.speed=0,.tick_ms=tick};
    assert(MsgCenter_Publish(TOPIC_MOTOR_FEEDBACK,&m,sizeof(m))==0);
}
static void command(ChassisCmd cmd)
{
    assert(MsgCenter_Publish(TOPIC_CHASSIS_CMD,&cmd,sizeof(cmd))==0);
    MsgCenter_Dispatch();
    assert(CAN_Manager_FlushTx(&can1_manager)==HAL_OK);
    assert(writes>0);
    /* ID1/2/3/4 always occupy bytes 0/2/4/6 regardless of geometry order. */
    for (uint8_t id=1;id<=4;++id) {
        unsigned slot=(id-1)*2;
        int16_t packed=(int16_t)((uint16_t)last_frame[slot]<<8 | last_frame[slot+1]);
        assert(packed==commanded_current[id]);
    }
}
static void expect_stopped(void)
{
    for (uint8_t id=1;id<=4;++id) assert(commanded_current[id]==0);
}
int main(void)
{
    setup();
    ChassisCmd cmd={.vx=.3f,.vy=-.4f,.wz=.5f,.enabled=true};
    command(cmd); expect_stopped(); /* No feedback, even in the first 100 ms. */
    feedback(3,0); feedback(2,0); feedback(1,0);
    command(cmd); expect_stopped(); /* Three wheels are insufficient. */
    feedback(4,0); now=1;
    command(cmd);
    const float n=(1.0f/sqrtf(2.0f))/(3.14159265358979f*.1f)*60*10;
    assert(fabsf(commanded_rpm[3]-.45f*n)<.002f);
    assert(fabsf(commanded_rpm[2]-.15f*n)<.002f);
    assert(fabsf(commanded_rpm[1]+.95f*n)<.002f); /* ID1 direction=-1. */
    assert(fabsf(commanded_rpm[4]-.35f*n)<.002f); /* ID4 direction=-1. */
    assert(commanded_current[3]>0 && commanded_current[2]>0);
    assert(commanded_current[1]<0 && commanded_current[4]>0);

    now=101; feedback(3,101); feedback(2,101); feedback(1,101);
    command(cmd); expect_stopped(); /* One stale wheel stops all four. */
    feedback(4,101); now=102;
    command(cmd); assert(commanded_current[1]<0);
    cmd.enabled=false; now=103;
    command(cmd); expect_stopped(); /* Nonzero stale setpoints cannot bypass enable. */
    cmd.enabled=true; cmd.vx=NAN;
    command(cmd); expect_stopped();

    setup(); geometry.configured=0; MsgCenter_Init(queue,32); ChassisApp_Init();
    for (uint8_t id=1;id<=4;++id) feedback(id,0);
    cmd=(ChassisCmd){.vx=1,.enabled=true};
    command(cmd); expect_stopped();

    setup(); geometry.wheels[0].motor_id=5; MsgCenter_Init(queue,32); ChassisApp_Init();
    for (uint8_t id=1;id<=4;++id) feedback(id,0);
    command(cmd); expect_stopped(); /* Bad mapping does not touch yaw ID5. */

    setup(); geometry.wheels[0].radius_m=0;
    for (uint8_t id=1;id<=4;++id) feedback(id,0);
    command(cmd); expect_stopped();
    /* Actual user dimensions, using the production config rather than fixture.
     * 0.3 m/s forward -> 555.72 rotor RPM; 0.6 rad/s yaw -> 600.18 RPM.
     */
    setup(); geometry=*g_robot_config_infantry_standard.omni;
    assert(geometry.configured);
    MsgCenter_Init(queue,32); ChassisApp_Init();
    for (uint8_t id=1;id<=4;++id) feedback(id,0);
    now=1;
    command((ChassisCmd){.vx=1,.enabled=true});
    for (uint8_t id=1;id<=4;++id)
        assert(fabsf(fabsf(commanded_rpm[id])-555.72f)<.02f);
    command((ChassisCmd){.wz=1,.enabled=true});
    const float rotation_by_id[4]={-600.18f,600.18f,-600.18f,600.18f};
    for (uint8_t id=1;id<=4;++id)
        assert(fabsf(commanded_rpm[id]-rotation_by_id[id-1])<.03f);
    puts("omni controller and CAN1 wheel slot integration: PASS");
    return 0;
}
