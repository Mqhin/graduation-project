#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <ui/src/ui.h>          // SquareLine Studio 导出的 UI
#include "car_control.h"         // 控制模块
#include <WiFi.h>
#include <esp_now.h>
#include "commands.h"            // 公共指令定义
#include <esp_wifi.h>            // 添加此头文件以使用 esp_wifi_set_channel

// 屏幕尺寸（必须与 TFT_eSPI 配置一致）
#define TFT_HOR_RES  320
#define TFT_VER_RES  240

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[TFT_HOR_RES * 10];
TFT_eSPI tft = TFT_eSPI();

// UI 控件指针（供 car_control 使用）
lv_obj_t* speedArc = NULL;
lv_obj_t* speedLabel = NULL;
lv_obj_t* leftIndicator = NULL;
lv_obj_t* rightIndicator = NULL;
lv_obj_t* headlight = NULL;
lv_obj_t* stop = NULL;

// 指示灯状态（供 car_control 使用）
bool leftActive = false;
bool rightActive = false;
bool leftState = false;
bool rightState = false;
bool nearActive = false;
bool farActive = false;

// ESP-NOW 接收变量
volatile CarCommand receivedCmd = CMD_NONE;
volatile int receivedParam = 0;

// 刷新回调
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    Serial.printf("刷新区域: (%d,%d)-(%d,%d)\n", area->x1, area->y1, area->x2, area->y2);
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t*)color_p, w * h, true);
    tft.endWrite();
    lv_disp_flush_ready(disp);
}

// 空触摸回调（副屏无触摸）
void my_touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    data->state = LV_INDEV_STATE_REL;
}

// 更新车速（供 car_control 调用）
void updateSpeed(int speed) {
    if (speed < 0) speed = 0;
    if (speed > 180) speed = 180;
    lv_arc_set_value(speedArc, speed);
    char buf[10];
    sprintf(buf, "%d", speed);
    lv_label_set_text(speedLabel, buf);
    lv_obj_invalidate(speedArc);
    lv_obj_invalidate(speedLabel);
    lv_refr_now(NULL);
    Serial.printf("updateSpeed(%d) 调用\n", speed);
}

// ESP-NOW 接收回调（仅保存数据，不处理 UI）
void onDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
    if (len >= sizeof(CarCommand)) {
        CarCommand cmd;
        memcpy(&cmd, data, sizeof(cmd));
        int param = 0;
        if (len == sizeof(CarCommand) + sizeof(int)) {
            memcpy(&param, data + sizeof(cmd), sizeof(int));
        }
        receivedCmd = cmd;
        receivedParam = param;
        Serial.printf("收到命令: %d, 参数: %d\n", cmd, param);
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("===== 副屏 ESP-NOW 版本 =====");

    // 初始化 WiFi 为 STA 模式（ESP-NOW 需要）
    WiFi.mode(WIFI_STA);
    Serial.print("副屏 MAC 地址: ");
    Serial.println(WiFi.macAddress());

    // 强制设置信道与主机一致（根据主机日志，信道为11）
    esp_wifi_set_channel(11, WIFI_SECOND_CHAN_NONE);
    Serial.println("从机已设置信道为11");

    // 初始化 ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW 初始化失败");
        return;
    }
    esp_now_register_recv_cb(onDataRecv);

    // 屏幕初始化
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    // LVGL 初始化
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, TFT_HOR_RES * 10);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = TFT_HOR_RES;
    disp_drv.ver_res = TFT_VER_RES;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touch_read;
    lv_indev_drv_register(&indev_drv);

    ui_init();  // 创建 UI

    // 获取控件指针（请根据实际命名修改）
    speedArc = ui_Arc1;
    speedLabel = ui_MABIAO;
    leftIndicator = ui_Image8;
    rightIndicator = ui_Image7;
    headlight = ui_Image9;
    stop = ui_Image10;

    // 初始隐藏所有指示灯
    // lv_obj_add_flag(leftIndicator, LV_OBJ_FLAG_HIDDEN);
    // lv_obj_add_flag(rightIndicator, LV_OBJ_FLAG_HIDDEN);
    // lv_obj_add_flag(headlight, LV_OBJ_FLAG_HIDDEN);
    // lv_obj_add_flag(stop, LV_OBJ_FLAG_HIDDEN);

    // 初始显示车速 0
    updateSpeed(0);

    Serial.println("副屏就绪，等待主机指令...");
    Serial.println("同时可通过串口发送命令调试：SPEED:数值 LEFT RIGHT OFF NEAR FAR START STOP");
}

void loop() {
    // ========== 处理 ESP-NOW 命令 ==========
    if (receivedCmd != CMD_NONE) {
        CarCommand cmd = receivedCmd;
        int param = receivedParam;
        receivedCmd = CMD_NONE;   // 清空标志

        switch (cmd) {
            case CMD_SPEED_SET:
                handleSpeed(param);
                break;
            case CMD_LEFT_TURN_ON:
                handleLeft();
                break;
            case CMD_LEFT_TURN_OFF:
                leftActive = false;
                leftState = false;
                lv_obj_add_flag(leftIndicator, LV_OBJ_FLAG_HIDDEN);
                lv_refr_now(NULL);
                Serial.println("左转向灯关闭");
                break;
            case CMD_RIGHT_TURN_ON:
                handleRight();
                break;
            case CMD_RIGHT_TURN_OFF:
                rightActive = false;
                rightState = false;
                lv_obj_add_flag(rightIndicator, LV_OBJ_FLAG_HIDDEN);
                lv_refr_now(NULL);
                Serial.println("右转向灯关闭");
                break;
            case CMD_HAZARD_ON:
                // 双闪开启：左右同时激活
                leftActive = true;
                rightActive = true;
                // 确保可见（闪烁由 car_hud_blink_update 处理）
                lv_obj_clear_flag(leftIndicator, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(rightIndicator, LV_OBJ_FLAG_HIDDEN);
                lv_refr_now(NULL);
                Serial.println("双闪开启");
                break;
            case CMD_HAZARD_OFF:
                leftActive = false;
                rightActive = false;
                leftState = false;
                rightState = false;
                lv_obj_add_flag(leftIndicator, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(rightIndicator, LV_OBJ_FLAG_HIDDEN);
                lv_refr_now(NULL);
                Serial.println("双闪关闭");
                break;
            case CMD_NEAR_LIGHT_ON:
                handleNear();
                break;
            case CMD_FAR_LIGHT_ON:
                handleFar();
                break;
            case CMD_ALL_OFF:
                handleOff();
                break;
            case CMD_START:
                handleStart();
                break;
            case CMD_STOP:
                handleStop();
                break;    
            default:
                Serial.println("未知 ESP-NOW 命令");
                break;
        }
    }

    // ========== 处理串口命令（调试用） ==========
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        if (cmd.length() == 0) return;

        Serial.print("收到串口命令: ");
        Serial.println(cmd);

        if (cmd.startsWith("SPEED:")) {
            int speed = cmd.substring(6).toInt();
            handleSpeed(speed);
        }
        else if (cmd.equals("LEFT")) {
            handleLeft();
        }
        else if (cmd.equals("RIGHT")) {
            handleRight();
        }
        else if (cmd.equals("OFF")) {
            handleOff();
        }
        else if (cmd.equals("NEAR")) {
            handleNear();
        }
        else if (cmd.equals("FAR")) {
            handleFar();
        }
        else if (cmd.equals("START")) {
            handleStart();
        }
        else if (cmd.equals("STOP")) {
            handleStop();
        }
        else {
            Serial.println("未知串口命令");
        }
    }

    // 指示灯闪烁（由 car_control 模块处理）
    car_hud_blink_update();

    lv_timer_handler();
    delay(5);
}