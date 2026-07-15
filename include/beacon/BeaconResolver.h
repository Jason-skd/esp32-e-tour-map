#pragma once

#include <Arduino.h>

#include "beacon/BeaconCatalog.h"
#include "beacon/BeaconTypes.h"

struct ResolvedBeacon
{
    BeaconIdentity identity;
    int trackNumber;
    const char *label;
    int rssi;
    int averageRssi;
    int maxRssi;
    int txPower;
    uint32_t seenAtMs;
    size_t sampleCount;
};

struct BeaconFrame
{
    BeaconScanStatus status;
    size_t rawCount;
    size_t count;
    size_t droppedCount;
    ResolvedBeacon beacons[MaxKnownBeacons];

    const ResolvedBeacon *best() const;
    const ResolvedBeacon *find(const BeaconIdentity &identity) const;
};

class BeaconResolver
{
public:
    explicit BeaconResolver(const BeaconCatalog &catalog);

    BeaconFrame resolve(const BeaconScanResult &scanResult) const;

private:
    const BeaconCatalog &catalog_;
};
