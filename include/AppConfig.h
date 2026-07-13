#pragma once

#include <Arduino.h>

namespace AppConfig
{
    constexpr int Mp3RxPin = 16;   // ESP32 RX2 -> MP3 TX
    constexpr int Mp3TxPin = 17;   // ESP32 TX2 <- MP3 RX
    constexpr int Mp3BusyPin = 27; // LOW while playing, HIGH while idle

    constexpr uint32_t DebugBaud = 115200;
    constexpr uint32_t Mp3Baud = 9600;

    constexpr int StartupVolume = 30;
    constexpr int StartupTrack = 1;
    constexpr uint32_t Mp3StartupDelayMs = 1500;
    constexpr uint32_t SelectTfCardDelayMs = 200;
}
