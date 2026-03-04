#include <C:\.platformio\packages\framework-arduinoespressif32\cores\esp32\Arduino.h>
#include "car_control.h"
#include <Arduino.h>
#include <lvgl.h>
#include <ui/src/ui.h>   // 根据实际路径调整


// ========== 外部函数声明（需在主程序中实现） ==========
extern void updateSpeed(int speed);   // 主文件中定义的更新车速函数

// ========== 图片资源声明（来自 ui.h，请确认名称正确） ==========
extern const lv_img_dsc_t ui_img_2125878674;   // 近光灯图片
extern const lv_img_dsc_t ui_img_314426297;    // 远光灯图片

// ========== 函数实现 ==========

void car_hud_control_init(void) {
    // 需要初始化什么可以放在这里
}

void handleSpeed(int speed) {
    updateSpeed(speed);
}

void handleLeft(void) {
    leftActive = true;
    rightActive = false;
    rightState = false;
    lv_obj_add_flag(rightIndicator, LV_OBJ_FLAG_HIDDEN);
    lv_timer_handler();
    Serial.println("左转向灯开启");
}

void handleRight(void) {
    rightActive = true;
    leftActive = false;
    leftState = false;
    lv_obj_add_flag(leftIndicator, LV_OBJ_FLAG_HIDDEN);
    lv_timer_handler();
    Serial.println("右转向灯开启");
}

void handleOff(void) {
    leftActive = false;
    rightActive = false;
    leftState = false;
    rightState = false;
    nearActive = false;
    farActive = false;
    lv_obj_add_flag(leftIndicator, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(rightIndicator, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(headlight, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(stop, LV_OBJ_FLAG_HIDDEN);
    lv_refr_now(NULL);
    Serial.println("所有指示灯关闭");
}

void handleNear(void) {
    nearActive = true;
    farActive = false;
    lv_img_set_src(headlight, &ui_img_2125878674);
    lv_obj_clear_flag(headlight, LV_OBJ_FLAG_HIDDEN);
    lv_refr_now(NULL);
    Serial.println("近光灯开启");
}

void handleFar(void) {
    farActive = true;
    nearActive = false;
    lv_img_set_src(headlight, &ui_img_314426297);
    lv_obj_clear_flag(headlight, LV_OBJ_FLAG_HIDDEN);
    lv_refr_now(NULL);
    Serial.println("远光灯开启");
}

void handleStart(void) {
    updateSpeed(30);
    leftActive = false;
    rightActive = false;
    leftState = false;
    rightState = false;
    nearActive = false;
    farActive = false;
    lv_obj_add_flag(leftIndicator, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(rightIndicator, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(headlight, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(stop, LV_OBJ_FLAG_HIDDEN);
    lv_refr_now(NULL);
    Serial.println("启动完成，所有指示灯关闭");
}

void handleStop(void) {
    updateSpeed(0);
    leftActive = true;
    rightActive = true;
    leftState = false;   // 重置相位，使双闪同步开始
    rightState = false;
    nearActive = false;
    farActive = false;
    lv_obj_add_flag(headlight, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(stop, LV_OBJ_FLAG_HIDDEN);   // 点亮 stop 灯
    lv_refr_now(NULL);
    Serial.println("停止，车速0，双闪开启");
}

void car_hud_blink_update(void) {
    static unsigned long lastToggle = 0;
    if (millis() - lastToggle >= 500) {
        lastToggle = millis();

        if (leftActive) {
            leftState = !leftState;
            if (leftState) {
                lv_obj_clear_flag(leftIndicator, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(leftIndicator, LV_OBJ_FLAG_HIDDEN);
            }
            lv_obj_invalidate(leftIndicator);
        }

        if (rightActive) {
            rightState = !rightState;
            if (rightState) {
                lv_obj_clear_flag(rightIndicator, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(rightIndicator, LV_OBJ_FLAG_HIDDEN);
            }
            lv_obj_invalidate(rightIndicator);
        }

        lv_refr_now(NULL);   // 确保闪烁及时显示
    }
}