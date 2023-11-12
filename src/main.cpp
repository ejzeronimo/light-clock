/*!
    @file main.cpp
    @brief Main application implementation
*/

#include <Arduino.h>

// wifi includes
#include <SPI.h>
#include <WiFiNINA.h>
// wifi config includes
#include "wifiSecrets.h"
// helper includes
#include "LedHelper.hpp"
#include "TimeHelper.hpp"

/// @brief The black color for the sky
#define COLOR_SKY_BLACK  CRGB(0, 0, 0)
/// @brief The pink color for the sky
#define COLOR_SKY_PINK   CRGB(253, 77, 58)
/// @brief The orange color for the sky
#define COLOR_SKY_ORANGE CRGB(254, 107, 2)
/// @brief The blue color for the sky
#define COLOR_SKY_BLUE   CRGB(37, 47, 108)
/// @brief The purple color for the sky
#define COLOR_SKY_PURPLE CRGB(34, 30, 62)
/// @brief The yellow color for the sun
#define COLOR_SUN        CRGB(234, 232, 58)
/// @brief The white color for the moon
#define COLOR_MOON       CRGB(120, 120, 120)

/// @brief Status of the wifi connection
int                status = WL_IDLE_STATUS;
/// @brief Wifi client for the NTP client
static WiFiClient  wifi;
/// @brief Current time
static TimeInfo_t  currentTime;
/// @brief Sunrise time
static TimeEvent_t sunriseTime;
/// @brief Sunset time
static TimeEvent_t sunsetTime;
/// @brief Color for the sky, set by the time of day
static CRGB        skyColor;

// NOTE: Local Definitions
/*!
    @brief Generates a color based on a rang of times and colors
    @param startTime The time in seconds for the range to start at
    @param endTime The time in seconds for the range to end at
    @param startColor The color for the range to start at
    @param endColor The color for the range to end at
    @returns The resulting color
*/
CRGB blendByTime(TimeInfo_t startTime, TimeInfo_t endTime, CRGB startColor, CRGB endColor);

/*!
    @brief Draws a "sphere" on a strand of pixels
    @param startTime The time in seconds for the range to start at
    @param endTime The time in seconds for the range to end at
    @param sphereColor The color for the sphere
    @param sphereDiameter Diameter in pixels for the sphere
*/
void drawSphereBasedOffTime(TimeInfo_t startTime, TimeInfo_t endTime, CRGB sphereColor, uint8_t sphereDiameter);

// NOTE: Global Functions
void setup()
{
    Serial.begin(9600);

    Serial.println("========= Starting Up =========");

    while(status != WL_CONNECTED)
    {
        Serial.print("Attempting to connect to WPA SSID: ");
        Serial.println(SECRET_SSID);

        status = WiFi.begin(SECRET_SSID, SECRET_PASS);

        delay(5000);
    }

    Serial.println("Connected to the network");

    Serial.println("Starting LEDs");
    startLeds();

    Serial.println("Starting Time client");
    startNtpService(wifi);
}

void loop()
{
    currentTime = getCurrentTime();
    sunriseTime = getSunrise();
    sunsetTime  = getSunset();

    Serial.println("======== Current State ========");

    // based off time in minutes, set state
    if(currentTime.timeInMinutes >= 0 && currentTime.timeInMinutes < sunriseTime.start.timeInMinutes)
    {
        // lights off default case
        Serial.println("State: Lights off");
        skyColor = COLOR_SKY_BLACK;
    }
    else if(currentTime.timeInMinutes >= sunriseTime.start.timeInMinutes && currentTime.timeInMinutes < sunriseTime.end.timeInMinutes)
    {
        // need to make the lights get bright for 30 min around sunrise
        Serial.println("State: Sunrise");
        skyColor = COLOR_SKY_BLACK;

        // black -> pink -> orange
        if(currentTime.timeInMinutes < sunriseTime.actual.timeInMinutes)
        {
            Serial.println("Blend: black -> pink");
            skyColor = blendByTime(sunriseTime.start, sunriseTime.actual, COLOR_SKY_BLACK, COLOR_SKY_PINK);
        }
        else
        {
            Serial.println("Blend: pink -> orange");
            skyColor = blendByTime(sunriseTime.actual, sunriseTime.end, COLOR_SKY_PINK, COLOR_SKY_ORANGE);
        }
    }
    else if(currentTime.timeInMinutes >= sunriseTime.end.timeInMinutes && currentTime.timeInMinutes < sunsetTime.start.timeInMinutes)
    {
        // sun in sky, blue bg
        Serial.println("State: Daylight");
        skyColor = COLOR_SKY_BLUE;

        TimeInfo_t afterSunrise = {
            .timeInSeconds = sunriseTime.end.timeInSeconds + (5 * 60),
            .timeInMinutes = sunriseTime.end.timeInMinutes + 5,
        };

        // orange -> blue
        if(currentTime.timeInMinutes < afterSunrise.timeInMinutes)
        {
            Serial.println("Blend: orange -> blue");
            skyColor = blendByTime(sunriseTime.end, afterSunrise, COLOR_SKY_ORANGE, COLOR_SKY_BLUE);
        }
    }
    else if(currentTime.timeInMinutes >= sunsetTime.start.timeInMinutes && currentTime.timeInMinutes < sunsetTime.end.timeInMinutes)
    {
        // need to make the lights go dim for 30 min around sunset
        Serial.println("State: Sunset ");
        skyColor = COLOR_SKY_BLUE;

        // blue -> orange -> pink
        if(currentTime.timeInMinutes < sunsetTime.actual.timeInMinutes)
        {
            Serial.println("Blend: blue -> orange");
            skyColor = blendByTime(sunsetTime.start, sunsetTime.actual, COLOR_SKY_BLUE, COLOR_SKY_ORANGE);
        }
        else
        {
            Serial.println("Blend: orange -> pink");
            skyColor = blendByTime(sunsetTime.actual, sunsetTime.end, COLOR_SKY_ORANGE, COLOR_SKY_PINK);
        }
    }
    else if(currentTime.timeInMinutes >= sunsetTime.end.timeInMinutes && currentTime.timeInMinutes < endOfDay.timeInMinutes)
    {
        // just purple fading to black after sunset till midnight
        Serial.println("State: Twilight");
        skyColor = COLOR_SKY_PURPLE;

        TimeInfo_t afterSunset = {
            .timeInSeconds = sunsetTime.end.timeInSeconds + (5 * 60),
            .timeInMinutes = sunsetTime.end.timeInMinutes + 5,
        };

        TimeInfo_t beforeMidnight = {
            .timeInSeconds = endOfDay.timeInSeconds - (5 * 60),
            .timeInMinutes = endOfDay.timeInMinutes - 5,
        };

        // pink -> purple -> black
        if(currentTime.timeInMinutes < afterSunset.timeInMinutes)
        {
            Serial.println("Blend: pink -> purple");
            skyColor = blendByTime(sunsetTime.end, afterSunset, COLOR_SKY_PINK, COLOR_SKY_PURPLE);
        }
        else if(currentTime.timeInMinutes >= beforeMidnight.timeInMinutes)
        {
            Serial.println("Blend: purple -> black");
            skyColor = blendByTime(beforeMidnight, endOfDay, COLOR_SKY_PURPLE, COLOR_SKY_BLACK);
        }
    }

    fill_solid(leds.pixels, leds.length, skyColor);

    // draw the sun / moon
    if(currentTime.timeInMinutes >= sunriseTime.start.timeInMinutes && currentTime.timeInMinutes < sunsetTime.end.timeInMinutes)
    {
        drawSphereBasedOffTime(sunriseTime.start, sunsetTime.end, COLOR_SUN, 19);
    }
    else if(currentTime.timeInMinutes >= sunsetTime.end.timeInMinutes && currentTime.timeInMinutes < endOfDay.timeInMinutes)
    {
        drawSphereBasedOffTime(sunsetTime.end, endOfDay, COLOR_MOON, 19);
    }  

    renderStrip();
    printTime();

    // grab the new time for sunrise and sunset after the day is over
    updateNtpService();
    delay(5000);
}

// NOTE: Local Functions
CRGB blendByTime(TimeInfo_t startTime, TimeInfo_t endTime, CRGB startColor, CRGB endColor)
{
    uint32_t duration      = endTime.timeInSeconds - startTime.timeInSeconds;
    float_t  rawBlendValue = (float)(currentTime.timeInSeconds - startTime.timeInSeconds) / duration;
    uint8_t  blendValue    = rawBlendValue * 255;

    Serial.print("Blend value: ");
    Serial.println(blendValue);

    return blend(startColor, endColor, blendValue);
}

void drawSphereBasedOffTime(TimeInfo_t startTime, TimeInfo_t endTime, CRGB sphereColor, uint8_t sphereDiameter)
{
    uint32_t duration         = endTime.timeInSeconds - startTime.timeInSeconds;
    float_t  rawPositionValue = (float)(currentTime.timeInSeconds - startTime.timeInSeconds) / duration;

    int16_t middlePixelPos = ((1 - rawPositionValue) * (leds.length + sphereDiameter)) - (sphereDiameter / 2);
    uint8_t startPixelPos  = middlePixelPos - (sphereDiameter / 2) + 3;
    uint8_t endPixelPos    = middlePixelPos + (sphereDiameter / 2) - 3;

    uint8_t startBufferPos = middlePixelPos - (sphereDiameter / 2);
    uint8_t endBufferPos   = middlePixelPos + (sphereDiameter / 2);

    Serial.print("Sphere pos: ");
    Serial.println(middlePixelPos);

    for(size_t i = 0; i < leds.length; i++)
    {
        if(i >= startPixelPos && i <= endPixelPos)
        {
            leds.pixels[i] = sphereColor;
        }
        else if(i >= startBufferPos && i <= endBufferPos)
        {
            int16_t temp          = middlePixelPos - i;
            float_t rawBlendValue = (float)(((sphereDiameter / 2) - abs(temp)) + 1) / 4;
            uint8_t blendValue    = (rawBlendValue)*255;

            leds.pixels[i] = blend(leds.pixels[i], sphereColor, blendValue);
        }
    }
}