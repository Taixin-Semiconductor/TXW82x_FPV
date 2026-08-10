/****************************************************************************
 * 文件名：keyboard_input_ui.h
 *
 * 键盘输入页面头文件
 * 提供外部调用接口
 ****************************************************************************/

#ifndef KEYBOARD_INPUT_UI_H
#define KEYBOARD_INPUT_UI_H

#include "lvgl/lvgl.h"

/* 前向声明 */
typedef struct keyboard_input_ctx_t keyboard_input_ctx_t;

/****************************************************************************
 * 外部入口：创建并压栈键盘输入页面
 * 用法：
 *   keyboard_input_ui_create("输入密码", 8, on_confirm_callback, on_cancel_callback, user_data);
 *
 * 参数：
 *   title      - 页面标题字符串
 *   min_length - 最小输入长度（字节），小于该长度时确认按钮禁用
 *   on_confirm - 确认回调函数，参数为用户数据和输入的文本
 *   on_cancel  - 取消回调函数（点击返回按钮时调用）
 *   user_data  - 用户数据指针，传递给回调函数
 ****************************************************************************/
void keyboard_input_ui_create(const char *title, uint8_t min_length, void (*on_confirm)(void *user_data, const char *text), void (*on_cancel)(void *user_data), void *user_data);

/****************************************************************************
 * 获取输入框中的文本
 * 参数：
 *   ui_s - 键盘输入页面上下文（从 on_confirm 回调中获取）
 * 返回：输入的文本字符串
 ****************************************************************************/
const char *keyboard_input_get_text(keyboard_input_ctx_t *ui_s);

#endif /* KEYBOARD_INPUT_UI_H */
