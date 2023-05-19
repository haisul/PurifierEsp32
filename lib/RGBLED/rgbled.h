#ifndef __RGBLED_H_
#define __RGBLED_H_

#include <Arduino.h>
#include <FastLED.h>

// when use Jtag debugger, the pin number is different
// #define LED_PIN 23
#define LED_PIN 13
// 2. set this to the number of LEDs on the RGB strip
#define NUM_LEDS 48
// 3. set this to a number between 0 and 100 to set a brightness
#define BRIGHTNESS 100
// array used by the FastLED library

const int colours[][3] = {{255, 255, 255}, {0, 0, 0}, {255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {127, 127, 127}, {255, 127, 127}, {127, 0, 255}};

typedef enum {
    SLOWEST = 0,
    SLOWER = 1,
    SLOW = 2,
    MEDIUM = 3,
    FAST = 4,
    FASTER = 5,
    FASTEST = 6
} speed_types;

typedef enum {
    WHITE = 0,
    BLACK = 1,
    RED = 2,
    GREEN = 3,
    BLUE = 4,
    TURQUOISE = 5,
    YELLOW = 6,
    PURPLE = 7
} colors;

typedef enum {
    FORWARD = 1,
    BACKWARD = -1,
} derection;

#define SLOWEST SLOWEST
#define SLOWER SLOWER
#define SLOW SLOW
#define MEDIUM MEDIUM
#define FAST FAST
#define FASTER FASTER
#define FASTEST FASTEST

#define WHITE WHITE
#define BLACK BLACK
#define RED RED
#define GREEN GREEN
#define BLUE BLUE
#define TURQUOISE TURQUOISE
#define YELLOW YELLOW
#define PURPLE PURPLE

#define FORWARD FORWARD
#define BACKWARD BACKWARD

class rgbLed {
private:
    CRGB leds[NUM_LEDS];
    const int (*colours)[3] = ::colours;
    int n = 0, brightnessMultiplier = 100;
    int curTime = millis();
    int delayTime = 1;

public:
    rgbLed();
    void setLedColour(int led, int red, int green, int blue);
    void show();
    float sinMuti(float angle, float multiplier);
    float sin255(float angle);
    float sin1(float angle);
    int mixValues(int a, int b, float ratio);
    int *mixColours(int red1, int green1, int blue1,
                    int red2, int green2, int blue2, float ratio);
    void incrementN(int speed, int direction);
    int *custom(int red, int green, int blue);
    void brightness(int b);
    void circleRainbow3(int speed, int direction);
    void circleRainbow2(int speed, int direction);
    void circleRainbow(int speed, int direction);
    void solidRainbow(int speed);
    void circleTwoColours(int red1, int green1, int blue1,
                          int red2, int green2, int blue2, int speed, int direction);
    void circleTwoColours(int colour1, int colour2, int speed, int direction);
    void circle(int red, int green, int blue, int speed, int direction);
    void circle(int colour, int speed, int direction);
    void fadeTwoColours(int red1, int green1, int blue1,
                        int red2, int green2, int blue2, int speed);
    void fadeTwoColours(int colour1, int colour2, int speed, int direction);
    void pulse(int red, int green, int blue, int speed);
    void pulse(int colour, int speed);
    void solidColour(int red, int green, int blue);
    void solidColour(int colour);
};

#endif