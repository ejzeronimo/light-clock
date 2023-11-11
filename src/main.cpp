/*!
    @file main.cpp
    @brief Main application implementation
*/

#include <Arduino.h>

// helper includes
#include "LedHelper.hpp"
#include "TimeHelper.hpp"

#include <SPI.h>
#include <WiFiNINA.h>

#include "wifiSecrets.h"

// wifi stuff
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int  status = WL_IDLE_STATUS;

WiFiClient wifi;

TimeInfo_t  currentTime;
TimeEvent_t sunriseTime;
TimeEvent_t sunsetTime;

CRGB skyColor;

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

// NOTE: Global Functions
void setup()
{
    Serial.begin(9600);

    while(status != WL_CONNECTED)
    {
        Serial.print("Attempting to connect to WPA SSID: ");
        Serial.println(ssid);

        status = WiFi.begin(ssid, pass);

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
        skyColor = CRGB(0, 0, 0);
    }
    else if(currentTime.timeInMinutes >= sunriseTime.start.timeInMinutes && currentTime.timeInMinutes < sunriseTime.end.timeInMinutes)
    {
        // need to make the lights get bright for 30 min around sunrise
        Serial.println("State: Sunrise");
        skyColor = CRGB(0, 0, 0);

        // black -> pink -> orange
        if(currentTime.timeInMinutes < sunriseTime.actual.timeInMinutes)
        {
            Serial.println("Blend: black -> pink");
            skyColor = blendByTime(sunriseTime.start, sunriseTime.actual, CRGB(0, 0, 0), CRGB(253, 77, 58));
        }
        else
        {
            Serial.println("Blend: pink -> orange");
            skyColor = blendByTime(sunriseTime.actual, sunriseTime.end, CRGB(253, 77, 58), CRGB(254, 107, 2));
        }
    }
    else if(currentTime.timeInMinutes >= sunriseTime.end.timeInMinutes && currentTime.timeInMinutes < sunsetTime.start.timeInMinutes)
    {
        // sun in sky, blue bg
        Serial.println("State: Daylight");
        skyColor = CRGB(37, 47, 108);

        TimeInfo_t afterSunrise = {
            .timeInSeconds = sunriseTime.end.timeInSeconds + (5 * 60),
            .timeInMinutes = sunriseTime.end.timeInMinutes + 5,
        };

        // orange -> blue
        if(currentTime.timeInMinutes < afterSunrise.timeInMinutes)
        {
            Serial.println("Blend: orange -> blue");
            skyColor = blendByTime(sunriseTime.end, afterSunrise, CRGB(254, 107, 2), CRGB(37, 47, 108));
        }
    }
    else if(currentTime.timeInMinutes >= sunsetTime.start.timeInMinutes && currentTime.timeInMinutes < sunsetTime.end.timeInMinutes)
    {
        // need to make the lights go dim for 30 min around sunset
        Serial.println("State: Sunset ");
        skyColor = CRGB(37, 47, 108);

        // blue -> orange -> pink
        if(currentTime.timeInMinutes < sunsetTime.actual.timeInMinutes)
        {
            Serial.println("Blend: blue -> orange");
            skyColor = blendByTime(sunsetTime.start, sunsetTime.actual, CRGB(37, 47, 108), CRGB(254, 107, 2));
        }
        else
        {
            Serial.println("Blend: orange -> pink");
            skyColor = blendByTime(sunsetTime.actual, sunsetTime.end, CRGB(254, 107, 2), CRGB(253, 77, 58));
        }
    }
    else if(currentTime.timeInMinutes >= sunsetTime.end.timeInMinutes && currentTime.timeInMinutes < endOfDay.timeInMinutes)
    {
        // just purple fading to black after sunset till midnight
        Serial.println("State: Twilight");
        skyColor = CRGB(34, 30, 62);

        TimeInfo_t afterSunset = {
            .timeInSeconds = sunsetTime.end.timeInSeconds + (5 * 60),
            .timeInMinutes = sunsetTime.end.timeInMinutes + 5,
        };

        TimeInfo_t beforeMidnight = {
            .timeInSeconds = endOfDay.timeInSeconds + (5 * 60),
            .timeInMinutes = endOfDay.timeInMinutes + 5,
        };

        // pink -> purple -> black
        if(currentTime.timeInMinutes < afterSunset.timeInMinutes)
        {
            Serial.println("Blend: pink -> purple");
            skyColor = blendByTime(sunsetTime.end, afterSunset, CRGB(253, 77, 58), CRGB(34, 30, 62));
        }
        else if(currentTime.timeInMinutes >= beforeMidnight.timeInMinutes)
        {
            Serial.println("Blend: purple -> black");
            skyColor = blendByTime(beforeMidnight, endOfDay, CRGB(34, 30, 62), CRGB(0, 0, 0));
        }
    }

    fill_solid(leds.pixels, leds.length, skyColor);

	// TODO: draw the sun

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