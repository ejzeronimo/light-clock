/*!
    @file TimeHelper.cpp
    @brief Time helper implementation
*/

#include "TimeHelper.hpp"

// time lib
#include <TimeLib.h>
// UDP include
#include <WiFiUdp.h>
// JSON include
#include <ArduinoJson.h>

/*!
    @brief IP for a NTP server
    @remarks pool.ntp.org
*/
static IPAddress           timeServer(162, 159, 200, 123);
/// @brief Buffer to hold incoming & outgoing packets
static uint8_t             packetBuffer[NTP_PACKET_SIZE];
/// @brief UDP client instance
static WiFiUDP             Udp;
/// @brief HTTP client instance
static HttpClient *        client;
/// @brief Dynamic doc to store the JSON result
static DynamicJsonDocument doc(1024);
/// @brief Values for the sunrise/sunset
static uint32_t            sunriseHour, sunriseMinute, sunsetHour, sunsetMinute;

// NOTE: Local Definitions
/*!
    @brief Gets the time from an UDP packet
    @returns The time recieved or 0
*/
time_t getNtpTime(void);

/// @brief Sends the request to a NTP server for the time
void sendNTPpacket(IPAddress & address);

void getRiseSetTimes(void);

// NOTE: Global Functions
void startNtpService(Client & httpClient)
{
    Udp.begin(NTP_UDP_PORT);

    client = new HttpClient(httpClient, REST_API_SERVER, REST_API_PORT);

    setSyncProvider(getNtpTime);

    getRiseSetTimes();
}

void updateNtpService(void)
{
    if((hour() == 0) && (minute() == 0) && (second() <= 15))
    {
        getRiseSetTimes();
    }
}

void printTime(void)
{
    Serial.print("24hr Time: ");
    Serial.print(hour());
    Serial.print(":");
    Serial.println(minute());
}

TimeInfo_t getCurrentTime(void)
{
    uint32_t minutes = (hour() * 60) + minute();

    return {
        .timeInSeconds = (minutes * 60) + second(),
        .timeInMinutes = minutes,
    };
}

TimeEvent_t getSunrise(void)
{
    uint32_t minutes = (sunriseHour * 60) + sunriseMinute;

    return {
        .start = {
            .timeInSeconds = (minutes - EVENT_BUFFER_MINUTES) * 60,
            .timeInMinutes = minutes - EVENT_BUFFER_MINUTES,
        },
        .actual = {
            .timeInSeconds = minutes * 60,
            .timeInMinutes = minutes,
        },
        .end = {
            .timeInSeconds = (minutes + EVENT_BUFFER_MINUTES) * 60,
            .timeInMinutes = minutes + EVENT_BUFFER_MINUTES,
        },
    };
}

TimeEvent_t getSunset(void)
{
    uint32_t minutes = (sunsetHour * 60) + sunsetMinute;

    return {
        .start = {
            .timeInSeconds = (minutes - EVENT_BUFFER_MINUTES) * 60,
            .timeInMinutes = minutes - EVENT_BUFFER_MINUTES,
        },
        .actual = {
            .timeInSeconds = minutes * 60,
            .timeInMinutes = minutes,
        },
        .end = {
            .timeInSeconds = (minutes + EVENT_BUFFER_MINUTES) * 60,
            .timeInMinutes = minutes + EVENT_BUFFER_MINUTES,
        },
    };
}

// NOTE: Local Functions
time_t getNtpTime(void)
{
    while(Udp.parsePacket() > 0)
    {
        // discard any previously received packets
    };

    sendNTPpacket(timeServer);
    uint32_t beginWait = millis();

    while(millis() - beginWait < 1500)
    {
        int16_t size = Udp.parsePacket();

        if(size >= NTP_PACKET_SIZE)
        {
            // read packet into the buffer
            Udp.read(packetBuffer, NTP_PACKET_SIZE);

            unsigned long secsSince1900;

            // convert four bytes starting at location 40 to a long integer
            secsSince1900 = (unsigned long)packetBuffer[40] << 24;
            secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
            secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
            secsSince1900 |= (unsigned long)packetBuffer[43];

            return secsSince1900 - 2208988800UL + TIMEZONE_OFFSET * SECS_PER_HOUR;
        }
    }

    // return 0 if unable to get the time
    return 0;
}

void sendNTPpacket(IPAddress & address)
{
    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, NTP_PACKET_SIZE);

    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    packetBuffer[0] = 0b11100011; // LI, Version, Mode
    packetBuffer[1] = 0;          // Stratum, or type of clock
    packetBuffer[2] = 6;          // Polling Interval
    packetBuffer[3] = 0xEC;       // Peer Clock Precision

    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12] = 49;
    packetBuffer[13] = 0x4E;
    packetBuffer[14] = 49;
    packetBuffer[15] = 52;

    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    Udp.beginPacket(address, 123); // NTP requests are to port 123
    Udp.write(packetBuffer, NTP_PACKET_SIZE);
    Udp.endPacket();
}

void getRiseSetTimes(void)
{
    Serial.println("Querying REST API ...");

    client->get(REST_API_ENDPOINT);

    int16_t statusCode = client->responseStatusCode();
    String  response   = client->responseBody();

    if(statusCode == 200)
    {
        deserializeJson(doc, response);

        // read values
        String tempRise = doc["results"]["sunrise"].as<String>();
        String tempSet  = doc["results"]["sunset"].as<String>();

        // 2023-08-07T10:46:53+00:00
        int year, month, day, seconds, timezoneHour, timezoneMinute;
        sscanf(tempRise.c_str(), "%d-%d-%dT%lu:%lu:%d+%d:%d", &year, &month, &day, &sunriseHour, &sunriseMinute, &seconds, &timezoneHour, &timezoneMinute);
        sscanf(tempSet.c_str(), "%d-%d-%dT%lu:%lu:%d+%d:%d", &year, &month, &day, &sunsetHour, &sunsetMinute, &seconds, &timezoneHour, &timezoneMinute);

        sunriseHour = sunriseHour + TIMEZONE_OFFSET;
        sunsetHour  = sunsetHour + TIMEZONE_OFFSET;
    }
}