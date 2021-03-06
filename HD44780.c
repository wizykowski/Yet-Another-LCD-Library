/**
 * Driver library for HD44780-compatible LCD displays.
 * https://github.com/armandas/Yet-Another-LCD-Library
 *
 * Author: Armandas Jarusauskas (http://projects.armandas.lt)
 *
 * Contributors:
 *    Sylvain Prat (https://github.com/sprat)
 *
 * This work is licensed under a
 * Creative Commons Attribution 3.0 Unported License.
 * http://creativecommons.org/licenses/by/3.0/
 *
 */

#include <delays.h>
#include "HD44780.h"

// private function prototypes
static void _send_nibble(unsigned char);
static void _send_byte(unsigned char);
static void _set_4bit_interface();

// global variables
unsigned char display_config[6];

// private utility functions
static void _send_nibble(unsigned char data) {
    data <<= DATA_SHIFT;          // shift the data as required
    LCD_DATA &= ~DATA_MASK;       // clear old data bits
    LCD_DATA |= DATA_MASK & data; // put in new data bits

    LCD_EN = 1;
    LCD_ENABLE_PULSE_DELAY();
    LCD_EN = 0;
    LCD_ENABLE_PULSE_DELAY();
}

static void _send_byte(unsigned char data) {
    _send_nibble(data >> 4);
    _send_nibble(data & 0x0F);
}

static void _set_4bit_interface() {
    LCD_RS = 0;
    LCD_RW = 0;
    LCD_ADDRESS_SETUP_DELAY();
    _send_nibble(0b0010);
    LCD_EXECUTION_DELAY();
}

/**
 * Display string stored in RAM
 *
 * Usage:
 *     char message[10] = "Hello";
 *     lcd_write(message);
 */
void lcd_write(char * str) {
    unsigned char i = 0;

    while (str[i] != '\0')
        lcd_data(str[i++]);
}

/**
 * Display string stored in program memory
 *
 * Usage:
 *     lcd_write_pgm("Hello");
 */
void lcd_write_pgm(const rom char * str) {
    unsigned char i = 0;

    while (str[i] != '\0')
        lcd_data(str[i++]);
}

/**
 * Move cursor to a given location
 */
void lcd_goto(unsigned char row, unsigned char col) {
    unsigned char addr;

    switch (row) {
        case 3:
            // fall through
        case 4:
            addr = ((row - 3) * 0x40) + 0x14 + col - 1;
            break;
        default:
            // rows 1 and 2
            addr = ((row - 1) * 0x40) + col - 1;
            break;
    }

    lcd_command(SET_DDRAM_ADDR | addr);
}

/**
 * Add a custom character
 */
void lcd_add_character(unsigned char addr, unsigned char * pattern) {
    unsigned char i;

    lcd_command(SET_CGRAM_ADDR | addr << 3);
    for (i = 0; i < 8; i++)
        lcd_data(pattern[i]);
}

/**
 * Lousy function for automatic LCD initialization
 */
void lcd_initialize(void) {
    unsigned char i;

    // set relevant pins as outputs
    LCD_DATA_DDR = ~DATA_MASK;
    LCD_RS_DDR = 0;
    LCD_RW_DDR = 0;
    LCD_EN_DDR = 0;

    #ifdef LCD_HAS_BACKLIGHT
    LCD_BL_DDR = 0;
    #endif

    // initialize the display_config
    for (i = 0; i < 6; i++) {
        display_config[i] = 0x00;
    }

    _set_4bit_interface();

    // function set
    lcd_flags_set(FUNCTION_SET, DATA_LENGTH | CHAR_FONT, 0);
    lcd_flags_set(FUNCTION_SET, DISPLAY_LINES, 1);

    #ifdef LCD_HAS_BACKLIGHT
    lcd_backlight_on();
    #endif
    lcd_display_on();
    lcd_cursor_off();
    lcd_blinking_off();

    lcd_flags_set(ENTRY_MODE, CURSOR_INCREMENT, 1);
    lcd_flags_set(ENTRY_MODE, ENABLE_SHIFTING, 0);

    lcd_clear();
    lcd_return_home();
}

void lcd_command(unsigned char command) {
    LCD_RS = 0;
    LCD_RW = 0;
    LCD_ADDRESS_SETUP_DELAY();
    _send_byte(command);
    LCD_EXECUTION_DELAY();
}

void lcd_data(unsigned char data) {
    LCD_RS = 1;
    LCD_RW = 0;
    LCD_ADDRESS_SETUP_DELAY();
    _send_byte(data);
    LCD_EXECUTION_DELAY();
}

void lcd_flags_set(unsigned char instruction,
                   unsigned char flags,
                   unsigned char value)
{
    unsigned char index;

    switch (instruction) {
        case ENTRY_MODE:
            index = 0;
            break;
        case DISPLAY_CONTROL:
            index = 1;
            break;
        case CURSOR_DISPLAY_SHIFT:
            index = 2;
            break;
        case FUNCTION_SET:
            index = 3;
            break;
        case SET_CGRAM_ADDR:
            index = 4;
            break;
        case SET_DDRAM_ADDR:
            index = 5;
            break;
    }

    if (value == 0)
        display_config[index] &= ~flags; // reset flags
    else
        display_config[index] |= flags; // set flags

    lcd_command(instruction | display_config[index]);
}
