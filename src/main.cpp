#include <Arduino.h>
#include <LedControl.h>
#include <Wire.h>
#include <RtcDS3231.h>
#include <string.h>

#include "utils.h"
#include "font_digit3x5.h"
#include "font_digit4x7.h"

// Main configuration
const uint32_t SLEEP_TIMER = 250;
const uint8_t DISPLAY_BRIGHTNESS_DEFAULT = 3;
const uint8_t DISPLAY_BRIGHTNESS_LOW = 0;
const uint8_t DISPLAY_BRIGHTNESS_HIGH = 7;

const uint8_t MATRIX_NUM_DEVICES = 4;
const uint8_t DEVICE_HEIGHT = 8;
const uint8_t DEVICE_WIDTH = 8;
const uint8_t DISPLAY_HEIGHT = 8;
const uint8_t DISPLAY_WIDTH = MATRIX_NUM_DEVICES * 8;

// PIN CONFIG
const uint8_t PIN_MATRIX_CLK = 13;  // or SCK
const uint8_t PIN_MATRIX_DATA = 11;  // or MOSI
const uint8_t PIN_MATRIX_CS = 10;  // or SS

// Board interfaces
LedControl led_matrix(PIN_MATRIX_DATA, PIN_MATRIX_CLK, PIN_MATRIX_CS, MATRIX_NUM_DEVICES);
RtcDS3231<TwoWire> rtc(Wire);

// Display buffer
char display_buffer[MATRIX_NUM_DEVICES * DISPLAY_HEIGHT];

// Time holder
struct {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} time;

// Serial input
char serial_input[256];
uint8_t serial_read_pos;


void handle_serial_command() {
    while (Serial.available() > 0) {
        
        char c = Serial.read();
        Serial.print(c);

        serial_input[serial_read_pos] = c;

        if (c == '\n') {;
            serial_input[serial_read_pos] = '\0';
            serial_read_pos = 0;

            char* pch = strtok(serial_input, " ");

            if (pch == NULL) {
                Serial.println("Where is the command? Type HELP");
                return;
            }

            str_to_uppper(pch);

            if (strncmp("HELP", pch, 4) == 0) {
                Serial.println("Available Commands:");
                Serial.println("SET Oct 12 1984, 16:35:31");
                Serial.println("TEMP");
                Serial.println("");
                return;
            } else if (strncmp("SET", pch, 3) == 0) {
                char *pdate = strtok(NULL, ",");

                if (pdate == NULL) {
                    Serial.println("Invalid date, see HELP");
                    return;
                }

                char *ptime = strtok(NULL," ");
                if (ptime == NULL) {
                    Serial.println("Invalid time, see HELP");
                    return;
                }

                Serial.print("Date: ");
                Serial.print(pdate);

                Serial.print(" Time: ");
                Serial.println(ptime);

                RtcDateTime new_time(pdate, ptime);
                rtc.SetDateTime(new_time);

                return;
            } else if (strncmp("TEMP", pch, 4) == 0) {
                RtcTemperature temp = rtc.GetTemperature();
                Serial.print("Current internal temperature: ");
                Serial.print(celsius_to_fahrenheit(temp.AsFloat()));
                Serial.println("F");
                return;
            } else {
                Serial.print("Error! Invalid command: ");
                Serial.println(pch);
                Serial.println("Type HELP");
                return;
            }


        } else if (c == '\b') {
            if (serial_read_pos > 0)
                serial_read_pos--;
        } else {
            if (serial_read_pos < 255) {
                serial_read_pos++;
            } else {
                Serial.println("!!! - Too much data without ending a line, reseting buffer");
                serial_read_pos = 0;
            }
        }
    }
}

void update_current_time() {
    RtcDateTime now = rtc.GetDateTime();
    time.hour = now.Hour();
    time.minute = now.Minute();
    time.second = now.Second();
}

void set_led_matrix_intensity(uint8_t intensity) {
    for (uint8_t i = 0; i < MATRIX_NUM_DEVICES; i++) {
        led_matrix.setIntensity(i, intensity);
    }
}

void clear_display_buffer() {
    for (uint8_t y = 0; y < DEVICE_HEIGHT; y++) {
        for (uint8_t x = 0; x < MATRIX_NUM_DEVICES; x++) {
            display_buffer[y * MATRIX_NUM_DEVICES + x] = 0;
        }
    }
}

void reverse_display_buffer() {
    for (uint8_t y = 0; y < DEVICE_HEIGHT; y++) {
        for (uint8_t x = 0; x < MATRIX_NUM_DEVICES; x++) {
            display_buffer[y * MATRIX_NUM_DEVICES + x] = ~display_buffer[y * MATRIX_NUM_DEVICES + x];
        }
    }
}

void draw_display_buffer() {
    for (uint8_t y = 0; y < DEVICE_HEIGHT; y++) {
        for (uint8_t x = 0; x < MATRIX_NUM_DEVICES; x++) {
            led_matrix.setRow(MATRIX_NUM_DEVICES - x - 1, y, display_buffer[y * MATRIX_NUM_DEVICES + x]);
        }
    }
}

void set_led_display_buffer(uint8_t x, uint8_t y, bool state) {
    if (x < 0 || x >= DISPLAY_WIDTH || y < 0 || y >= DISPLAY_HEIGHT)
        return;

    uint8_t addr = y * MATRIX_NUM_DEVICES + (x / DEVICE_WIDTH);
    uint8_t bit = 7 - (x % DEVICE_WIDTH);

    if (state) {
        display_buffer[addr] |=  (1 << bit);
    } else {
        display_buffer[addr] &=  ~(1 << bit);
    }
}


void plot_3x5digit_on_display_buffer(uint8_t x, uint8_t y, uint8_t digit) {
    digit %= 10;
    const byte* digit_font = font_digit3x5[digit];

    for (uint8_t font_y = 0; font_y < 5; font_y++) {
        uint8_t line = digit_font[font_y];

        for (uint8_t font_x = 0; font_x < 3; font_x++) {
            bool state = (line & (1 << (2 - font_x)));

            if (state)
                set_led_display_buffer(x + font_x, y + font_y, state);
        }
    }
}


void plot_4x7digit_on_display_buffer(uint8_t x, uint8_t y, uint8_t digit) {
    digit %= 10;
    const byte* digit_font = font_digit4x7[digit];

    for (uint8_t font_y = 0; font_y < 7; font_y++) {
        uint8_t line = digit_font[font_y];

        for (uint8_t font_x = 0; font_x < 4; font_x++) {
            bool state = (line & (1 << (3 - font_x)));

            if (state)
                set_led_display_buffer(x + font_x, y + font_y, state);
        }
    }
}

void plot_time(uint8_t x, uint8_t y, bool seconds) {

    int hour = (time.hour > 12) ? time.hour - 12 : time.hour;

    if (hour == 0)
        hour = 12;

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

void plot_time_big(uint8_t x, uint8_t y) {

    int hour = (time.hour > 12) ? time.hour - 12 : time.hour;

    if (hour == 0)
        hour = 12;

    if (hour >= 10)
        plot_4x7digit_on_display_buffer(x, y, hour / 10);
    
    plot_4x7digit_on_display_buffer(x + 5, y, hour % 10);

    set_led_display_buffer(x + 10, y + 2, true);
    set_led_display_buffer(x + 10, y + 4, true);

    plot_4x7digit_on_display_buffer(x + 12, y, time.minute / 10);
    plot_4x7digit_on_display_buffer(x + 17, y, time.minute % 10);
}

void plot_centered_time_simple() {
    plot_time(7, 1, false);
}

void plot_centered_time_full() {
    plot_time(2, 1, true);
}

void plot_centered_time_big() {
    if (is_double_digit_hour_in_12h(time.hour)) {
        plot_time_big(5,1);
    } else {
        plot_time_big(3,1);
    }
}

void update_display() {
    // Configure brightness
    if (time.hour < 7) {
        set_led_matrix_intensity(DISPLAY_BRIGHTNESS_LOW);
    } else if (time.hour < 18) {
        set_led_matrix_intensity(DISPLAY_BRIGHTNESS_HIGH);
    } else {
        set_led_matrix_intensity(DISPLAY_BRIGHTNESS_DEFAULT);
    }

    clear_display_buffer();

    plot_centered_time_big();

    draw_display_buffer();
}

void setup() {
    Serial.begin(9600);
    Serial.println("Black Clock Serial Console. Please type HELP");
    serial_read_pos = 0;

    time.hour = 00;
    time.minute = 00;
    time.second = 00;

    pinMode(LED_BUILTIN, OUTPUT);

    // Initialized Matrix
    for (uint8_t i = 0; i < MATRIX_NUM_DEVICES; i++) {
        led_matrix.shutdown(i, false);
    }

    set_led_matrix_intensity(DISPLAY_BRIGHTNESS_DEFAULT);

    // Initialize RTC
    rtc.Begin();

    if (!rtc.IsDateTimeValid()) {
        RtcDateTime compiled(__DATE__, __TIME__);
        rtc.SetDateTime(compiled);
    }

    if (!rtc.GetIsRunning()) {
        rtc.SetIsRunning(true);
    }

    rtc.Enable32kHzPin(false);
    rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone); 
}

void loop() {

    digitalWrite(LED_BUILTIN, HIGH);

    // Read Time from RTC
    update_current_time();

    // Check any input
    handle_serial_command();

    // Display
    update_display();

    // Sleep
    if (SLEEP_TIMER > 0) {
        digitalWrite(LED_BUILTIN, LOW);
        delay(SLEEP_TIMER);
    }

}
