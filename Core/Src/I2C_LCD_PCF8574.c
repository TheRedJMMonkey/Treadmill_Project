#include "I2C_LCD_PCF8574.h"

extern I2C_HandleTypeDef hi2c1;

static const uint16_t lcd_addr = (0x27 << 1); // STM is dumb and makes us shift it
static const uint32_t I2C_TIMEOUT = 100;
static const uint32_t DELAY = 2000;

uint8_t bl_on = 1;
uint8_t is_lcd_initialized = 0;

void LCD_WrCtrlNyb(uint8_t control_nybble)
{
    uint8_t exp_byte = 0x00;
    uint32_t ix = 0;

    exp_byte &= ~RS_MASK;

    exp_byte &= ~RW_MASK;

    if (bl_on)
    {
        exp_byte |= BL_MASK;
    }
    else
    {
        exp_byte &= ~BL_MASK;
    }
    exp_byte |= (control_nybble & 0x0F) << 4;

    exp_byte |= E_MASK;

    HAL_I2C_Master_Transmit(&hi2c1, lcd_addr, &exp_byte, 1, I2C_TIMEOUT);
    for (ix = 0; ix < DELAY; ix++)
        ; // STM doesn't have a micro-second delay

    exp_byte &= ~E_MASK;

    HAL_I2C_Master_Transmit(&hi2c1, lcd_addr, &exp_byte, 1, I2C_TIMEOUT);
    for (ix = 0; ix < DELAY; ix++)
        ;
}

void LCD_WrDataNyb(uint8_t data_nybble)
{
    uint8_t exp_byte = 0x00;
    uint32_t ix = 0;

    exp_byte |= RS_MASK;

    exp_byte &= ~RW_MASK;

    if (bl_on)
    {
        exp_byte |= BL_MASK;
    }
    else
    {
        exp_byte &= ~BL_MASK;
    }

    exp_byte |= (data_nybble & 0x0F) << 4;
    exp_byte |= E_MASK;

    HAL_I2C_Master_Transmit(&hi2c1, lcd_addr, &exp_byte, 1, I2C_TIMEOUT);
    for (ix = 0; ix < DELAY; ix++)
        ;

    exp_byte &= ~E_MASK;

    HAL_I2C_Master_Transmit(&hi2c1, lcd_addr, &exp_byte, 1, I2C_TIMEOUT);
    for (ix = 0; ix < DELAY; ix++)
        ;
}

void LCD_WriteData(uint8_t data_byte)
{
    LCD_WrDataNyb((data_byte) >> 4);
    LCD_WrDataNyb((data_byte & 0x0F));
}

void LCD_WriteControl(uint8_t control_byte)
{
    LCD_WrCtrlNyb((control_byte) >> 4);
    LCD_WrCtrlNyb((control_byte & 0x0F));
}

void LCD_Init(void)
{
    HAL_Delay(40);
    LCD_WrCtrlNyb(LCD_DISPLAY_8BIT_CMD);
    HAL_Delay(5);
    LCD_WrCtrlNyb(LCD_DISPLAY_8BIT_CMD);
    HAL_Delay(15);
    LCD_WrCtrlNyb(LCD_DISPLAY_8BIT_CMD);
    HAL_Delay(1);
    LCD_WrCtrlNyb(LCD_DISPLAY_4BIT_CMD);
    HAL_Delay(5);

    LCD_WriteControl(LCD_CURSOR_AUTO_INCREMENT_CMD);
    LCD_WriteControl(LCD_TURN_ON_DISPLAY_AND_CURSOR_CMD);
    LCD_WriteControl(LCD_MODE_2_ROWS_5_BY_10_CMD);
    LCD_WriteControl(LCD_TURN_OFF_DISPLAY_AND_CURSOR_CMD);
    LCD_WriteControl(LCD_CLEAR_DISPLAY_CMD);
    LCD_WriteControl(LCD_TURN_ON_DISPLAY_BUT_CURSOR_OFF_CMD);
    LCD_WriteControl(LCD_RESET_CURSOR_POSITION_CMD);

    HAL_Delay(5);
}

void LCD_Enable(void)
{
    LCD_WriteControl(LCD_TURN_ON_DISPLAY_BUT_CURSOR_OFF_CMD);
}

void LCD_Start(void)
{
    if (is_lcd_initialized == 0)
    {
        LCD_Init();
        is_lcd_initialized = 1;
    }

    LCD_Enable();
}

void LCD_Stop(void)
{
    LCD_WriteControl(LCD_TURN_OFF_DISPLAY_AND_CURSOR_CMD);
}

void LCD_Position(uint8_t row, uint8_t col)
{
    switch (row)
    {
    case 0:
        LCD_WriteControl(LCD_ROW_0_ST + col);
        break;
    case 1:
        LCD_WriteControl(LCD_ROW_1_ST + col);
        break;
    case 2:
        LCD_WriteControl(LCD_ROW_2_ST + col);
        break;
    case 3:
        LCD_WriteControl(LCD_ROW_3_ST + col);
        break;
    default:
        break;
    }
}

void LCD_PrintString(char const string[])
{
    uint8_t idx = 1;
    char current = *string;

    while ('\0' != current)
    {
        LCD_WriteData((uint8_t)current);
        current = string[idx];
        idx++;
    }
}

void LCD_PutChar(char output_char)
{
    LCD_WriteData((uint8_t)output_char);
}
