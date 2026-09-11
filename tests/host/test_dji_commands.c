/* Exercise the real DJI adapter, registry, and CAN aggregation with captured BSP writes. */
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "can_manager.h"
#include "bsp_can.h"
#include "bsp_time.h"
#include "motor_adapter.h"
#include "motor_driver.h"
#include "dji_motor_protocol.h"
#include "robot_config.h"
#include "message_center.h"

CAN_Manager_t can1_manager, can2_manager;
static CAN_HandleTypeDef handles[2];
static MotorRegistry_t registries[2];
static MotorContext_t contexts[16];
static unsigned count;
static struct { BspCanChannel bus; uint16_t id; uint8_t data[8]; } frames[32];
uint32_t BspTime_NowMs(void) { return 100; }
uint32_t BspTime_NowUs(void) { return 100000; }
bool BspCan_Start(BspCanChannel c) { (void)c; return true; }
bool BspCan_OutputsArmed(void) { return true; }
bool BspCan_Read(BspCanChannel c, BspCanFrame *f) { (void)c; (void)f; return false; }
bool BspCan_MatchesNativeHandle(BspCanChannel c, const void *h)
{
    return (c == BSP_CAN_CHANNEL_1 && h == &handles[0]) ||
           (c == BSP_CAN_CHANNEL_2 && h == &handles[1]);
}
bool BspCan_Write(BspCanChannel c, uint16_t id, const uint8_t *data, uint8_t len)
{
    assert(len == 8 && count < 32);
    frames[count].bus = c; frames[count].id = id;
    memcpy(frames[count++].data, data, 8);
    return true;
}
/* The driver shim provides contexts and routes commands; the adapter and CAN code are real. */
MotorContext_t *MotorDriver_GetContext(uint8_t id) { return id < 16 ? &contexts[id] : NULL; }
void MotorDriver_SendCurrent(uint8_t id, int16_t value)
{
    CAN_Manager_t *m = contexts[id].config->can_channel == CAN_CHANNEL_1 ? &can1_manager : &can2_manager;
    assert(CAN_Manager_SendMotorCurrent(m, id, value) == HAL_OK);
}
void MotorDriver_FlushAll(void) { assert(CAN_Manager_FlushTx(&can1_manager) == HAL_OK); assert(CAN_Manager_FlushTx(&can2_manager) == HAL_OK); }
void MotorDriver_ResetPID(uint8_t id) { (void)id; }
int16_t MotorDriver_ComputeCurrent(uint8_t id, float value, bool angle) { (void)id; (void)value; (void)angle; return 0; }

static void expect(unsigned i, BspCanChannel c, uint16_t id, unsigned slot, int16_t value)
{
    uint8_t data[8] = {0};
    data[slot * 2] = (uint8_t)((uint16_t)value >> 8);
    data[slot * 2 + 1] = (uint8_t)value;
    assert(frames[i].bus == c && frames[i].id == id);
    assert(memcmp(frames[i].data, data, 8) == 0);
}

int main(void)
{
    const RobotConfig_t *robot = RobotConfig_Get();
    const MotorAdapterOps *adapter = DjiMotorAdapter_Get();
    for (uint8_t i = 0; i < robot->total_motor_count; ++i) {
        const MotorConfig_t *cfg = &robot->motor_configs[i];
        assert(adapter->validate(cfg) == ROBOT_STATUS_OK);
        contexts[cfg->motor_id] = (MotorContext_t){.initialized=true, .config=cfg, .type=cfg->type};
    }
    assert(CAN_Manager_Init(&can1_manager, CAN_CHANNEL_1, &handles[0], robot, &registries[0]) == HAL_OK);
    assert(CAN_Manager_Init(&can2_manager, CAN_CHANNEL_2, &handles[1], robot, &registries[1]) == HAL_OK);
    assert(adapter->command_current(5, 30000) == ROBOT_STATUS_OK);
    assert(adapter->command_current(8, -30000) == ROBOT_STATUS_OK);
    adapter->flush();
    assert(count == 2);
    expect(0, BSP_CAN_CHANNEL_1, 0x2FE, 0, 546);
    expect(1, BSP_CAN_CHANNEL_2, 0x1FF, 3, -25000);
    adapter->flush(); assert(count == 2); // No unused command group emitted.
    assert(CAN_Manager_SendMotorCurrent(&can1_manager, 5, -32768) == HAL_OK);
    adapter->flush(); expect(2, BSP_CAN_CHANNEL_1, 0x2FE, 0, -546);
    assert(CAN_Manager_SendGM6020Current(&handles[0], 5, 32767) == HAL_OK);
    expect(3, BSP_CAN_CHANNEL_1, 0x2FE, 0, 546);
    assert(adapter->stop(5) == ROBOT_STATUS_OK);
    adapter->flush(); expect(4, BSP_CAN_CHANNEL_1, 0x2FE, 0, 0);

    MotorConfig_t cfg = *contexts[5].config;
    cfg.protocol.dji.gm6020_mode = GM6020_COMMAND_VOLTAGE;
    cfg.can_tx_id = 0x2FE;
    assert(adapter->validate(&cfg) != ROBOT_STATUS_OK); // Reject mode/ID mismatch.
    cfg.can_tx_id = 0x2FF; cfg.tx_slot = 1;
    assert(adapter->validate(&cfg) != ROBOT_STATUS_OK);
    cfg.tx_slot = 0; cfg.protocol.dji.command_limit = 26000;
    assert(adapter->validate(&cfg) != ROBOT_STATUS_OK);
    cfg.protocol.dji.command_limit = 0;
    assert(DjiMotor_CommandLimit(&cfg) == 25000);
    cfg.protocol.dji.gm6020_mode = (GM6020CommandMode_e)99;
    assert(adapter->validate(&cfg) != ROBOT_STATUS_OK);

    cfg = *contexts[5].config;
    /* Keep optional current-mode coverage independent of the live robot mode. */
    cfg.protocol.dji.gm6020_mode = GM6020_COMMAND_CURRENT;
    cfg.protocol.dji.command_limit = 4096;
    cfg.can_tx_id = 0x2FE;
    RobotConfig_t one = {.motor_configs=&cfg, .total_motor_count=1};
    contexts[5].config = &cfg;
    assert(CAN_Manager_Init(&can1_manager, CAN_CHANNEL_1, &handles[0], &one, &registries[0]) == HAL_OK);
    assert(adapter->command_current(5, 30000) == ROBOT_STATUS_OK);
    assert(adapter->command_current(8, -30000) == ROBOT_STATUS_OK);
    adapter->flush();
    expect(5, BSP_CAN_CHANNEL_1, 0x2FE, 0, 4096);
    expect(6, BSP_CAN_CHANNEL_2, 0x1FF, 3, -25000);
    assert(CAN_Manager_SendGM6020Current(&handles[0], 5, -30000) == HAL_OK);
    expect(7, BSP_CAN_CHANNEL_1, 0x2FE, 0, -4096);

    cfg.can_rx_id = 0x205; cfg.can_tx_id = 0x1FE;
    cfg.protocol.dji.command_limit = 0;
    assert(CAN_Manager_Init(&can1_manager, CAN_CHANNEL_1, &handles[0], &one, &registries[0]) == HAL_OK);
    assert(CAN_Manager_SendMotorCurrent(&can1_manager, 5, -30000) == HAL_OK);
    adapter->flush(); expect(8, BSP_CAN_CHANNEL_1, 0x1FE, 0, -16384);
    cfg.can_tx_id = 0x1FF; // Bypass adapter and ensure CAN boundary also rejects mismatch.
    assert(CAN_Manager_SendMotorCurrent(&can1_manager, 5, 100) == HAL_ERROR);
    adapter->flush(); assert(count == 9);
    puts("DJI voltage/current command integration: PASS");
    return 0;
}
