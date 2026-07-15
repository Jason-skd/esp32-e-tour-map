#pragma once

#include <Arduino.h>

#include "beacon/BeaconTypes.h"

class BLEAdvertisedDevice;
class BLEAdvertisedDeviceCallbacks;
class BLEScan;

struct BeaconScannerConfig
{
    const char *targetUuid;
    bool activeScan;
    bool wantDuplicates;
    uint16_t intervalMs;
    uint16_t windowMs;
};

class BeaconScanner
{
public:
    explicit BeaconScanner(const BeaconScannerConfig &config);

    void begin();
    BeaconScanResult scanOnce(uint32_t durationSeconds);

private:
    friend class BeaconScannerCallbacks;

    void resetResult(BeaconScanStatus status);
    void handleAdvertisedDevice(BLEAdvertisedDevice advertisedDevice);
    void appendObservation(const BeaconObservation &observation);

    BeaconScannerConfig config_;
    String targetUuid_;
    BLEScan *scan_;
    BLEAdvertisedDeviceCallbacks *callbacks_;
    BeaconScanResult currentResult_;
};
