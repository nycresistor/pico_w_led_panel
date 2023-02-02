#ifndef __LED_MATRIX_H__
#define __LED_MATRIX_H__
#include <stdint.h>

#define ASCII_OFFSET 0x20
#define DIGIT_OFFSET 95

#define WIDTH 90
#define HEIGHT 7

extern uint8_t LETTERS[][5];

/// The framebuffer is double-buffered. All drawing methods draw to the offscreen
/// draw buffer; swap_buffer() sends the current buffer to the display.
/// draw buffer;

/// Set up led matrix.
void ledmatrix_setup();

/// Draw current framebuffer.
void ledmatrix_draw();

/// Commit the draw buffer to the framebuffer.
void swap_buffer();

void draw_col(
    const uint8_t col,
    const uint8_t bits,
    const uint8_t bright);

void draw_px(
    const uint8_t col,
    const uint8_t row,
    const uint8_t bright);

void draw_string(
    const char outText[]);

void draw_char(
    unsigned col,
    const char c);

void draw_small_digit(
    uint8_t column,
    unsigned digit,
    unsigned blinking);

void draw_time(
    uint8_t dig1,
    uint8_t dig2,
    uint8_t dig3,
    uint8_t dig4);

void draw_clear();

#endif // __LED_MATRIX_H__
