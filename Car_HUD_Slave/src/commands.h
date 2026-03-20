// commands.h
#ifndef COMMANDS_H
#define COMMANDS_H

// ESP-NOW 指令码定义
// commands.h（在原有基础上增加两行）
typedef enum {
    CMD_NONE = 0,
    CMD_SPEED_SET = 1,
    CMD_LEFT_TURN_ON = 2,
    CMD_LEFT_TURN_OFF = 3,
    CMD_RIGHT_TURN_ON = 4,
    CMD_RIGHT_TURN_OFF = 5,
    CMD_HAZARD_ON = 6,
    CMD_HAZARD_OFF = 7,
    CMD_NEAR_LIGHT_ON = 8,
    CMD_FAR_LIGHT_ON = 9,
    CMD_ALL_OFF = 10,
    CMD_START = 11,          // 新增
    CMD_STOP = 12             // 新增
} CarCommand;

#endif