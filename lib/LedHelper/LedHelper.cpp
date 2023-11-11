/*!
    @file LedHelper.cpp
    @brief LED helper implementation
*/

#include "LedHelper.hpp"

LightStrip_t leds = {
    .length = LEFT_PIXEL_LENGTH + RIGHT_PIXEL_LENGTH,
    .pixels = new CRGB[LEFT_PIXEL_LENGTH + RIGHT_PIXEL_LENGTH],
};

/// @brief left LED strip
static CRGB stripLeft[LEFT_PIXEL_LENGTH];
/// @brief right LED strip
static CRGB stripRight[RIGHT_PIXEL_LENGTH];

// NOTE: Global Functions
void startLeds(void)
{
    FastLED.addLeds<NEOPIXEL, LEFT_PIXEL_PIN>(stripLeft, LEFT_PIXEL_LENGTH).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<NEOPIXEL, RIGHT_PIXEL_PIN>(stripRight, RIGHT_PIXEL_LENGTH).setCorrection(TypicalLEDStrip);

    // measureLength();
    fill_solid(leds.pixels, leds.length, CRGB(0, 0, 0));
    renderStrip();
}

void measureLength(void)
{
    for(size_t i = 0; i < 100; i++)
    {
        CRGB tempColor;

        if(i % 10 == 0)
        {
            tempColor.r = random() * 255;
            tempColor.g = random() * 255;
            tempColor.b = random() * 255;
        }

        stripLeft[i]  = tempColor;
        stripRight[i] = tempColor;
    }

    FastLED.show();
}

void renderStrip(void)
{
    for(size_t i = 0; i < leds.length; i++)
    {
        if(i < LEFT_PIXEL_LENGTH)
        {
            stripLeft[(LEFT_PIXEL_LENGTH - 1) - i] = leds.pixels[i];
        }
        else
        {
            stripRight[i - LEFT_PIXEL_LENGTH] = leds.pixels[i];
        }
    }

    FastLED.show();
}