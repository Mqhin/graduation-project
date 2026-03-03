#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <ui\src\ui.h>

// 屏幕尺寸（应与你的 TFT_eSPI 配置一致）
#define TFT_HOR_RES  320
#define TFT_VER_RES  240

// LVGL 显示缓冲区
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[TFT_HOR_RES * 10];  // 缓冲区大小：10行像素

// 指向 TFT_eSPI 对象的指针（全局）
TFT_eSPI tft = TFT_eSPI();

// 显示刷新回调（LVGL 调用此函数将数据发送到屏幕）
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t*)color_p, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

// 触摸读取回调（本测试不使用触摸，但必须提供一个空函数）
void my_touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    data->state = LV_INDEV_STATE_REL;
}

// 按钮事件回调
static void btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *label = (lv_obj_t*)lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED) {
        static int cnt = 0;
        cnt++;
        lv_label_set_text_fmt(label, "Button clicked %d times", cnt);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("LVGL Test Start");

    // 初始化 TFT 屏幕
    tft.init();
    tft.setRotation(1);  // 根据你的屏幕方向调整
    tft.fillScreen(TFT_BLACK);

    // 初始化 LVGL
    lv_init();

    // 初始化显示缓冲区
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, TFT_HOR_RES * 10);

    // 注册显示驱动
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = TFT_HOR_RES;
    disp_drv.ver_res = TFT_VER_RES;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // 注册输入设备驱动（这里用空触摸驱动）
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touch_read;
    lv_indev_drv_register(&indev_drv);

    // 创建简单 UI
    // 1. 创建一个屏幕（如果不显式创建，LVGL 会使用默认屏幕）
    ui_init();

    Serial.println("UI created");
}

void loop() {
    lv_timer_handler();  // 处理 LVGL 任务
    delay(5);            // 适当延时，避免 CPU 占用过高
}