# Architecture Overview

> **归档文档，不是当前规范。** 本文描述重构前的三层结构。新增代码必须遵循
> [当前全局架构](../architecture/overview.md)和冻结边界。

Our architecture draws inspiration from the foundational work by Hunan University, specifically their [basic framework](https://github.com/HNUYueLuRM/basic_framework.git).

## Three-Layer Architecture

The control system follows a three-layer architecture pattern:

1. **Application Layer** (`application/`) - High-level robot control logic
2. **Module Layer** (`modules/`) - Reusable functional modules
3. **Hardware Abstraction Layer** (`Inc/`, `Src/`) - STM32 HAL and hardware drivers

```
┌─────────────────────────────────────┐
│   Application Layer (application/) │
│   - Chassis controller              │
│   - Gimbal controller               │
│   - Shooter controller              │
│   - Command controller              │
└─────────────────────────────────────┘
              ↓ uses
┌─────────────────────────────────────┐
│   Module Layer (modules/)           │
│   - Message center (pub/sub)       │
│   - CAN communication               │
│   - IMU sensors                     │
│   - Motor drivers                   │
│   - Vision communication            │
└─────────────────────────────────────┘
              ↓ uses
┌─────────────────────────────────────┐
│   Hardware Layer (Inc/, Src/)       │
│   - STM32 HAL drivers               │
│   - Peripheral initialization       │
│   - Interrupt handlers              │
└─────────────────────────────────────┘
```

## Application Layer

The application layer contains the high-level control logic for different robot subsystems:

- **`chassis/`** - Controls the four-wheel chassis motors
- **`gimbal/`** - Controls the gimbal (pitch/yaw) motors
- **`shoot/`** - Controls the shooter mechanism
- **`cmd/`** - Central command controller that coordinates all subsystems
- **`app_subscriptions.h`** - Defines how application modules subscribe to messages

Each controller subscribes to relevant topics from the message center and publishes commands or status updates. For example, the chassis controller subscribes to:
- `TOPIC_CHASSIS_CMD` - Movement commands
- `TOPIC_IMU_UPDATE` - IMU sensor data
- `TOPIC_MOTOR_FEEDBACK` - Motor feedback data

## Module Layer

The module layer provides reusable functional components:

- **`message_center/`** - Publish-subscribe message system
- **`can_comm/`** - CAN bus communication and motor management
- **`imu/`** - IMU sensor drivers (BMI088, WT61C)
- **`motor/`** - Motor driver interfaces (GM6020)
- **`remote/`** - Remote control receiver
- **`vision_comm/`** - Vision system communication protocol
- **`algorithm/`** - Control algorithms (PID)
- **`debug_print/`** - Debug printing utilities

These modules are independent and can be used by multiple application controllers. They communicate through the message center using topics.

## Hardware Layer (Inc/ and Src/)

The `Inc/` and `Src/` directories contain STM32CubeMX-generated code for hardware initialization and HAL (Hardware Abstraction Layer) drivers. When you configure peripherals (CAN, UART, SPI, etc.) in STM32CubeMX and generate code, it creates these files.

**Only modify code between `USER CODE BEGIN` and `USER CODE END` markers!**

For example, in `Src/main.c`:

```c
/* USER CODE BEGIN Includes */
#include "message_center.h"
#include "chassis_controller.h"
// ... your includes
/* USER CODE END Includes */

/* USER CODE BEGIN 2 */
// Your initialization code here
MsgCenter_Init(g_msg_queue, MSG_CENTER_QUEUE_LEN);
ChassisApp_Init();
/* USER CODE END 2 */
```

If you modify code outside these markers, your changes will be **overwritten** the next time you regenerate code from STM32CubeMX.

## How It All Works Together

1. **Initialization** (`main.c`):
   - STM32CubeMX-generated code initializes hardware peripherals
   - User code initializes the message center
   - Application controllers subscribe to topics
   - Module drivers start (CAN, IMU, etc.)

2. **Runtime** (main loop):
   - Sensors publish data to message center (IMU updates, motor feedback, vision data)
   - Message center dispatches events to subscribers
   - Application controllers process events and compute control outputs
   - Controllers send commands via CAN or other interfaces

3. **Communication Flow**:
   ```
   Hardware Event (e.g., CAN message received)
        ↓
   Module Layer (CAN manager publishes TOPIC_CAN_RX)
        ↓
   Message Center (dispatches to subscribers)
        ↓
   Application Layer (chassis controller processes event)
        ↓
   Module Layer (sends motor command via CAN)
        ↓
   Hardware (CAN bus sends to motors)
   ```
