# CLAUDE.md

> **归档文档，不是当前规范。** 本文保留重构前的仓库说明，其中的三层架构、
> 载荷上限、循环频率和修改底层建议可能已失效。当前入口见
> [文档中心](../README.md)和[全局架构](../architecture/overview.md)。

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

RoboMaster control firmware for STM32F407 microcontroller running on the NYUSH Robotics Club C Board. This firmware controls a competition robot with a chassis, gimbal, and shooter system using a three-layer architecture.

## Build Commands

### Build
```bash
# Using CMake directly
cmake --build build

# Or use VSCode task
# Ctrl+Shift+B / Cmd+Shift+B (default task is flash: dfu-util)
```

### Flash to Board
Two methods available:

**Method 1: DFU-Util (USB, requires DFU mode)**
```bash
# Build and flash in one command (VSCode default build task)
# This runs: build → convert to .bin → flash via dfu-util
arm-none-eabi-objcopy -O binary build/NYUSH_Infantry.elf build/NYUSH_Infantry.bin
dfu-util -a 0 -s 0x08000000:leave -D build/NYUSH_Infantry.bin
```

**Method 2: OpenOCD (requires ST-Link debugger)**
```bash
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "init; reset init; program build/NYUSH_Infantry.elf verify reset exit"
```

### Clean Build
```bash
rm -rf build/*
cmake -S . -B build
cmake --build build
```

## Architecture

### Three-Layer Design

The codebase follows a strict three-layer architecture:

1. **Application Layer** (`application/`) - High-level robot control logic
   - `cmd/` - Central command controller coordinating all subsystems
   - `chassis/` - Four-wheel chassis control
   - `gimbal/` - Pitch/yaw gimbal control
   - `shoot/` - Shooter mechanism control
   - `app_subscriptions.h` - Defines topic subscriptions for each app

2. **Module Layer** (`modules/`) - Reusable functional modules
   - `message_center/` - Publish-subscribe message bus (event-driven communication)
   - `can_comm/` - CAN bus communication and motor management
   - `motor/` - Motor driver interfaces (GM6020)
   - `imu/` - IMU sensor drivers (BMI088, WT61C)
   - `remote/` - Remote control receiver (DR16)
   - `vision_comm/` - Vision system communication via Seasky protocol
   - `algorithm/` - Control algorithms (PID controllers)
   - `debug_print/` - Debug printing utilities

3. **Hardware Layer** (`Inc/`, `Src/`) - STM32CubeMX generated code
   - HAL drivers and peripheral initialization
   - **CRITICAL**: Only modify code between `USER CODE BEGIN` and `USER CODE END` markers
   - Changes outside these markers will be overwritten when regenerating from STM32CubeMX

### Communication Flow

All inter-module communication uses the publish-subscribe pattern via message center:

```
Hardware Event (e.g., CAN RX interrupt)
    ↓
Module publishes to topic (e.g., TOPIC_MOTOR_FEEDBACK)
    ↓
Message Center queues event in ring buffer
    ↓
Main loop calls MsgCenter_Dispatch()
    ↓
Subscribers' callbacks are invoked
    ↓
Application controllers process data and publish commands
    ↓
Modules execute commands (e.g., send CAN motor currents)
```

## Message Center (Pub-Sub System)

The message center is the central nervous system of the firmware. Key points:

### Available Topics
- `TOPIC_RC_UPDATE` - Remote control data
- `TOPIC_IMU_UPDATE` - IMU sensor data (gyro, accel)
- `TOPIC_CAN_RX` - Raw CAN messages
- `TOPIC_MOTOR_FEEDBACK` - Motor feedback (M3508 chassis motors)
- `TOPIC_GM6020_FEEDBACK` - GM6020 motor feedback (gimbal)
- `TOPIC_CHASSIS_CMD` - Chassis movement commands
- `TOPIC_GIMBAL_CMD` - Gimbal pitch/yaw commands
- `TOPIC_SHOOT_CMD` - Shooter commands
- `TOPIC_VISION_DATA` - Vision system target data

### Publishing Messages
```c
// From interrupt handlers or anywhere
MsgCenter_Publish(TOPIC_IMU_UPDATE, &sensor_data, sizeof(SensorData));
```
- **ISR-safe**: Can be called from interrupt handlers
- Messages are queued in ring buffer (default 128 events)
- Overwrite policy if buffer is full

### Subscribing to Topics
```c
static void on_chassis_cmd(const MsgEvent *ev, void *user_data) {
    if (ev->size == sizeof(ChassisCmd)) {
        ChassisCmd cmd;
        memcpy(&cmd, ev->data, sizeof(ChassisCmd));
        // Process command...
    }
}

void ChassisApp_Init(void) {
    MsgCenter_Subscribe(TOPIC_CHASSIS_CMD, on_chassis_cmd, NULL);
}
```

### Dispatching Events
```c
// In main loop (Src/main.c)
while (1) {
    MsgCenter_Dispatch();  // Processes all queued events
    HAL_Delay(5);
}
```

### Limitations
- Max payload: 64 bytes (`MC_MAX_PAYLOAD`)
- Max subscribers per topic: 8 (`MC_MAX_SUBS_PER_TOPIC`)
- Ring buffer size: 128 events (configurable in `main.c`)

## CAN Communication

### Motor Types and IDs

**GM6020 (Gimbal Motors)**
- Feedback CAN ID: `0x205 + motor_id` (motor_id 1-7 → CAN ID 0x205-0x20B)
- Control CAN ID: `0x1FF` (motors 1-4) or `0x2FF` (motors 5-7)
- Current range: -25000 to 25000

**M3508 + C620 ESC (Chassis/Shooter Motors)**
- Feedback CAN ID: `0x200 + motor_id` (motor_id 1-8 → CAN ID 0x201-0x208)
- Control CAN ID: `0x200` (motors 1-4) or `0x1FF` (motors 5-8)
- Current range: -16384 to 16384

### CAN Data Format
All motor commands and feedback split 16-bit values across two bytes:
```c
// High byte first (big-endian)
data[0] = (value >> 8) & 0xFF;  // High byte
data[1] = value & 0xFF;         // Low byte
```

### CAN Managers
Two CAN channels managed by `can_manager.c`:
- `can1_manager` - Typically chassis motors
- `can2_manager` - Typically gimbal motors

CAN RX callbacks automatically publish to message center:
- Raw CAN data → `TOPIC_CAN_RX`
- Parsed motor feedback → `TOPIC_MOTOR_FEEDBACK` or `TOPIC_GM6020_FEEDBACK`

## Vision Communication (Seasky Protocol)

### Communication Channel
- Physical: USB Virtual COM Port (VCP)
- Protocol: Seasky binary protocol with CRC8/CRC16
- Receive: 18 bytes (vision → board)
- Send: 36 bytes (board → vision)

### Frame Structure
```
[SOF: 0xA5][Data Length: 2 bytes][CRC8: 1 byte]
[Command ID: 2 bytes][Flags: 2 bytes][Float Data: N×4 bytes]
[CRC16: 2 bytes]
```

### Command IDs
- `0x0001` - Vision data from upper computer (receive)
- `0x0002` - Attitude data to upper computer (send)

### Receive Data (Vision → Board)
- Fire mode (NO_FIRE, AUTO_FIRE, AUTO_AIM)
- Target state (NO_TARGET, TARGET_CONVERGING, READY_TO_FIRE)
- Target type (HERO1, INFANTRY3, etc.)
- Pitch/yaw angles (radians)
- Published to `TOPIC_VISION_DATA`

### Send Data (Board → Vision)
- Yaw, pitch, roll from IMU (radians)
- Enemy color, work mode, bullet speed flags
- Sent at 100Hz (every 10ms)

### Testing
Use `script/test_vision_comm.py` to simulate vision system and test protocol implementation.

## Initialization Sequence (Src/main.c)

Understanding the initialization order is critical:

```c
// 1. HAL initialization
HAL_Init();
SystemClock_Config();

// 2. Peripheral initialization (STM32CubeMX generated)
MX_GPIO_Init();
MX_DMA_Init();
MX_CAN1_Init();
MX_CAN2_Init();
MX_SPI1_Init();      // BMI088 IMU
MX_I2C3_Init();
MX_USART1_UART_Init();  // WT61C IMU
MX_USART3_UART_Init();  // DR16 Remote Control
MX_USB_DEVICE_Init();   // Vision communication
MX_TIM4_Init();
MX_USART6_UART_Init();

// 3. Module initialization
BMI088_init();
MsgCenter_Init(g_msg_queue, MSG_CENTER_QUEUE_LEN);

// 4. Application initialization (in this order!)
CmdController_Init();    // Command controller first
ChassisApp_Init();
ShooterApp_Init();
GimbalApp_Init();

// 5. Start subsystems
remote_control_init();
CAN_Manager_Init(&can1_manager, CAN_CHANNEL_1, &hcan1);
CAN_Manager_Init(&can2_manager, CAN_CHANNEL_2, &hcan2);
CAN_Manager_Start(&can1_manager);
CAN_Manager_Start(&can2_manager);
VisionComm_Init();

// 6. Wait for ESC boot (500ms)
HAL_Delay(WAIT_ESC_BOOT_MS);

// 7. Start IMU
WT61C_Init(&huart1);
HAL_UARTEx_ReceiveToIdle_DMA(&huart1, wt61c_rxbuf, RX_DMA_BUF_SZ);

// 8. Main loop
while (1) {
    MsgCenter_Dispatch();  // Critical: dispatches all queued events
    HAL_Delay(5);          // 200Hz main loop
}
```

## CMake Structure

The build uses layered static libraries:

```cmake
# Main executable
add_executable(NYUSH_Infantry)
target_link_libraries(NYUSH_Infantry stm32cubemx application modules)

# Module layer library
add_library(modules STATIC
    modules/message_center/message_center.c
    modules/algorithm/pid.c
    modules/motor/gm6020_motor.c
    # ... all module sources
)

# Application layer library
add_library(application STATIC
    application/cmd/cmd_controller.c
    application/chassis/chassis_controller.c
    application/shoot/shooter_controller.c
    application/gimbal/gimbal_controller.c
)
target_link_libraries(application PUBLIC stm32cubemx modules)
```

When adding new files:
1. Add source to appropriate library in `CMakeLists.txt`
2. Add include directory if creating new module folder
3. Rebuild with `cmake --build build`

## Development Workflow

### Adding a New Feature
1. **Plan the data flow**: What topics will you publish/subscribe to?
2. **Implement in module layer**: Add reusable functionality in `modules/`
3. **Integrate in application layer**: Subscribe to topics, process events, publish commands
4. **Update CMakeLists.txt**: Add new source files to appropriate library
5. **Test incrementally**: Use debug printing (`modules/debug_print/`) to verify

### Modifying STM32CubeMX Configuration
1. Open `NYUSH_Infantry.ioc` in STM32CubeMX
2. Make peripheral changes (add UART, change pin, etc.)
3. Generate code (preserves `USER CODE` sections)
4. Rebuild firmware

### Common Pitfalls
- **Forgetting to dispatch**: Messages won't be processed without `MsgCenter_Dispatch()` in main loop
- **Buffer overflow**: Publishing messages >64 bytes will truncate data
- **Missing subscriptions**: Controllers won't receive data if not subscribed in `*App_Init()`
- **Editing outside USER CODE blocks**: Changes will be lost when regenerating from STM32CubeMX
- **CAN ID conflicts**: Ensure motor IDs are set correctly on hardware (see docs/tutorials/can.md)

## Logger System (modules/logger/)

The firmware uses a unified logger module for all debug output with tag-based filtering and automatic rate limiting.

### Log Tags (Subsystems)
- `LOG_TAG_SYS` - System/boot messages
- `LOG_TAG_CMD` - Command controller
- `LOG_TAG_CHA` - Chassis controller
- `LOG_TAG_GIM` - Gimbal controller
- `LOG_TAG_SHO` - Shooter controller
- `LOG_TAG_SEN` - Sentry controller (swerve chassis)
- `LOG_TAG_MOT` - Motor driver
- `LOG_TAG_IMU` - IMU sensors
- `LOG_TAG_CAN` - CAN communication
- `LOG_TAG_VIS` - Vision communication
- `LOG_TAG_RC` - Remote control
- `LOG_TAG_DEBUG` - General debug

### API Usage

```c
// Text logging with levels
LOG_INFO(LOG_TAG_SYS, "System initialized");
LOG_WARN(LOG_TAG_MOT, "Motor %d timeout", motor_id);
LOG_ERROR(LOG_TAG_CAN, "CAN bus error: 0x%X", error_code);
LOG_DEBUG(LOG_TAG_GIM, "Angle: %.2f", angle);

// CSV data logging (auto rate-limited)
LOG_CSV(LOG_TAG_GIM, "PITCH,%.2f,%.2f,%d", target, current, speed);
LOG_CSV(LOG_TAG_IMU, "%.2f,%.2f,%.2f", yaw, pitch, roll);
```

### Configuration (modules/logger/logger_config.h)

Enable/disable tags at compile time:
```c
#define LOG_ENABLE_GIM    1  // Enable gimbal logs
#define LOG_ENABLE_CAN    0  // Disable CAN logs (too verbose)
```

### CSV Format Standard

All CSV data: `TAG,timestamp_ms,field1,field2,...`

Example output:
```
GIM,123456,PITCH,45.20,44.80,150,12.50
IMU,123500,45.20,30.10,0.00,45.20,0
CMD,123600,1,2,3,45.20,30.10,15.50
```

## Python Debug Tools (script/)

### smart_logger.py - Unified Debug Tool

**Replaces all individual plotting scripts** with tag-based filtering and auto-detection.

```bash
# Monitor all subsystems
python3 script/smart_logger.py

# Monitor specific tags
python3 script/smart_logger.py --tags GIM,IMU

# Save data without plotting
python3 script/smart_logger.py --save data.csv --no-plot

# List available tags
python3 script/smart_logger.py --list-tags
```

Features:
- Tag subscription (filter which subsystems to monitor)
- Auto-detect CSV formats (CMD, IMU, GIM sub-types)
- Real-time plotting with auto-layout
- Data recording/playback
- Auto port detection

### Other Tools

- `test_vision_comm.py` - Test vision communication protocol (Seasky binary protocol)
- `deprecated/` - Old plotting scripts (replaced by smart_logger.py)

## Documentation

See `docs/` for detailed guides:
- `tutorials/setup-guide.md` - Development environment setup
- `architecture.md` - Detailed architecture explanation
- `pub-sub.md` - Message center implementation details
- `vision-protocol.md` - Seasky protocol specification
- `tutorials/can.md` - CAN bus and motor communication

## Key Design Principles

1. **Event-driven**: All communication goes through message center
2. **Loose coupling**: Modules don't directly call each other
3. **ISR-safe publishing**: Interrupt handlers can safely publish to message center
4. **Callback processing**: All callbacks run in main loop context (not ISRs)
5. **STM32CubeMX compatibility**: Hardware layer respects USER CODE boundaries
