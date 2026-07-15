#pragma once

#include <Arduino.h>

constexpr size_t MaxBeaconObservations = 32;
constexpr size_t MaxKnownBeacons = 16;
constexpr size_t MaxRssiSamplesPerBeacon = 5;

struct BeaconIdentity
{
    uint16_t major;
    uint16_t minor;
};

inline bool sameBeaconIdentity(const BeaconIdentity &left, const BeaconIdentity &right)
{
    return left.major == right.major && left.minor == right.minor;
}

struct BeaconObservation
{
    BeaconIdentity identity;
    int rssi;
    int txPower;
    uint32_t seenAtMs;
};

enum class BeaconScanStatus
{
    Ok,
    NotStarted,
    ParameterError,
};

struct BeaconScanResult
{
    BeaconScanStatus status;
    size_t count;
    size_t droppedCount;
    BeaconObservation observations[MaxBeaconObservations];
};
