/*!
    @file TimeHelper.hpp
    @brief Time helper definition
*/

#ifndef TimeHelper_hpp_INCLUDED
#define TimeHelper_hpp_INCLUDED

// C include
#include <stdint.h>
// HTTP include
#include <ArduinoHttpClient.h>

/*!
    @brief Hours to offset the time
    @remarks Eastern Standard Time (USA)
*/
#define TIMEZONE_OFFSET      -5
/// @brief Time in minutes to surround and event with
#define EVENT_BUFFER_MINUTES 15U
/// @brief NTP time is in the first 48 bytes of message
#define NTP_PACKET_SIZE      48
/// @brief Port for the UDP client
#define NTP_UDP_PORT         8888U
/// @brief Root URL for the REST API
#define REST_API_SERVER      "api.sunrise-sunset.org"
/// @brief Endpoint URI for the REST API
#define REST_API_ENDPOINT    "/json?lat=39.9914391&lng=-86.0546511&formatted=0"
/// @brief Port for the REST API
#define REST_API_PORT        80U

// NOTE: Global Types
/// @brief Struct to hold types of time info
typedef struct TimeInfo_t
{
    /// @brief Time converted to seconds
    uint32_t timeInSeconds;
    /// @brief Time converted to minutes
    uint32_t timeInMinutes;
} TimeInfo_t;

/// @brief Struct to hold times realted to an event
typedef struct TimeEvent_t
{
    /// @brief Start time of the event
    TimeInfo_t start;
    /// @brief Actual time of the event
    TimeInfo_t actual;
    /// @brief End time of the event
    TimeInfo_t end;
} TimeEvent_t;

// NOTE: Global Variables
const TimeInfo_t endOfDay = {
    .timeInSeconds = 86400,
    .timeInMinutes = 1440,
};

// NOTE: Global Definitions
/*!
    @brief Starts the NTP service
    @param httpClient The WiFi client to use for HTTP
*/
void startNtpService(Client & httpClient);

/// @brief Updates the sunrise/sunset times
void updateNtpService(void);


/// @brief Prints the time
void printTime(void);

/*!
    @brief Get the current time
    @returns The time data
*/
TimeInfo_t getCurrentTime(void);

/*!
    @brief Get the sunrise info for the day
    @returns The event data
*/
TimeEvent_t getSunrise(void);

/*!
    @brief Get the sunset info for the day
    @returns The event data
*/
TimeEvent_t getSunset(void);

#endif /* TimeHelper_hpp_INCLUDED */
