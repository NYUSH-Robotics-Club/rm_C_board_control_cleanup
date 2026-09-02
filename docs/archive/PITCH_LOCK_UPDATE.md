# Pitch轴控制优化更新说明

> **阶段性归档。** 本文记录当时的 Pitch 锁定改动，不保证所有命令、路径和
> 参数仍与当前代码一致。现行说明见[云台耦合补偿](../guides/gimbal-compensation.md)。

## 更新内容

优化了自瞄模式下的pitch轴控制策略：**自瞄时视觉不控制pitch，但遥控器可以手动调整pitch**。

## 功能特性

### 1. 自瞄模式下的Pitch控制

当视觉系统激活（`vision_valid = true`）时：
- ✅ 视觉系统不控制pitch轴（只控制yaw）
- ✅ 遥控器可以手动调整pitch角度
- ✅ 禁用yaw-pitch耦合补偿（避免干扰）
- ✅ 保持重力补偿以稳定pitch位置
- ✅ 操作员可以随时微调pitch以适应目标高度

### 2. 实时状态显示

USB CDC输出显示pitch控制模式：
```
=== 云台角度补偿计算 ===
云台高度: 30.0 cm
当前Pitch角度: XX.XX° (X.XXXX rad)
当前Yaw角度: XX.XX°
Pitch控制: 遥控器手动 (自瞄时视觉不控制pitch)    ← 自瞄模式
Pitch控制: 遥控器手动 + Yaw-Pitch耦合补偿        ← 手动模式
目标水平距离: XXX.X cm
...
```

### 3. 控制模式对比

| 模式 | Yaw控制 | Pitch控制 | Yaw-Pitch补偿 |
|-----|---------|-----------|--------------|
| **自瞄模式** | 视觉追踪 | 遥控器手动 | 禁用 |
| **手动模式** | 遥控器手动 | 遥控器手动 | 启用 |

## 修改的文件

```
application/gimbal/gimbal_controller.c
  - 参数改名：lock_pitch → disable_yaw_pitch_compensation
  - 移除pitch完全锁定逻辑
  - pitch始终响应遥控器输入（s_last_cmd.pitch_rate）
  - 自瞄时禁用yaw-pitch耦合补偿
  - 更新状态显示，说明pitch由遥控器控制

application/gimbal/gimbal_controller.h
  - 更新函数声明和注释

docs/gimbal_compensation.md
  - 更新pitch控制模式说明
  - 明确自瞄时遥控器可控制pitch
  - 更新测试指南
```

## 使用方法

### 1. 编译和烧录
```bash
cd /home/nyu/Codespace/robomaster-control
cmake --build build
arm-none-eabi-objcopy -O binary build/NYUSH_Infantry.elf build/NYUSH_Infantry.bin
dfu-util -a 0 -s 0x08000000:leave -D build/NYUSH_Infantry.bin
```

### 2. 测试Pitch控制

#### 基本测试步骤：
1. **手动模式测试**：
   - 上电后，用遥控器调整pitch角度
   - 验证pitch响应正常
   - 显示 "Pitch控制: 遥控器手动 + Yaw-Pitch耦合补偿"

2. **自瞄模式测试**：
   - 触发自瞄模式（vision_valid = true）
   - **关键**：用遥控器调整pitch → 应该可以响应
   - 显示 "Pitch控制: 遥控器手动 (自瞄时视觉不控制pitch)"
   - Yaw轴由视觉系统控制

3. **实战场景测试**：
   - 启动自瞄，锁定目标
   - 如果目标高度变化，用遥控器微调pitch
   - Yaw继续跟踪，pitch由操作员控制

#### 查看实时状态：
```bash
# 方法1：串口终端
screen /dev/ttyACM0 115200

# 方法2：Python监控工具
python3 script/smart_logger.py --tags GIM
```

### 3. 预期效果

**自瞄模式下**：
- ✅ Yaw轴由视觉系统追踪目标
- ✅ Pitch可以用遥控器手动调整
- ✅ USB CDC显示 "Pitch控制: 遥控器手动 (自瞄时视觉不控制pitch)"
- ✅ 无yaw-pitch耦合补偿（避免干扰yaw追踪）

**手动模式下**：
- ✅ Pitch和Yaw都响应遥控器输入
- ✅ USB CDC显示 "Pitch控制: 遥控器手动 + Yaw-Pitch耦合补偿"
- ✅ Yaw-pitch耦合补偿正常工作（提高手动瞄准精度）

## 技术细节

### 核心实现

#### 1. Pitch控制函数
```c
int16_t GimbalController_PitchControl(uint8_t id,
                                      float rate_normalized,
                                      SensorData *sensor_data,
                                      bool disable_yaw_pitch_compensation) {
    // 始终响应遥控器输入
    float sensitivity = 60.0f;
    c->angle_target += c->config->direction * sensitivity * rate_normalized;

    // 只在非自瞄模式下应用yaw-pitch耦合补偿
    if (!disable_yaw_pitch_compensation && yaw && yaw->angle_initialized) {
        // 计算yaw变化
        float yaw_delta = current_yaw_angle - last_yaw_angle;

        // 计算pitch补偿
        float pitch_compensation = sin(yaw_delta_rad) * tan(pitch_angle_rad);

        // 应用补偿
        c->angle_target -= pitch_compensation_ticks;
    }

    // PID计算 + 重力补偿
    // ...
}
```

#### 2. 调用方式

```c
// 在on_gimbal_cmd回调中
bool use_vision_target = s_last_cmd.vision_valid;

// pitch控制调用 - 关键：始终传递pitch_rate
int16_t pitch_current = GimbalController_PitchControl(
    s_pitch_motor_id,
    s_last_cmd.pitch_rate,  // ← 遥控器始终可以控制pitch
    &s_last_sensor,
    use_vision_target  // ← 自瞄时禁用yaw-pitch补偿
);
```

## 应用场景

### 典型使用场景

#### 场景1：地面目标自瞄
- **情况**：目标装甲板高度基本固定
- **操作**：
  1. 手动调整pitch到合适角度（如15°）
  2. 触发自瞄，yaw自动追踪
  3. 如果目标高度变化，操作员微调pitch
- **优势**：Yaw追踪稳定，pitch可灵活调整

#### 场景2：不同距离目标切换
- **情况**：目标距离变化导致pitch需要调整
- **操作**：
  1. 自瞄追踪近距离目标
  2. 切换到远距离目标时，操作员调整pitch
  3. Yaw自动切换到新目标
- **优势**：无需退出自瞄模式

#### 场景3：多层目标
- **情况**：敌方有高台和地面目标
- **操作**：
  1. Yaw自动追踪水平方向
  2. 操作员根据目标高度调整pitch
  3. 实现快速高低切换
- **优势**：操作员和视觉系统协同工作

## 优势

1. **灵活性**：操作员可随时调整pitch适应不同目标高度
2. **稳定性**：Yaw追踪不受pitch调整干扰
3. **简化逻辑**：视觉系统只需处理水平追踪
4. **避免耦合**：自瞄时消除yaw-pitch耦合效应
5. **人机协同**：发挥视觉快速追踪和人工判断的优势

## 后续扩展

如果需要让视觉系统控制pitch（适用于高度变化目标）：

1. 在`GimbalCmd`中添加`vision_pitch_control`标志
2. 修改`GimbalController_PitchControl`逻辑：
   ```c
   if (lock_pitch && !vision_pitch_control) {
       // 完全锁定
   } else if (vision_valid) {
       // 视觉控制pitch
       c->angle_target = current_angle + vision_pitch_err;
   }
   ```
3. 通过配置文件选择模式

## 固件信息

- **编译时间**：2026-01-12
- **固件大小**：117 KB
- **RAM使用**：36736 / 131072 bytes (28.03%)
- **Flash使用**：119008 / 1048576 bytes (11.35%)

## 文档

详细技术文档请参考：
- [云台耦合补偿](../guides/gimbal-compensation.md) - 当前补偿说明
- [全局架构](../architecture/overview.md) - 当前项目架构

## 问题排查

### Q1: Pitch在自瞄时还在动
**检查**：
- 确认`vision_valid`标志是否正确设置
- 查看USB CDC输出的"Pitch状态"
- 检查是否有外力干扰

### Q2: 切换模式时有跳变
**原因**：PID积分器累积
**解决**：锁定模式下已自动重置状态变量

### Q3: Pitch锁定位置不准
**原因**：重力补偿系数未校准
**解决**：调整配置文件中的`gravity_compensation`参数

## 更新日志

**2026-01-12**
- ✅ 实现pitch轴锁定功能
- ✅ 添加lock_pitch参数
- ✅ 更新状态显示
- ✅ 完善文档和测试指南

---

**作者**：Claude Code
**版本**：v1.0
**日期**：2026-01-12
