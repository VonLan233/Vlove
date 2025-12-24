# Vlove - VR手势识别手套与空气琴

<p align="center">
  <strong>基于ESP32的智能手套系统，集成手势识别与空气琴功能</strong>
</p>

---

## 目录

- [项目概述](#项目概述)
- [功能特性](#功能特性)
- [硬件要求](#硬件要求)
- [快速开始](#快速开始)
- [固件架构](#固件架构)
- [配置说明](#配置说明)
- [命令参考](#命令参考)
- [通信协议](#通信协议)
- [Python客户端](#python客户端)
- [手势定义](#手势定义)
- [故障排除](#故障排除)

---

## 项目概述

Vlove是一个开源的VR手套项目，灵感来源于[LucidGloves](https://github.com/LucidVR/lucidgloves)。项目采用模块化C++架构，支持手势识别、空气琴演奏和原始数据输出三种工作模式。

### 技术规格

| 项目 | 规格 |
|------|------|
| 主控芯片 | ESP32 DOIT V1 |
| 传感器 | 5路电位器 (线拉式) |
| 采样频率 | 100 Hz |
| ADC分辨率 | 12位 (0-4095) |
| 通信方式 | USB串口 / 蓝牙 |
| 波特率 | 115200 bps |
| 循环周期 | 10 ms |

---

## 功能特性

### 1. 手势识别模式 (G)

支持20种预定义手势的实时识别：

- **数字手势**: 0-9
- **通用手势**: 👍 👎 ✌️ 🤘 👌 ✊ 🖐️ 👆 🤙

**特点**:
- 3次去抖确认，有效避免误触发
- 响应延迟约30ms
- 基于手指开/闭/半开状态的规则匹配

### 2. 空气琴模式

三种钢琴演奏模式，音符映射到C大调音阶：

| 模式 | 名称 | 说明 |
|------|------|------|
| P1 | 单音模式 | 每根手指对应一个音符 (C4-G4)，支持力度控制 |
| P2 | 滑音模式 | 中指控制音符开关，食指控制音高弯曲 (±2半音) |
| P3 | 和弦模式 | 多根手指组合形成和弦，自动检测和弦变化 |

**MIDI音符映射**:
```
拇指 → C4 (60)
食指 → D4 (62)
中指 → E4 (64)
无名指 → F4 (65)
小指 → G4 (67)
```

### 3. 原始数据模式 (R)

输出传感器的原始值和校准后的映射值，适用于：
- 调试和故障排查
- 自定义应用开发
- 数据采集与分析

### 4. 自动校准系统

- 最小值/最大值动态捕获
- EEPROM持久化存储
- 线性映射到0-4095范围
- 异常校准警告机制

### 5. OpenGloves协议 (VR模式)

支持SteamVR集成，兼容[OpenGloves驱动](https://github.com/LucidVR/opengloves-driver)：

- Alpha编码格式输出
- 支持手指弯曲追踪
- 可选IMU姿态数据

### 6. IMU姿态追踪 (可选)

支持MPU6050 6轴IMU：

- Madgwick AHRS姿态融合算法
- 四元数输出
- 自动校准陀螺仪偏移

---

## 硬件要求

### 工作原理

采用线拉式电位器设计：

```
┌─────────────────────────────────────────────────────────┐
│                                                         │
│   指尖 ←──── 拉线 ←──── 电位器 ←──── ESP32 ADC        │
│                           │                             │
│                        旋转轴                           │
│                                                         │
└─────────────────────────────────────────────────────────┘

手指弯曲 → 拉动线 → 电位器旋转 → 阻值变化 → ADC读数变化
```

**原理说明**：
1. 线的一端固定在指尖，另一端缠绕在电位器轴上
2. 手指弯曲时拉动线，带动电位器旋转
3. 电位器输出电压随旋转角度变化
4. ESP32的ADC读取电压值，映射为手指位置 (0-4095)

### ESP32引脚配置

| 手指 | GPIO | ADC通道 | 说明 |
|------|------|---------|------|
| 拇指 (Thumb) | 36 | ADC1_0 | 仅输入 |
| 食指 (Index) | 39 | ADC1_3 | 仅输入 |
| 中指 (Middle) | 34 | ADC1_6 | 仅输入 |
| 无名指 (Ring) | 35 | ADC1_7 | 仅输入 |
| 小指 (Pinky) | 32 | ADC1_4 | 支持输入输出 |
| LED状态灯 | 2 | - | 可选 |

### 电位器接线

每个电位器有3个引脚：

```
电位器引脚:
  ┌───┐
  │ 1 │ ── VCC (3.3V)
  │ 2 │ ── 信号输出 (接ESP32 GPIO)
  │ 3 │ ── GND
  └───┘

接线示意:
  ESP32 3.3V ────┬────┬────┬────┬──── 电位器1 VCC
                 │    │    │    └──── 电位器2 VCC
                 │    │    └───────── ...
                 │    └────────────── 电位器5 VCC
                 │
  ESP32 GND ─────┴────┴────┴────┴──── 所有电位器 GND

  GPIO 36 ────────────────────────── 电位器1 信号 (拇指)
  GPIO 39 ────────────────────────── 电位器2 信号 (食指)
  GPIO 34 ────────────────────────── 电位器3 信号 (中指)
  GPIO 35 ────────────────────────── 电位器4 信号 (无名指)
  GPIO 32 ────────────────────────── 电位器5 信号 (小指)
```

### 推荐元件

| 元件 | 规格 | 数量 |
|------|------|------|
| 电位器 | 10kΩ 旋转电位器 | 5个 |
| 拉线 | 钓鱼线/编织线 0.3-0.5mm | 适量 |
| ESP32 | DOIT DevKit V1 | 1个 |
| MPU6050 | 6轴IMU模块 (可选) | 1个 |

> **注意**: ESP32的ADC输入范围为0-3.3V，电位器VCC必须接3.3V而非5V。

### IMU接线 (可选)

如需手部姿态追踪功能，添加MPU6050模块：

```
MPU6050          ESP32
  VCC     →     3.3V
  GND     →     GND
  SCL     →     GPIO 22
  SDA     →     GPIO 21
```

> **提示**: 如不需要IMU功能，可在 `Config.h` 中注释掉 `#define ENABLE_IMU`

---

## 快速开始

### 1. 安装依赖

**Arduino IDE配置**:
1. 安装 [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32)
2. 选择开发板: `ESP32 Dev Module`
3. 选择端口

### 2. 上传固件

```bash
# 项目结构
Vlove/
├── firmware/
│   ├── Vlove.ino          # 主程序入口
│   └── src/               # 模块源码
└── python/                # Python客户端
```

1. 打开 `firmware/Vlove.ino`
2. 选择开发板和端口
3. 点击上传

### 3. 首次校准

首次启动会自动进入校准模式：

```
****************************************
*       CALIBRATION MODE ACTIVE        *
****************************************

Move ALL fingers through full range:
  1. Make a tight fist
  2. Open hand fully
  3. Repeat several times

Type 'DONE' or press ENTER when finished.
```

**校准步骤**:
1. 完全握拳 → 完全张开手掌
2. 重复3-5次，确保覆盖完整活动范围
3. 输入 `DONE` 或按回车完成校准

### 4. 开始使用

校准完成后自动进入手势识别模式：

```
========================================
  Vlove Ready - Gesture Mode
========================================
Commands: G/P1/P2/P3/R | CAL | BT | HELP
```

---

## 固件架构

### 模块结构

```
firmware/
├── Vlove.ino                 # 主程序
│   ├── setup()               # 初始化
│   ├── loop()                # 主循环
│   ├── handleCommands()      # 命令处理
│   └── processXxxMode()      # 模式处理函数
│
└── src/
    ├── Config.h              # 配置定义
    │   ├── 引脚配置
    │   ├── 阈值定义
    │   ├── 枚举类型
    │   └── MIDI音符
    │
    ├── Calibration.h         # 校准模块
    │   ├── begin()           # 初始化EEPROM
    │   ├── startCalibration()
    │   ├── update()          # 更新min/max
    │   ├── stopCalibration()
    │   ├── mapValue()        # 值映射
    │   └── saveToEEPROM()    # 持久化
    │
    ├── GestureRecognizer.h   # 手势识别
    │   ├── update()          # 更新识别
    │   ├── getCurrentGesture()
    │   └── getGestureName()
    │
    ├── AirPiano.h            # 空气琴
    │   ├── update()          # 处理输入
    │   ├── getEvent()        # 获取事件
    │   └── setMode()         # 设置模式
    │
    └── Communication.h       # 通信模块
        ├── begin()           # 初始化
        ├── toggleBluetooth()
        ├── sendGesture()
        ├── sendPianoEvent()
        └── sendRawData()
```

### 工作流程

```
┌─────────────────────────────────────────────────────┐
│                      启动                           │
└─────────────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────┐
│  初始化串口 → 加载EEPROM → 检查校准数据             │
└─────────────────────────────────────────────────────┘
                         │
           ┌─────────────┴─────────────┐
           │ 有效校准?                 │
           ▼                           ▼
     ┌──────────┐               ┌──────────┐
     │ 正常启动 │               │ 校准模式 │
     └──────────┘               └──────────┘
           │                           │
           └─────────────┬─────────────┘
                         ▼
┌─────────────────────────────────────────────────────┐
│                   主循环 (10ms)                     │
│  ┌─────────────────────────────────────────────┐    │
│  │ 1. 读取串口命令                             │    │
│  │ 2. 读取5路ADC传感器                         │    │
│  │ 3. 应用校准映射                             │    │
│  │ 4. 根据模式处理数据                         │    │
│  │ 5. 发送输出                                 │    │
│  └─────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────┘
```

---

## 配置说明

所有配置位于 `src/Config.h`：

### 引脚配置

```cpp
#define PIN_THUMB   36
#define PIN_INDEX   39
#define PIN_MIDDLE  34
#define PIN_RING    35
#define PIN_PINKY   32
#define PIN_LED     2
```

### 阈值配置

```cpp
#define FINGER_OPEN_THRESHOLD    3000  // 手指打开阈值
#define FINGER_CLOSED_THRESHOLD  1000  // 手指闭合阈值
#define FINGER_HALF_THRESHOLD    2000  // 手指半开阈值
#define GESTURE_DEBOUNCE_COUNT   3     // 手势去抖次数
```

### 钢琴阈值

```cpp
#define NOTE_ON_THRESHOLD   1500  // 音符触发阈值
#define NOTE_OFF_THRESHOLD  2500  // 音符释放阈值
```

### 系统配置

```cpp
#define BAUD_RATE      115200
#define LOOP_DELAY_MS  10
#define ANALOG_MAX     4095
#define EEPROM_SIZE    512
```

---

## 命令参考

通过串口发送命令控制设备：

### 模式切换

| 命令 | 别名 | 功能 |
|------|------|------|
| `G` | `GESTURE` | 切换到手势识别模式 |
| `P1` | `PIANO1` | 切换到单音模式 |
| `P2` | `PIANO2` | 切换到滑音模式 |
| `P3` | `PIANO3` | 切换到和弦模式 |
| `R` | `RAW` | 切换到原始数据模式 |
| `VR` | `OPENGLOVES` / `OG` | 切换到OpenGloves模式 (SteamVR) |

### 校准控制

| 命令 | 功能 |
|------|------|
| `CAL` | 启动校准模式 |
| `DONE` / `STOP` | 完成校准 |
| `CLEAR` | 清除EEPROM并重新校准 |

### 硬件控制

| 命令 | 功能 |
|------|------|
| `BT` | 开启/关闭蓝牙 |
| `IMU` | 显示当前IMU姿态数据 |
| `IMUCAL` | 校准IMU陀螺仪 |
| `HELP` / `H` / `?` | 显示帮助信息 |

---

## 通信协议

### 手势数据帧

```
G,<gesture_id>,<gesture_name>
```

**示例**:
```
G,1,One        # 数字1
G,13,Peace     # ✌️ 手势
G,11,ThumbsUp  # 👍 手势
```

### 钢琴数据帧

**单音模式**:
```
P,ON,<note>,<velocity>    # 音符开始
P,OFF,<note>              # 音符结束
```

**滑音模式**:
```
P,BEND,<note>,<pitch_bend>  # pitch_bend: -8192 ~ 8191
```

**和弦模式**:
```
P,ON,CHORD,<note1>,<note2>,...
P,OFF,CHORD,<note1>,<note2>,...
```

**示例**:
```
P,ON,64,100           # E4音符开始，力度100
P,OFF,64              # E4音符结束
P,BEND,64,4096        # E4音高上弯
P,ON,CHORD,60,64,67   # C大三和弦开始
P,OFF,CHORD,60,64,67  # C大三和弦结束
```

### 原始数据帧

```
R,<raw0>,<raw1>,<raw2>,<raw3>,<raw4>,<map0>,<map1>,<map2>,<map3>,<map4>
```

**示例**:
```
R,1024,2048,3072,1500,2500,512,1024,2048,750,1500
```

### OpenGloves数据帧 (VR模式)

兼容OpenGloves驱动的Alpha编码格式：

**仅手指数据**:
```
A<thumb>B<index>C<middle>D<ring>E<pinky>
```

**包含IMU四元数**:
```
A<thumb>B<index>C<middle>D<ring>E<pinky>(qw|qx|qy|qz)
```

**示例**:
```
A2048B3000C3500D2800E2500                    # 仅手指
A2048B3000C3500D2800E2500(0.9239|0.0|0.3827|0.0)  # 含IMU
```

---

## SteamVR集成

### 1. 安装OpenGloves驱动

1. 下载 [OpenGloves Driver](https://github.com/LucidVR/opengloves-driver/releases)
2. 解压到SteamVR的 `drivers` 目录
3. 启动SteamVR

### 2. 配置驱动

编辑 `opengloves/resources/settings/default.vrsettings`：

```json
{
  "communication_protocol": "serial",
  "serial_port": "COM3",  // 或 /dev/ttyUSB0
  "encoding_protocol": "alpha"
}
```

### 3. 启动手套

```bash
# ESP32切换到VR模式
VR
# 或
OPENGLOVES
```

SteamVR应自动检测并显示手部模型。

---

## Python客户端

### 安装依赖

```bash
pip install pyserial numpy sounddevice
```

### 使用方法

```bash
cd Vlove/python

# 自动检测串口
python vlove_client.py

# 指定串口
python vlove_client.py /dev/cu.usbserial-110   # macOS
python vlove_client.py COM3                     # Windows
python vlove_client.py /dev/ttyUSB0             # Linux
```

### 客户端功能

- 自动检测ESP32串口 (CH340/CP210x/usbserial)
- 解析手势、钢琴、原始数据
- 实时音频反馈 (正弦波合成)
- 支持MIDI音符播放和音高弯曲

### 模块说明

| 文件 | 功能 |
|------|------|
| `vlove_client.py` | 主程序，串口通信和数据解析 |
| `audio_player.py` | 音频引擎，波形合成和播放 |

---

## 手势定义

### 数字手势 (0-9)

| 手势 | 拇指 | 食指 | 中指 | 无名指 | 小指 |
|------|:----:|:----:|:----:|:------:|:----:|
| 0 / ✊ | 关 | 关 | 关 | 关 | 关 |
| 1 | 关 | 开 | 关 | 关 | 关 |
| 2 / ✌️ | 关 | 开 | 开 | 关 | 关 |
| 3 | 关 | 开 | 开 | 开 | 关 |
| 4 | 关 | 开 | 开 | 开 | 开 |
| 5 / 🖐️ | 开 | 开 | 开 | 开 | 开 |
| 6 / 🤙 | 开 | 关 | 关 | 关 | 开 |
| 7 | 开 | 开 | 开 | 关 | 关 |
| 8 | 开 | 开 | 开 | 开 | 关 |
| 9 / 👍 | 开 | 关 | 关 | 关 | 关 |

### 特殊手势

| 手势 | 拇指 | 食指 | 中指 | 无名指 | 小指 | 说明 |
|------|:----:|:----:|:----:|:------:|:----:|------|
| 🤘 Rock | 关 | 开 | 关 | 关 | 开 | 摇滚手势 |
| 👌 OK | 半 | 半 | 开 | 开 | 开 | OK手势 |
| 👆 Point | 半 | 开 | 关 | 关 | 关 | 指向手势 |

> **图例**: 开 = 手指伸直, 关 = 手指弯曲, 半 = 手指半弯

---

## 故障排除

### 常见问题

**Q: 上传固件失败**
- 检查是否选择正确的开发板 (ESP32 Dev Module)
- 尝试按住BOOT按钮后再上传
- 检查USB线是否支持数据传输

**Q: 串口无输出**
- 确认波特率设置为115200
- 检查串口监视器的换行设置
- 尝试重启ESP32

**Q: 校准后手势识别不准确**
- 重新执行校准，确保完整活动范围
- 检查传感器接线是否松动
- 使用 `R` 命令查看原始数据是否正常

**Q: 蓝牙无法连接**
- 确认已执行 `BT` 命令开启蓝牙
- 设备名称为 "Vlove"
- 检查手机/电脑蓝牙是否开启

**Q: Python客户端找不到串口**
- 手动指定串口路径
- 检查串口驱动是否安装 (CH340/CP210x)
- 确认ESP32已通过USB连接

### 调试技巧

1. **使用原始数据模式**: 发送 `R` 命令查看传感器原始值
2. **检查校准范围**: 校准后的min/max值应有足够差值 (>500)
3. **查看串口输出**: 系统启动时会输出校准状态信息

---

## 项目结构

```
Vlove/
├── firmware/                    # ESP32固件
│   ├── Vlove.ino               # 主程序
│   └── src/
│       ├── Config.h            # 配置文件
│       ├── Calibration.h       # 校准模块
│       ├── GestureRecognizer.h # 手势识别
│       ├── AirPiano.h          # 空气琴
│       ├── Communication.h     # 通信模块 (含OpenGloves)
│       └── IMU.h               # IMU模块 (MPU6050)
│
├── python/                      # Python客户端
│   ├── vlove_client.py         # 主程序
│   └── audio_player.py         # 音频播放
│
└── README.md                    # 本文档
```

---

## 未来计划

- [x] **OpenGloves协议** - 集成SteamVR支持 ✅ 已完成
- [x] **IMU姿态追踪** - 添加MPU6050实现手部旋转追踪 ✅ 已完成
- [ ] **机器学习手势识别** - 用KNN/决策树替代硬编码规则，支持自定义手势训练
- [x] **力反馈** - 舵机驱动的触觉反馈
- [x] **3D打印外壳** - 完整的硬件设计文件

---

## 致谢

本项目参考了 [LucidGloves](https://github.com/LucidVR/lucidgloves) 的设计思路和架构。

---

## 许可证

MIT License
