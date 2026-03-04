#ifndef CAR_CONTROL_H
#define CAR_CONTROL_H

#include <lvgl.h>
#include <C:\.platformio\packages\framework-arduinoespressif32\cores\esp32\Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

// 控件指针声明 (这些指针需要在主程序中定义并赋值)
extern lv_obj_t* speedArc;
extern lv_obj_t* speedLabel;
extern lv_obj_t* leftIndicator;
extern lv_obj_t* rightIndicator;
extern lv_obj_t* headlight;
extern lv_obj_t* stop;

// 指示灯状态变量声明
extern bool leftActive;
extern bool rightActive;
extern bool leftState;
extern bool rightState;
extern bool nearActive;
extern bool farActive;

// 初始化函数（可选，如果需要在模块内初始化一些东西）
//void car_hud_control_init(void);

// 功能函数声明
void handleSpeed(int speed);
void handleLeft(void);
void handleRight(void);
void handleOff(void);
void handleNear(void);
void handleFar(void);
void handleStart(void);
void handleStop(void);

// 闪烁处理函数（可放在 loop 中调用）
void car_hud_blink_update(void);

#ifdef __cplusplus
}
#endif

#endif // CAR_HUD_CONTROL_H