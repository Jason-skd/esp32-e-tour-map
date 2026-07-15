#pragma once

#include <Arduino.h>

#include "beacon/BeaconResolver.h"
#include "beacon/BeaconScanner.h"

class BeaconConsoleController
{
public:
    BeaconConsoleController(Stream &console, BeaconScanner &scanner, BeaconResolver &resolver);

    void scan(uint32_t durationSeconds);

private:
    void printScanResult(const BeaconScanResult &scanResult);
    void printFrame(const BeaconFrame &frame);
    const char *statusToString(BeaconScanStatus status) const;

    Stream &console_;
    BeaconScanner &scanner_;
    BeaconResolver &resolver_;
};
