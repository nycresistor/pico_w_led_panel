/*
 * Adapted from Trammell's https://github.com/osresearch/sparktime.git
 * 18 * 5x7 modules = 90x7 screen
 */

#include "pico/stdlib.h"
#include "led_matrix.h"

// Pin definitions

#define LED_LATCH 22
#define LED_CLOCK2 21
#define LED_CLOCK1 20
#define LED_DATA 19
#define LED_ENABLE 18
#define ROW0 5
#define ROW1 6
#define ROW2 7
#define ROW3 8
#define ROW4 9
#define ROW5 10
#define ROW6 11

// Frame buffer
static uint8_t fb[HEIGHT][WIDTH];

// Row pins as an array for convenience
static const uint16_t row_pins[HEIGHT] = { ROW0, ROW1, ROW2, ROW3, ROW4, ROW5, ROW6 };

void ledmatrix_setup()
{
    for (int i = 0; i < HEIGHT; i++) {
        gpio_init(row_pins[i]);
        gpio_set_dir(row_pins[i], GPIO_IN);

        //                  ___          _____          ___
        //      ___        /  /\        /  /::\        /  /\    
        //     /  /\      /  /::\      /  /:/\:\      /  /::\   
        //    /  /:/     /  /:/\:\    /  /:/  \:\    /  /:/\:\  
        //   /  /:/     /  /:/  \:\  /__/:/ \__\:|  /  /:/  \:\ 
        //  /  /::\    /__/:/ \__\:\ \  \:\ /  /:/ /__/:/ \__\:\
        // /__/:/\:\   \  \:\ /  /:/  \  \:\  /:/  \  \:\ /  /:/
        // \__\/  \:\   \  \:\  /:/    \  \:\/:/    \  \:\  /:/
        //      \  \:\   \  \:\/:/      \  \::/      \  \:\/:/
        //       \__\/    \  \::/        \__\/        \  \::/
        //                 \__\/                       \__\/
        // gpio_disable_pulls(row_pins[i]);  //// TODO: triple-check that this works
        gpio_pull_down(row_pins[i]);
    }
    // The pins for the shift registers; this array is only needed for initialization.
    static const uint16_t sr_pins[] = { LED_DATA, LED_LATCH, LED_ENABLE, LED_CLOCK1, LED_CLOCK2 };
    for (int i = 0; i < 5; i++) {
        gpio_init(sr_pins[i]);
        gpio_set_dir(sr_pins[i], GPIO_OUT);
    }

    // Load a test pattern into the framebuffer.
    for (int y = 0; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++)
            fb[y][x] = (x % 8) < y ? 0 : 1;
}

static inline void row(const int row_pin,
    const uint8_t* const data,
    const uint8_t bright,
    const unsigned delay)
{
    // clock 1 drops first, starting the cycle
    gpio_put(LED_CLOCK1, 0);
    sleep_us(1);
    gpio_put(LED_LATCH, 0);
    // then the evens are clocked out
    for (unsigned i = 0; i < WIDTH; i += 2) {
        gpio_put(LED_CLOCK1, 1);
        gpio_put(LED_DATA, data[i] > bright ? 1 : 0);
        gpio_put(LED_CLOCK1, 0);
    }

    // after the evens clock2 goes HIGH
    gpio_put(LED_CLOCK2, 1);
    sleep_us(1);

    // and then the odds are clocked out
    for (unsigned i = 1; i < WIDTH; i += 2) {
        gpio_put(LED_CLOCK2, 1);
        gpio_put(LED_DATA, data[i] > bright ? 1 : 0);
        gpio_put(LED_CLOCK2, 0);
    }

    // always return the data pin to zero
    gpio_put(LED_DATA, 0);

    //// enable goes high a few microseconds before the clocks and latch
    // digitalWrite(LED_ENABLE, 1);

    // latch goes high
    gpio_put(LED_LATCH, 1);
    // delayMicroseconds(1);

    // then the reset the clocks to their idle state
    gpio_put(LED_CLOCK1, 1);
    gpio_put(LED_CLOCK2, 0);

    // turn on the pin
    gpio_put(LED_ENABLE, 0);
    gpio_set_dir(row_pin, GPIO_OUT);
    gpio_put(row_pin, 0);

    sleep_us(delay);

    // turn off the pin
    gpio_set_dir(row_pin, GPIO_IN);

    // disable the output
    gpio_put(LED_ENABLE, 1);
}

static const unsigned bright[] = {
    1,
    30,
    60,
    90,
    120,
    160,
    250,
    400
};

void pwm_loop()
{
    // for(unsigned x = 0; x < 256 ; x++)
    {
        for (unsigned i = 0; i < 8; i++) {
            row(ROW0, fb[0], i * 256 / 8, bright[i]);
            row(ROW1, fb[1], i * 256 / 8, bright[i]);
            row(ROW2, fb[2], i * 256 / 8, bright[i]);
            row(ROW3, fb[3], i * 256 / 8, bright[i]);
            row(ROW4, fb[4], i * 256 / 8, bright[i]);
            row(ROW5, fb[5], i * 256 / 8, bright[i]);
            row(ROW6, fb[6], i * 256 / 8, bright[i]);
        }
    }
}

void binary_loop()
{
    row(ROW0, fb[0], 0, 400);
    row(ROW1, fb[1], 0, 400);
    row(ROW2, fb[2], 0, 400);
    row(ROW3, fb[3], 0, 400);
    row(ROW4, fb[4], 0, 400);
    row(ROW5, fb[5], 0, 400);
    row(ROW6, fb[6], 0, 400);
}

void ledmatrix_draw()
{
    binary_loop();
    // pwm_loop();
}

/** Set an entire column at once */
void ledmatrix_set_col(
    const uint8_t col,
    const uint8_t bits,
    const uint8_t bright)
{
    for (uint8_t i = 0; i < HEIGHT; i++)
        ledmatrix_set(col, i, (bits & (1 << i)) ? bright : 0);
}

void ledmatrix_set(
    const uint8_t col,
    const uint8_t row,
    const uint8_t bright)
{
    fb[row][col] = bright;
}
