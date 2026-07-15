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

    constexpr uint32_t SerialStartupDelayMs = 500;
    constexpr uint32_t Mp3StartupDelayMs = 1500;
    constexpr uint32_t SelectTfCardDelayMs = 200;
    constexpr uint32_t SetVolumeDelayMs = 100;

    constexpr char BeaconTargetUuid[] = "e2c56db5-dffb-48d2-b060-d0f5a71096e0";
    constexpr bool BeaconActiveScan = true;
    constexpr bool BeaconWantDuplicates = true;
    constexpr uint16_t BeaconScanIntervalMs = 100;
    constexpr uint16_t BeaconScanWindowMs = 80;
    constexpr uint32_t BeaconDefaultScanSeconds = 3;

    constexpr uint32_t AudioGuideScanDurationSeconds = 1;
    constexpr uint32_t AudioGuideScanIntervalMs = 300;
    constexpr uint32_t AudioGuideReplayCooldownMs = 60000;
    constexpr uint32_t AudioGuideBeaconPresenceTimeoutMs = 20000;
    constexpr uint32_t AudioGuideSwitchConfirmMs = 5000;
    constexpr int AudioGuideSwitchRssiMargin = 0;
}
