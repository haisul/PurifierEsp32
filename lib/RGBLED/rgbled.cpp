#include "rgbled.h"

rgbLed::rgbLed() {
    FastLED.addLeds<SM16703, LED_PIN, RBG>(leds, NUM_LEDS);
}

/* ( MODE 1 ) only one colour, no changes */
/** Wrapper for the other solidColour(), this one takes a predefined colour
 *
 * @param colour colour from predefined 8 colours
 */
void rgbLed::solidColour(int colour) {
    solidColour(colours[colour][0], colours[colour][1], colours[colour][2]);
}

/** One colour, no animation
 *
 * @param red red value
 * @param green green value
 * @param blue blue value
 */
void rgbLed::solidColour(int red, int green, int blue) {
    for (int i = 0; i < NUM_LEDS; i++) {
        setLedColour(i, red, green, blue);
    }
    delay(100000);
}

/* ( MODE 2 ) pulsing colour, only one colour */
/** Wrapper for the other pulse(), this one takes a predefined colour
 *
 * @param colour colour from predefined 8 colours
 * @param speed speed of animation
 */
void rgbLed::pulse(int colour, int speed) {
    pulse(colours[colour][0], colours[colour][1], colours[colour][2], speed);
}

/** One colour turning on and off smoothly
 *
 * @param red red value
 * @param green green value
 * @param blue blue value
 * @param speed speed of animation
 */
void rgbLed::pulse(int red, int green, int blue, int speed) {
    for (int i = 0; i < NUM_LEDS; i++) {
        setLedColour(i, sinMuti(n, red), sinMuti(n, green), sinMuti(n, blue));
    }
    incrementN(speed, FORWARD);
}

/* ( MODE 3 ) fading with two colours, */
/** Wrapper for the other circle(), this one takes 2 predefined colours
 *
 * @param colour1 colour 1 from predefined 8 colours
 * @param colour2 colour 2 from predefined 8 colours
 * @param speed speed of animation
 * @param direction direction of flow
 */
void rgbLed::fadeTwoColours(int colour1, int colour2, int speed, int direction) {
    fadeTwoColours(colours[colour1][0], colours[colour1][1], colours[colour1][2],
                   colours[colour2][0], colours[colour2][1], colours[colour2][2], speed);
}

/** whole strip changing between two colours smoothly
 *
 * @param red1 red value 1
 * @param green1 green value 1
 * @param blue1 blue value 1
 * @param red2 red value 2
 * @param green2 green value 2
 * @param blue2 blue value 2
 * @param speed speed of animation
 * @param direction direction of flow
 */
void rgbLed::fadeTwoColours(int red1, int green1, int blue1,
                            int red2, int green2, int blue2, int speed) {

    for (int i = 0; i < NUM_LEDS; i++) {
        setLedColour(i,
                     mixValues(red1, red2, sin1(n)),
                     mixValues(green1, green2, sin1(n)),
                     mixValues(blue1, blue2, sin1(n)));
    }

    incrementN(speed, FORWARD);
}

/* ( MODE 4 ) circling, only one colour */
/** Wrapper for the other circle(), this one takes a predefined colour
 *
 * @param colour colour from predefined 8 colours
 * @param speed speed of animation
 * @param direction direction of flow
 */
void rgbLed::circle(int colour, int speed, int direction) {
    circle(colours[colour][0], colours[colour][1], colours[colour][2], speed, direction);
}

/** One colour going around
 *
 * @param red red value
 * @param green green value
 * @param blue blue value
 * @param speed speed of animation
 * @param direction direction of flow
 */
void rgbLed::circle(int red, int green, int blue, int speed, int direction) {
    for (int i = 0; i < NUM_LEDS; i++) {
        setLedColour(i, sinMuti(n + i * 24, red), sinMuti(n + i * 24, green), sinMuti(n + i * 24, blue));
    }
    incrementN(speed, direction);
}

/* ( MODE 5 ) circling, two colours */
/** Wrapper for the other circleTwoColours(), this one takes predefined colours
 *
 * @param colour1 colour 1 from predefined 8 colours
 * @param colour2 colour 2 from predefined 8 colours
 * @param speed speed of animation
 * @param direction direction of flow
 */
void rgbLed::circleTwoColours(int colour1, int colour2, int speed, int direction) {
    circleTwoColours(colours[colour1][0], colours[colour1][1], colours[colour1][2],
                     colours[colour2][0], colours[colour2][1], colours[colour2][2], speed, direction);
}

/** Two colours going around
 *
 * @param red1 red value 1
 * @param green1 green value 1
 * @param blue1 blue value 1
 * @param red2 red value 2
 * @param green2 green value 2
 * @param blue2 blue value 2
 * @param speed speed of animation
 * @param direction direction of flow
 */
void rgbLed::circleTwoColours(int red1, int green1, int blue1,
                              int red2, int green2, int blue2, int speed, int direction) {

    for (int i = 0; i < NUM_LEDS; i++) {
        setLedColour(i,
                     mixValues(red1, red2, sin1(n + i * 24)),
                     mixValues(green1, green2, sin1(n + i * 24)),
                     mixValues(blue1, blue2, sin1(n + i * 24)));
    }

    incrementN(speed, direction);
}

/* ( MODE 6 ) each LED going through rainbow together */
/** each LED going through rainbow together
 *
 * @param speed speed of animation
 */
void rgbLed::solidRainbow(int speed) {
    for (int i = 0; i < NUM_LEDS; i++) {
        setLedColour(i, sin255(n), sin255(n + 120), sin255(n + 240));
    }
    incrementN(speed, FORWARD);
}

/* ( MODE 7 ) LEDs going through rainbow in waves */
/** Standard rainbow going around
 *
 * @param speed speed of animation
 * @param direction direction of flow
 */
void rgbLed::circleRainbow(int speed, int direction) {
    for (int i = 0; i < NUM_LEDS; i++) {
        setLedColour(i, sin255(n + i * 24), sin255(n + i * 24 + 120), sin255(n + i * 24 + 240));
    }
    incrementN(speed / 4, direction);
}

/** Funny rainbow going around
 *
 * @param speed speed of animation
 * @param direction direction of flow
 */
void rgbLed::circleRainbow2(int speed, int direction) {
    for (int i = 0; i < NUM_LEDS; i++) {
        setLedColour(i, sin255(n + i * 6), sin255(n + i * 0), sin255(n + i * 24));
    }
    incrementN(speed, direction);
}

/** Funny rainbow going around
 *
 * @param speed speed of animation
 * @param direction direction of flow
 */
void rgbLed::circleRainbow3(int speed, int direction) {
    for (int i = 0; i < NUM_LEDS; i++) {
        // setLedColour(i, sin255(n + i * 2), sin255(n + i * 6), sin255(n + i * 0));
        setLedColour(i, sin255(n + i * 2), sin255(n + i * 6), sin255(n + i * 0));
    }
    incrementN(speed, direction);
}

// -------------------
// main helper methods
// -------------------

/** sets max brightness value
 *
 * @param b brightness from 0 to 100
 */
void rgbLed::brightness(int b) {
    brightnessMultiplier = b % 101;
}

/** generates a colour (3 element int array) from 3 arguments
 *
 * @param red red value
 * @param green green value
 * @param blue blue value
 */
int *rgbLed::custom(int red, int green, int blue) {
    int customRGB[3] = {red, green, blue};
    return customRGB;
}

/** increments the counter for animations by the defined rate
 *
 * @param speed speed value from 0 to 5, referring to speed constants at the top of file
 * @param b direction for changing n, 1 is adding, -1 is taking away
 */
void rgbLed::incrementN(int speed, int direction) {
    switch (speed) {
    case SLOWEST:
        delayTime = 512;
        break;
    case SLOWER:
        delayTime = 256;
        break;
    case SLOW:
        delayTime = 128;
        break;
    case MEDIUM:
        delayTime = 64;
        break;
    case FAST:
        delayTime = 16;
        break;
    case FASTER:
        delayTime = 4;
        break;
    case FASTEST:
        delayTime = 2;
        break;
    default:
        break;
    }
    vTaskDelay(delayTime);
    n += 5 * direction;
}

/** Gives back the weighted average of two colours
 *
 * @param red1 first red value
 * @param red2 second red value
 * @param green1 first green value
 * @param green2 second green value
 * @param blue1 first blue value
 * @param blue2 second blue value
 * @param ratio ratio of the two colours, 0 is max weight on a, 1 is max weight on b
 */
int *rgbLed::mixColours(int red1, int green1, int blue1,
                        int red2, int green2, int blue2, float ratio) {
    int mixedColour[3] = {mixValues(red1, red2, ratio),
                          mixValues(green1, green2, ratio),
                          mixValues(blue1, blue2, ratio)};
    return mixedColour;
}

/** Gives back the weighted average of two integers
 *
 * @param a first value
 * @param b second value
 * @param ratio ratio of the two numbers, 0 is max weight on a, 1 is max weight on b
 */
int rgbLed::mixValues(int a, int b, float ratio) {
    if (ratio >= 0 && ratio <= 1) {
        return a * (1 - ratio) + b * (ratio);
    } else if (ratio > 1) {
        return a;
    } else {
        return b;
    }
}

/** Gives a value from 0 to 1 based on the angle given
 *
 * @param angle angle in degrees
 */
float rgbLed::sin1(float angle) {
    return sinMuti(angle, 1);
}

/** Gives a value from 0 to 255 based on the angle given
 *
 * @param angle angle in degrees
 */
float rgbLed::sin255(float angle) {
    return sinMuti(angle, 255);
}

/** Gives a value from 0 to multiplier based on the angle given
 *
 * @param angle angle in degrees
 * @param multiplier maximum value
 */
float rgbLed::sinMuti(float angle, float multiplier) {
    multiplier /= 2;
    float rad = angle * PI / 180;
    return multiplier + sin(rad) * multiplier;
}

/** Submits the current leds list and it shows up on the strip */
void rgbLed::show() {
    // Serial.printf("%d\n", millis() - curTime);
    // curTime = millis();
    FastLED.show(255);
}

/** Set a specified LED's colour
 *
 * Takes R, G and B values, and applies the colour to the specified LED, also considering the brightness value
 *
 * @param led index of the LED we are setting
 * @param red red colour value 0-255
 * @param green green colour value 0-255
 * @param blue blue colour value 0-255
 */
void rgbLed::setLedColour(int led, int red, int green, int blue) {
    float multiplier = brightnessMultiplier / 100.0f;
    // Serial.printf("%d\n",red);
    // Serial.printf("%f\n",(red % 256) * multiplier);
    leds[led % NUM_LEDS] = CRGB((red % 256) * multiplier, (green % 256) * multiplier, (blue % 256) * multiplier);
    // leds[led % NUM_LEDS] = CRGB(r, g, b);
    show();
}
