# graduation-project
# 基于 ESP32-S3 的双屏车载智能交互系统

本项目设计并实现了一套双屏无线车载智能交互系统，由主机（中控屏）和从机（仪表盘）构成。主机提供触摸交互界面，支持车辆状态控制、音乐播放、天气显示、时间同步等功能；从机模拟仪表盘，实时显示车速、转向灯、近远光灯等状态。两屏之间通过 ESP-NOW 协议实现低延迟无线指令传输。软件基于 FreeRTOS 多任务系统，将 UI 刷新、网络请求、音频播放、无线通信等任务模块化分离，确保系统实时性与稳定性。

## 硬件配置

### 主机（中控）
- **主控**：ESP32-S3-DevKitC
- **屏幕**：3.2 寸 ILI9341 SPI 触摸屏（带 FT6336U 电容触摸）
- **音频输出**：MAX98357 I2S 功放 + 扬声器
- **存储**：MicroSD 卡（与屏幕共用 SPI 总线）
- **网络**：Wi-Fi（用于时间同步和天气获取）

### 从机（仪表盘）
- **主控**：ESP32-S3-DevKitC（或同系列）
- **屏幕**：3.2 寸 ILI9341 SPI 屏幕（无触摸）
- **无线通信**：ESP-NOW（与主机配对）

### 关键引脚定义（主机）
```cpp
// 触摸屏
#define I2C_SDA 8
#define I2C_SCL 9
#define TOUCH_RST 5
#define TOUCH_INT 3

// 屏幕 SPI（与 SD 卡共用）
#define PIN_MOSI  7
#define PIN_MISO  17
#define PIN_SCLK  15
#define PIN_TFT_CS 4
#define PIN_SD_CS 9

// I2S 音频
#define I2S_BCLK  12
#define I2S_LRC   11
#define I2S_DIN   10
```
> 从机引脚与主机类似（仅无需触摸），具体可参考 `config.h`。

## 软件环境

- **开发平台**：PlatformIO（VS Code 插件）
- **板型**：`4d_systems_esp32s3_gen4_r8n16`（或 `esp32-s3-devkitc-1`）
- **框架**：Arduino
- **主要依赖库**：
  - `bodmer/TFT_eSPI` – 屏幕驱动
  - `lvgl/lvgl` – 图形库
  - `esphome/ESP32-audioI2S` – 音乐播放
  - `bblanchon/ArduinoJson` – JSON 解析
  - `schreibfaul1/ESP32-audioI2S` – 音频播放（实际使用）
  - `Wire`, `SPI`, `SD`, `WiFi`, `esp_now` – 内置库
- **图形界面设计**：SquareLine Studio 1.6.0（导出 C 代码）

## 主要功能模块

- **触摸控制**：FT6336U 驱动，坐标映射与旋转
- **双屏无线通信**：ESP-NOW 协议，自定义指令集（`commands.h`）
- **GUI**：LVGL + SquareLine Studio 设计，包含主页、音乐、控制等界面
- **音频播放**：SD 卡读取 WAV/MP3 文件，I2S 输出至 MAX98357
- **WiFi & 天气**：连接指定热点，获取高德地图实时天气和预报
- **时间同步**：NTP 网络时间，显示于 UI
- **多任务调度**：FreeRTOS 分离 UI、网络、音频、串口调试等任务

## 典型问题及解决方案

### 1. 屏幕点亮但触摸无反应
- **现象**：I2C 扫描能发现设备，但触摸回调无数据。
- **原因**：触摸芯片未正确复位。
- **解决**：在 `setup()` 中手动拉低 `TOUCH_RST` 引脚 20ms 再拉高，延时 50ms。

### 2. 触摸坐标映射不准
- **现象**：点击位置与 UI 控件不符。
- **原因**：原始坐标（0~4095）与屏幕分辨率（320x240）比例不匹配，且存在旋转。
- **解决**：
  - 先根据 `tft.setRotation()` 进行物理坐标到逻辑坐标的转换。
  - 再线性映射到屏幕像素：
    ```cpp
    data->point.x = (logic_x * TFT_HOR_RES) / 4095;
    data->point.y = (logic_y * TFT_VER_RES) / 4095;
    ```

### 3. SD 卡初始化失败（音频任务）
- **现象**：`sdcard_mount failed`，多任务环境下 SD 卡无法挂载。
- **原因**：屏幕与 SD 卡共用 SPI 总线，多任务并发导致冲突。
- **解决**：
  - 在音频任务中，操作 SD 卡前强制拉高屏幕片选 `TFT_CS`。
  - 为 SD 卡创建独立 SPI 对象并降低频率（1MHz）。
  - 增加重试机制（3~10 次）。

### 4. ESP-NOW 发送失败
- **现象**：主机发送指令后从机无反应，串口打印“发送状态: 失败”。
- **原因**：主机连接了 Wi-Fi 后信道改变，从机未同步信道。
- **解决**：
  - 主机打印当前信道 `WiFi.channel()`。
  - 从机在 `setup()` 中强制设置相同信道：
    ```cpp
    esp_wifi_set_channel(11, WIFI_SECOND_CHAN_NONE);
    ```

### 5. LVGL 跨任务操作崩溃
- **现象**：点击按钮后系统重启，报 `LoadProhibited` 错误。
- **原因**：在非 LVGL 任务（如天气任务）中直接调用了 `lv_label_set_text` 等函数。
- **解决**：所有 LVGL 操作统一放在 `lvgl_task` 中执行，其他任务通过全局标志或队列传递数据。

### 6. 音乐播放文件格式不支持
- **现象**：`readAudioHeader(): Processing stopped due to invalid audio header`。
- **原因**：WAV 文件为 24-bit，ESP32-audioI2S 仅支持 16-bit PCM。
- **解决**：使用 Audacity 将文件转换为 16-bit 44.1kHz 的 WAV 格式。

## 使用说明

1. **烧录从机**：将从机代码烧录到从机开发板，观察串口打印的 MAC 地址。
2. **修改主机 MAC 地址**：将主机代码中 `slaveMac[]` 改为从机实际 MAC。
3. **准备 SD 卡**：格式化为 FAT32，在根目录创建 `music` 文件夹，放入 16-bit 44.1kHz 的 WAV 文件。
4. **配置 Wi-Fi**：在主机代码的 `app_task` 中修改 `test_ssid` 和 `test_password` 为你的热点信息。
5. **烧录主机**：将主机代码烧录到主机开发板，打开串口监视器（115200）。
6. **操作**：通过触摸屏按钮或串口命令控制车辆状态、播放音乐、查看天气。

