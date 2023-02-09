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



// Frame buffers
//typedef struct {
//    uint8_t fb[HEIGHT][WIDTH];
//} FrameBuf;

uint8_t fb1[HEIGHT*WIDTH];
uint8_t fb2[HEIGHT*WIDTH];


static uint8_t* disp_fb = fb2;
static uint8_t* draw_fb = fb1;

/// Commit the draw buffer to the framebuffer.
void swap_buffer() {
    /// TODO: SYNC
    uint8_t* tmp = disp_fb;
    disp_fb = draw_fb;
    draw_fb = tmp;
}

// Row pins as an array for convenience
static const uint16_t row_pins[HEIGHT] = { ROW0, ROW1, ROW2, ROW3, ROW4, ROW5, ROW6 };

void ledmatrix_setup()
{
    for (int i = 0; i < HEIGHT; i++) {
        gpio_init(row_pins[i]);
        gpio_set_dir(row_pins[i], GPIO_IN);
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
            disp_fb[y*WIDTH + x] = (x % 8) < y ? 0 : 1;
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
            for (int j = 0; j < HEIGHT; j++) {
                row(row_pins[j], disp_fb+(j*WIDTH), i * 256 / 8, bright[i]);
            }
        }
    }
}

void binary_loop()
{
    for (int i = 0; i < HEIGHT; i++) {
        row(row_pins[i], disp_fb+(i*WIDTH), 0, 400);
    }
}

void ledmatrix_draw()
{
    binary_loop();
    // pwm_loop();
}

/** Set an entire column at once */
void draw_col(
    const uint8_t col,
    const uint8_t bits,
    const uint8_t bright)
{
    for (uint8_t i = 0; i < HEIGHT; i++)
        draw_px(col, i, (bits & (1 << i)) ? bright : 0);
}

void draw_px(
    const uint8_t col,
    const uint8_t row,
    const uint8_t bright)
{
    draw_fb[row*WIDTH+col] = bright;
}


uint8_t
draw_char(
	unsigned col,
	const char c,
        bool proportional
)
{
    uint8_t w = 0;
    for (uint8_t y=0 ; y<5 ; y++) {
        uint8_t cv = LETTERS[c-ASCII_OFFSET][y];
        if (!proportional || (cv != 0)) {
            draw_col(col++, LETTERS[c - ASCII_OFFSET][y], 0xFF); w++;
        }
    }
    return w;
}


/*
 * Output 4 Characters to Display
 */
void
draw_string(
            const char * outText,
            bool proportional
        
)
{
	for (int i=0 ; i < WIDTH && *outText ; i+=1)
            i += draw_char(i, *outText++, proportional);
}


void
draw_small_digit(
	uint8_t column,
	unsigned digit,
	unsigned blinking
)
{
	for (unsigned i=0 ; i < 4 ; i++)
	{
		draw_col(
			column+i,
			LETTERS[digit+DIGIT_OFFSET][i+1],
			0xFF //blinkON && blinking ? 0 : 0xFF
		);
	}
}


/*
 * Clear LED Matrix
 */
void
draw_clear()
{
	for (int i=0 ; i<WIDTH ; i++)
		draw_col(i, 0, 0);
}



/**
 * Display the four digit time with small characters.
 *
 *
 * Fills diplay with formated time
 * Depending on postion of "1"s spacing is adjusted beween it and next digit
 * Blinks if it settng mode
 * displays AM/PM dot and Alarm on dot
 */
void
draw_time(
	uint8_t dig1,
	uint8_t dig2,
	uint8_t dig3,
	uint8_t dig4
)
{
	const int blinkHour = 0;
	const int blinkMin = 0;

	draw_small_digit( 2, dig1, blinkHour);
	draw_small_digit( 6, dig2, blinkHour);

	// the slowly flashing " : "
	static uint16_t bright = 0;
	uint8_t b = bright++ / 1;
	if (b >= 128)
		b = 0xFF - b;
	draw_px(10, 2, 2*b);
	draw_px(10, 4, 2*b);

	draw_small_digit(12, dig3, blinkMin);
	draw_small_digit(16, dig4, blinkMin);

#if 0
	AMPMALARMDOTS = 0;

	// Alarm dot (top left) Do not display while setting alarm
	if (ALARMON && (STATE == 1))
		bitSet(AMPMALARMDOTS,6);

	// AM / PM dot (bottom left) (Display or Set Time)
	if(PM_NotAM_flag && (STATE == 1 || STATE == 2) && TH_Not24_flag)
		bitSet(AMPMALARMDOTS,0);

	// AM / PM dot (bottom left) (Set Alarm Time)
	if(A_PM_NotAM_flag && (STATE == 3) && TH_Not24_flag)
		bitSet(AMPMALARMDOTS,0);
#endif
}
