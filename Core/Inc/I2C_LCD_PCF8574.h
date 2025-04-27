#ifndef I2C_LCD_PCF8574_H

#define I2C_LCD_PCF8574_H

#include "stm32f0xx_hal.h"

void LCD_Init(void);
void LCD_Enable(void);
void LCD_Start(void);
void LCD_Stop(void);
void LCD_WriteControl(uint8_t control_byte);
void LCD_WriteData(uint8_t data_byte);
void LCD_PrintString(char const string[]);
void LCD_Position(uint8_t row, uint8_t col);
void LCD_PutChar(char output_char);
void LCD_WrDataNyb(uint8_t data_nybble);
void LCD_WrCtrlNyb(uint8_t control_nybble);

#define LCD_CLEAR_DISPLAY_CMD (0x01u)
#define LCD_DISPLAY_4BIT_CMD (0x02u)
#define LCD_MOVE_CURSOR_HOME_CMD (0x02u)
#define LCD_DISPLAY_8BIT_CMD (0x03u)
#define LCD_RESET_CURSOR_POSITION_CMD (0x03u)
#define LCD_MOVE_CURSOR_LEFT_CMD (0x04u)
#define LCD_MOVE_CURSOR_RIGHT_CMD (0x06u)
#define LCD_CURSOR_AUTO_INCREMENT_CMD (0x06u)
#define LCD_TURN_OFF_DISPLAY_AND_CURSOR_CMD (0x08u)
#define LCD_TURN_ON_DISPLAY_BUT_CURSOR_OFF_CMD (0x0Cu)
#define LCD_WINK_CURSOR_CMD (0x0Du)
#define LCD_TURN_ON_DISPLAY_AND_CURSOR_CMD (0x0Eu)
#define LCD_BLINK_CURSOR_CMD (0x0Fu)
#define LCD_SHIFT_CURSOR_LEFT_CMD (0x10u)
#define LCD_SHIFT_CURSOR_RIGHT_CMD (0x14u)
#define LCD_SCROLL_DISPLAY_LEFT_CMD (0x18u)
#define LCD_SCROLL_DISPLAY_RIGHT_CMD (0x1Eu)
#define LCD_MODE_2_ROWS_5_BY_10_CMD (0x2Cu)

#define LCD_ROW_0_ST (0x80u)
#define LCD_ROW_1_ST (0xC0u)
#define LCD_ROW_2_ST (0x94u)
#define LCD_ROW_3_ST (0xD4u)

#define LCD_LONGEST_CMD_US (0x651u)
#define LCD_WAIT_CYCLE (0x10u)
#define LCD_READY_DELAY ((LCD_LONGEST_CMD_US * 4u) / (LCD_WAIT_CYCLE))

#define LCD_ClearDisplay() LCD_WriteControl(LCD_CLEAR_DISPLAY_CMD);

#define RS_MASK (0x01u);
#define RW_MASK (0x02u);
#define E_MASK (0x04u);
#define BL_MASK (0x08u);
#define DATA_MASK = (0xF0u);

#endif
