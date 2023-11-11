/*!
    @file LedHelper.hpp
    @brief LED helper definition
*/

#ifndef LedHelper_hpp_INCLUDED
#define LedHelper_hpp_INCLUDED

// LED include
#include <FastLED.h>

/// @brief Length of the left strip of LEDs
#define LEFT_PIXEL_LENGTH  99U
/// @brief Pin for the left strip of LEDs
#define LEFT_PIXEL_PIN     3U
/// @brief Length of the left right of LEDs
#define RIGHT_PIXEL_LENGTH 99U
/// @brief Pin for the right strip of LEDs
#define RIGHT_PIXEL_PIN    2U

// NOTE: Global Types
/// @brief Struct to define what a strip is
typedef struct LightStrip_t
{
    /// @brief Length of the strip
    uint8_t length;
    /// @brief Time converted to minutes
    CRGB *  pixels;
} LightStrip_t;

// NOTE: Global Variables
/// @brief LED strip
extern LightStrip_t leds;

// NOTE: Global Definitions
/// @brief Inits the LEDs
void startLeds(void);

/// @brief Function to light the strips up in such a way that they can be counted
void measureLength(void);

/// @brief Converts the virtual strand to real coordinates onthe actual strands
void renderStrip(void);

#endif /* LedHelper_hpp_INCLUDED */
