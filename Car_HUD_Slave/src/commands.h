// commands.h
#ifndef COMMANDS_H
#define COMMANDS_H

// ESP-NOW 指令码定义
typedef enum {
    CMD_NONE = 0,
    CMD_SPEED_SET = 1,        // 设置车速，后跟 int 参数
    CMD_LEFT_TURN_ON = 2,
    CMD_LEFT_TURN_OFF = 3,
    CMD_RIGHT_TURN_ON = 4,
    CMD_RIGHT_TURN_OFF = 5,
    CMD_HAZARD_ON = 6,
    CMD_HAZARD_OFF = 7,
    CMD_NEAR_LIGHT_ON = 8,
    CMD_FAR_LIGHT_ON = 9,
    CMD_ALL_OFF = 10          // 关闭所有灯
} CarCommand;

#endif