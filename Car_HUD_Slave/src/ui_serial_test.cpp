#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <ui/src/ui.h>   // 根据实际路径调整

// ========== ESP-NOW 相关头文件（已注释，需要时取消） ==========
// #include <WiFi.h>
// #include <esp_now.h>

// 屏幕尺寸（必须与 TFT_eSPI 配置一致）
#define TFT_HOR_RES  320
#define TFT_VER_RES  240

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[TFT_HOR_RES * 10];
TFT_eSPI tft = TFT_eSPI();

// UI 控件指针
static lv_obj_t* speedArc = NULL;
static lv_obj_t* speedLabel = NULL;
static lv_obj_t* leftIndicator = NULL;
static lv_obj_t* rightIndicator = NULL;

// 指示灯状态
bool leftActive = false;
bool rightActive = false;
bool leftState = false;
bool rightState = false;
unsigned long lastToggle = 0;

// ========== ESP-NOW 全局变量（已注释） ==========
/*
// 指令码定义（需与主机一致）
typedef enum {
    CMD_NONE = 0,
    CMD_ACCELERATE = 1,
    CMD_DECELERATE = 2,
    CMD_LEFT_TURN_ON = 3,
    CMD_LEFT_TURN_OFF = 4,
    CMD_RIGHT_TURN_ON = 5,
    CMD_RIGHT_TURN_OFF = 6,
    CMD_HAZARD_ON = 7,
    CMD_HAZARD_OFF = 8,
    CMD_VOLUME_UP = 9,
    CMD_VOLUME_DOWN = 10,
    CMD_SPEED_SET = 11   // 直接设置车速，携带数值
} CarCommand;

// 接收到的指令和参数
volatile CarCommand receivedCmd = CMD_NONE;
volatile int receivedParam = 0;  // 用于携带车速值

// ESP-NOW 接收回调
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
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
*/

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

// 空触摸回调
void my_touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    data->state = LV_INDEV_STATE_REL;
}

// 更新车速
void updateSpeed(int speed) {
    if (speed < 0) speed = 0;
    if (speed > 180) speed = 180;
    lv_arc_set_value(speedArc, speed);
    char buf[10];
    sprintf(buf, "%d", speed);
    lv_label_set_text(speedLabel, buf);
    lv_obj_invalidate(speedArc);
    lv_obj_invalidate(speedLabel);
    
    lv_refr_now(NULL);   // 强制立即刷新
    Serial.printf("updateSpeed(%d) 调用, 弧值=%d, 标签=%s\n", 
                  speed, lv_arc_get_value(speedArc), lv_label_get_text(speedLabel));
}

// 切换指示灯
void toggleIndicator(lv_obj_t* ind, bool* state) {
    *state = !*state;
    if (*state) {
        lv_obj_clear_flag(ind, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(ind, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_invalidate(ind);
    lv_refr_now(NULL);               // 强制立即刷新
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("===== 精简测试版 (ESP-NOW 已注释) =====");

    // ========== ESP-NOW 初始化（需要时取消注释） ==========
    /*
    WiFi.mode(WIFI_STA);
    Serial.print("副屏 MAC 地址: ");
    Serial.println(WiFi.macAddress());

    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW 初始化失败");
        return;
    }
    esp_now_register_recv_cb(onDataRecv);
    */

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

    // 获取控件指针
    speedArc = ui_Arc1;
    speedLabel = ui_MABIAO;
    leftIndicator = ui_Image8;
    rightIndicator = ui_Image7;

    // 打印指针
    Serial.printf("speedArc: %p\n", speedArc);
    Serial.printf("speedLabel: %p\n", speedLabel);
    Serial.printf("leftIndicator: %p\n", leftIndicator);

    // 初始显示车速 90
    updateSpeed(90);

    Serial.println("就绪，输入命令：SPEED:数值  LEFT  RIGHT  OFF");

    // 打印指示灯位置和尺寸
    Serial.printf("leftIndicator: pos=(%d,%d), size=%dx%d\n", 
              lv_obj_get_x(leftIndicator), lv_obj_get_y(leftIndicator),
              lv_obj_get_width(leftIndicator), lv_obj_get_height(leftIndicator));
    Serial.printf("rightIndicator: pos=(%d,%d), size=%dx%d\n", 
              lv_obj_get_x(rightIndicator), lv_obj_get_y(rightIndicator),
              lv_obj_get_width(rightIndicator), lv_obj_get_height(rightIndicator));

    // 强制设置指示灯背景色（便于观察）
//     lv_obj_set_style_bg_color(leftIndicator, lv_color_hex(0xFF0000), 0);
//     lv_obj_set_style_bg_opa(leftIndicator, LV_OPA_COVER, 0);
//     lv_obj_set_style_bg_color(rightIndicator, lv_color_hex(0x00FF00), 0);
//     lv_obj_set_style_bg_opa(rightIndicator, LV_OPA_COVER, 0);
 }

void loop() {
    // ========== 处理 ESP-NOW 命令（需要时取消注释） ==========
    /*
    if (receivedCmd != CMD_NONE) {
        CarCommand cmd = receivedCmd;
        int param = receivedParam;
        receivedCmd = CMD_NONE;  // 清空，避免重复处理

        switch (cmd) {
            case CMD_LEFT_TURN_ON:
                leftActive = true;
                rightActive = false;
                rightState = false;
                lv_obj_add_flag(rightIndicator, LV_OBJ_FLAG_HIDDEN);
                Serial.println("左转向灯开启");
                break;
            case CMD_LEFT_TURN_OFF:
                leftActive = false;
                leftState = false;
                lv_obj_add_flag(leftIndicator, LV_OBJ_FLAG_HIDDEN);
                Serial.println("左转向灯关闭");
                break;
            case CMD_RIGHT_TURN_ON:
                rightActive = true;
                leftActive = false;
                leftState = false;
                lv_obj_add_flag(leftIndicator, LV_OBJ_FLAG_HIDDEN);
                Serial.println("右转向灯开启");
                break;
            case CMD_RIGHT_TURN_OFF:
                rightActive = false;
                rightState = false;
                lv_obj_add_flag(rightIndicator, LV_OBJ_FLAG_HIDDEN);
                Serial.println("右转向灯关闭");
                break;
            case CMD_HAZARD_ON:
                leftActive = true;
                rightActive = true;
                Serial.println("双闪开启");
                break;
            case CMD_HAZARD_OFF:
                leftActive = false;
                rightActive = false;
                leftState = false;
                rightState = false;
                lv_obj_add_flag(leftIndicator, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(rightIndicator, LV_OBJ_FLAG_HIDDEN);
                Serial.println("双闪关闭");
                break;
            case CMD_SPEED_SET:
                updateSpeed(param);
                break;
            default:
                Serial.println("未知命令");
                break;
        }
    }
    */

    // 处理串口命令（当前主要调试方式）
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        if (cmd.length() == 0) return;

        Serial.print("收到命令: ");
        Serial.println(cmd);

        if (cmd.startsWith("SPEED:")) {
            int speed = cmd.substring(6).toInt();
            updateSpeed(speed);
        }
        else if (cmd.equals("LEFT")) {
            leftActive = true;
            rightActive = false;
            rightState = false;
            lv_obj_add_flag(rightIndicator, LV_OBJ_FLAG_HIDDEN);
            lv_timer_handler();
            Serial.println("左转向灯开启");
        }
        else if (cmd.equals("RIGHT")) {
            rightActive = true;
            leftActive = false;
            leftState = false;
            lv_obj_add_flag(leftIndicator, LV_OBJ_FLAG_HIDDEN);
            lv_timer_handler();
            Serial.println("右转向灯开启");
        }
        else if (cmd.equals("OFF")) {
            leftActive = false;
            rightActive = false;
            leftState = false;
            rightState = false;
            lv_obj_add_flag(leftIndicator, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(rightIndicator, LV_OBJ_FLAG_HIDDEN);
            lv_refr_now(NULL);   // 强制刷新，确保立即隐藏
            Serial.println("所有指示灯关闭");
        }
        else {
            Serial.println("未知命令");
        }
    }

    // 指示灯闪烁
    if (millis() - lastToggle >= 500) {
        lastToggle = millis();
        Serial.printf("闪烁: leftActive=%d, leftState=%d\n", leftActive, leftState);
        if (leftActive) toggleIndicator(leftIndicator, &leftState);
        if (rightActive) toggleIndicator(rightIndicator, &rightState);
    }

    lv_timer_handler();
    delay(5);
}