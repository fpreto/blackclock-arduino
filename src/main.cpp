#include <Arduino.h>
#include <LedControl.h>

// Main configuration
#define DEBUG
const unsigned long SLEEP_TIMER = 500;
const int MATRIX_NUM_DEVICES = 4;
const int DEVICE_HEIGHT = 8;
const int DEVICE_WIDTH = 8;
const int DISPLAY_HEIGHT = 8;
const int DISPLAY_WIDTH = MATRIX_NUM_DEVICES * 8;

const byte font_digit3x5[10][5] = {
    {
        0b111,
        0b101,
        0b101,
        0b101,
        0b111
    },

    {
        0b010,
        0b010,
        0b010,
        0b010,
        0b010
    },

    {
        0b111,
        0b001,
        0b111,
        0b100,
        0b111
    },

    {
        0b111,
        0b001,
        0b111,
        0b001,
        0b111
    },

    {
        0b101,
        0b101,
        0b111,
        0b001,
        0b001
    },

    {
        0b111,
        0b100,
        0b111,
        0b001,
        0b111
    },

    {
        0b111,
        0b100,
        0b111,
        0b101,
        0b111
    },

    {
        0b111,
        0b001,
        0b001,
        0b001,
        0b001
    },

    {
        0b111,
        0b101,
        0b111,
        0b101,
        0b111
    },

    {
        0b111,
        0b101,
        0b111,
        0b001,
        0b111
    }
};

// PIN CONFIG
#define	PIN_MATRIX_CLK		13  // or SCK
#define	PIN_MATRIX_DATA 	11  // or MOSI
#define	PIN_MATRIX_CS		10  // or SS

// Static variables
LedControl led_matrix(PIN_MATRIX_DATA, PIN_MATRIX_CLK, PIN_MATRIX_CS, MATRIX_NUM_DEVICES);

byte display_buffer[MATRIX_NUM_DEVICES * DISPLAY_HEIGHT];

bool has_rtc = false;
int counter = 0;
struct {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} time;

void update_current_time() {

    counter++;
    if (counter == 2) {
        time.second++;
        counter = 0;
    }

    if (time.second == 60) {
        time.minute++;
        time.second = 0;
    }

    if (time.minute == 60) {
        time.minute = 0;
        time.hour++;
    }

    if (time.hour == 13) {
        time.hour = 1;
    }


}

void set_led_matrix_intensity(int intensity) {
    for (int i = 0; i < MATRIX_NUM_DEVICES; i++) {
        led_matrix.setIntensity(i, intensity);
    }
}

void clear_display_buffer() {
    for (int y = 0; y < DEVICE_HEIGHT; y++) {
        for (int x = 0; x < MATRIX_NUM_DEVICES; x++) {
            display_buffer[y * MATRIX_NUM_DEVICES + x] = 0;
        }
    }
}

void reverse_display_buffer() {
    for (int y = 0; y < DEVICE_HEIGHT; y++) {
        for (int x = 0; x < MATRIX_NUM_DEVICES; x++) {
            display_buffer[y * MATRIX_NUM_DEVICES + x] = ~display_buffer[y * MATRIX_NUM_DEVICES + x];
        }
    }
}

void draw_display_buffer() {
    for (int y = 0; y < DEVICE_HEIGHT; y++) {
        for (int x = 0; x < MATRIX_NUM_DEVICES; x++) {
            led_matrix.setRow(MATRIX_NUM_DEVICES - x - 1, y, display_buffer[y * MATRIX_NUM_DEVICES + x]);
        }
    }
}

void set_led_display_buffer(int x, int y, bool state) {

    if (x < 0 || x >= DISPLAY_WIDTH || y < 0 || y >= DISPLAY_HEIGHT)
        return;

    int addr = y * MATRIX_NUM_DEVICES + (x / DEVICE_WIDTH);
    int bit = 7 - (x % DEVICE_WIDTH);

    if (state) {
        display_buffer[addr] |=  (1 << bit);
    } else {
        display_buffer[addr] &=  ~(1 << bit);
    }
}


void plot_3x5digit_on_display_buffer(int x, int y, int digit) {
    digit %= 10;
    const byte* digit_font = font_digit3x5[digit];

    for (int font_y = 0; font_y < 5; font_y++) {
        byte line = digit_font[font_y];

        for (int font_x = 0; font_x < 3; font_x++) {
            bool state = (line & (1 << (2 - font_x)));

            if (state)
                set_led_display_buffer(x + font_x, y + font_y, state);
        }
    }
}

void plot_time(int x, int y, bool seconds) {

    int hour = (time.hour > 12) ? time.hour - 12 : time.hour;
    if (hour >= 10)
        plot_3x5digit_on_display_buffer(x, y, hour / 10);
    
    plot_3x5digit_on_display_buffer(x + 4, y, hour % 10);

    set_led_display_buffer(x + 8, y + 1, true);
    set_led_display_buffer(x + 8, y + 3, true);

    plot_3x5digit_on_display_buffer(x + 10, y, time.minute / 10);
    plot_3x5digit_on_display_buffer(x + 14, y, time.minute % 10);

    if (seconds) {
        set_led_display_buffer(x + 18, y + 1, true);
        set_led_display_buffer(x + 18, y + 3, true);

        plot_3x5digit_on_display_buffer(x + 20, y, time.second / 10);
        plot_3x5digit_on_display_buffer(x + 24, y, time.second % 10);
    }

}

void plot_centered_time_simple() {
    plot_time(7, 1, false);
}

void plot_centered_time_full() {
    plot_time(2, 1, true);
}

void update_display() {
    clear_display_buffer();

    plot_centered_time_full();

    //reverse_display_buffer();

    draw_display_buffer();
}

void setup() {
#ifdef DEBUG
    Serial.begin(9600);
#endif

    time.hour = 12;
    time.minute = 00;
    time.second = 00;

    pinMode(LED_BUILTIN, OUTPUT);

    // Initialized Matrix
    for (int i = 0; i < MATRIX_NUM_DEVICES; i++) {
        led_matrix.shutdown(i, false);
    }

    set_led_matrix_intensity(3);
}

void loop() {

    digitalWrite(LED_BUILTIN, HIGH);

    // Read Time from RTC
    update_current_time();

    // Check any input

    // Display
    update_display();

    // Sleep
    if (SLEEP_TIMER > 0) {
        digitalWrite(LED_BUILTIN, LOW);
        delay(SLEEP_TIMER);
    }

}