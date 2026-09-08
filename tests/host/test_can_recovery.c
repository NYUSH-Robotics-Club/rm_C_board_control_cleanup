/* Exercise fault entry, mailbox cancellation, bounded retries and operator rearming. */
#include "main.h"
#include "bsp_can.h"
#include <assert.h>
#include <stdio.h>
static CAN_TypeDef regs[2];
CAN_HandleTypeDef hcan1={&regs[0]}, hcan2={&regs[1]};
static uint32_t now_ms, aborts, writes;
static int abort_completes=1;
uint32_t HAL_GetTick(void) { return now_ms; }
HAL_StatusTypeDef HAL_CAN_ConfigFilter(CAN_HandleTypeDef *h,CAN_FilterTypeDef *f)
{ (void)h; assert(f->SlaveStartFilterBank==14U); return HAL_OK; }
HAL_StatusTypeDef HAL_CAN_Start(CAN_HandleTypeDef *h) {h->Instance->free_slots=3;return HAL_OK;}
HAL_StatusTypeDef HAL_CAN_ActivateNotification(CAN_HandleTypeDef *h,uint32_t n) {(void)h;(void)n;return HAL_OK;}
HAL_StatusTypeDef HAL_CAN_GetRxMessage(CAN_HandleTypeDef *h,uint32_t n,CAN_RxHeaderTypeDef *r,uint8_t *d)
{(void)h;(void)n;(void)r;(void)d;return HAL_ERROR;}
HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef *h,CAN_TxHeaderTypeDef *t,uint8_t *d,uint32_t *m)
{(void)h;(void)t;(void)d;*m=0;writes++;return HAL_OK;}
HAL_StatusTypeDef HAL_CAN_AbortTxRequest(CAN_HandleTypeDef *h,uint32_t m)
{assert(m==7U);aborts++;if(abort_completes)h->Instance->free_slots=3;return HAL_OK;}
uint32_t HAL_CAN_GetTxMailboxesFreeLevel(CAN_HandleTypeDef *h){return h->Instance->free_slots;}
uint32_t HAL_CAN_GetError(CAN_HandleTypeDef *h){(void)h;return 0;}
int main(void) {
 assert(BspCan_Start(BSP_CAN_CHANNEL_1)&&BspCan_Start(BSP_CAN_CHANNEL_2));
 uint8_t zero[8]={0}, command[8]={0,100};
 BspCan_Service(0);
 assert(!BspCan_TryArm(499));
 assert(!BspCan_Write(BSP_CAN_CHANNEL_1,0x200,command,8));
 assert(BspCan_Write(BSP_CAN_CHANNEL_1,0x200,zero,8));
 assert(BspCan_TryArm(500));
 assert(BspCan_Write(BSP_CAN_CHANNEL_1,0x200,command,8));
 regs[0].ESR=0x00FF0037; regs[0].free_slots=0; regs[1].free_slots=0;
 now_ms=600;
 assert(!BspCan_Write(BSP_CAN_CHANNEL_1,0x200,command,8));
 assert(!BspCan_OutputsArmed() && aborts==2 && regs[1].free_slots==3);
 const BspCanRecovery *s=BspCan_GetRecovery(BSP_CAN_CHANNEL_1);
 assert(s->fault_count==1 && s->first_fault_esr==0x00FF0037);
 BspCan_Service(601); assert(regs[0].MCR & CAN_MCR_INRQ);
 regs[0].MSR=CAN_MSR_INAK;
 BspCan_Service(602); assert(!(regs[0].MCR & CAN_MCR_INRQ));
 regs[0].MSR=0; BspCan_Service(603);
 assert(!BspCan_TryArm(2000)); /* elapsed time alone is never a recovery */
 regs[0].ESR=0; BspCan_Service(604);
 assert(s->recovery_count==1 && !BspCan_OutputsArmed());
 assert(!BspCan_TryArm(1103)); assert(BspCan_TryArm(1104));
 /* Abort stuck: never request initialization with old mailboxes pending. */
 abort_completes=0;regs[0].ESR=4;regs[0].free_slots=0;
 BspCan_Service(1200);BspCan_Service(1220);
 assert(s->phase==5 && s->timeout_count==1 && !(regs[0].MCR&1));
 assert(!BspCan_Write(BSP_CAN_CHANNEL_2,0x200,command,8));
 BspCan_Service(2219);assert(s->phase==5);
 abort_completes=1;BspCan_Service(2220);assert(s->phase==1);
 BspCan_Service(2221);assert(s->phase==2);
 assert(s->last_error_esr==0x00FF0037 && writes==2);
 puts("CAN recovery: PASS (real BSP, abort before init, bus sync, lock, timeout/retry)");
}
