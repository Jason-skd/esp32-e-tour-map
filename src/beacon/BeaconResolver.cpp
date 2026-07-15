#include "beacon/BeaconResolver.h"

namespace
{
    struct ResolveAccumulator
    {
        ResolvedBeacon beacon;
        int samples[MaxRssiSamplesPerBeacon];
        size_t storedSampleCount;
        int rssiSum;
    };

    int averageSamples(const int *samples, size_t sampleCount)
    {
        if (sampleCount == 0)
        {
            return 0;
        }

        int sum = 0;
        for (size_t i = 0; i < sampleCount; ++i)
        {
            sum += samples[i];
        }

        return sum / static_cast<int>(sampleCount);
    }

    int trimmedAverage(const int *samples, size_t sampleCount)
    {
        if (sampleCount < MaxRssiSamplesPerBeacon)
        {
            return averageSamples(samples, sampleCount);
        }

        int sorted[MaxRssiSamplesPerBeacon];
        for (size_t i = 0; i < MaxRssiSamplesPerBeacon; ++i)
        {
            sorted[i] = samples[i];
        }

        for (size_t i = 0; i < MaxRssiSamplesPerBeacon - 1; ++i)
        {
            for (size_t j = i + 1; j < MaxRssiSamplesPerBeacon; ++j)
            {
                if (sorted[i] > sorted[j])
                {
                    int temp = sorted[i];
                    sorted[i] = sorted[j];
                    sorted[j] = temp;
                }
            }
        }

        return (sorted[1] + sorted[2] + sorted[3]) / 3;
    }
}

const ResolvedBeacon *BeaconFrame::best() const
{
    const ResolvedBeacon *bestBeacon = nullptr;

    for (size_t i = 0; i < count; ++i)
    {
        const ResolvedBeacon &beacon = beacons[i];
        if (bestBeacon == nullptr || beacon.rssi > bestBeacon->rssi)
        {
            bestBeacon = &beacon;
        }
    }

    return bestBeacon;
}

const ResolvedBeacon *BeaconFrame::find(const BeaconIdentity &identity) const
{
    for (size_t i = 0; i < count; ++i)
    {
        if (sameBeaconIdentity(beacons[i].identity, identity))
        {
            return &beacons[i];
        }
    }

    return nullptr;
}

BeaconResolver::BeaconResolver(const BeaconCatalog &catalog)
    : catalog_(catalog)
{
}

BeaconFrame BeaconResolver::resolve(const BeaconScanResult &scanResult) const
{
    BeaconFrame frame;
    frame.status = scanResult.status;
    frame.rawCount = scanResult.count;
    frame.count = 0;
    frame.droppedCount = scanResult.droppedCount;

    if (scanResult.status != BeaconScanStatus::Ok)
    {
        return frame;
    }

    ResolveAccumulator accumulators[MaxKnownBeacons];
    size_t accumulatorCount = 0;

    for (size_t i = 0; i < scanResult.count; ++i)
    {
        const BeaconObservation &observation = scanResult.observations[i];
        const BeaconDefinition *definition = catalog_.find(observation.identity);
        if (definition == nullptr)
        {
            continue;
        }

        size_t matchIndex = accumulatorCount;
        for (size_t j = 0; j < accumulatorCount; ++j)
        {
            if (sameBeaconIdentity(accumulators[j].beacon.identity, observation.identity))
            {
                matchIndex = j;
                break;
            }
        }

        if (matchIndex == accumulatorCount)
        {
            if (accumulatorCount >= MaxKnownBeacons)
            {
                ++frame.droppedCount;
                continue;
            }

            ResolvedBeacon &beacon = accumulators[accumulatorCount].beacon;
            beacon.identity = definition->identity;
            beacon.trackNumber = definition->trackNumber;
            beacon.label = definition->label;
            beacon.rssi = observation.rssi;
            beacon.averageRssi = observation.rssi;
            beacon.maxRssi = observation.rssi;
            beacon.txPower = observation.txPower;
            beacon.seenAtMs = observation.seenAtMs;
            beacon.sampleCount = 0;

            accumulators[accumulatorCount].storedSampleCount = 0;
            accumulators[accumulatorCount].rssiSum = 0;
            matchIndex = accumulatorCount;
            ++accumulatorCount;
        }

        ResolveAccumulator &entry = accumulators[matchIndex];
        ++entry.beacon.sampleCount;
        entry.rssiSum += observation.rssi;

        if (entry.storedSampleCount < MaxRssiSamplesPerBeacon)
        {
            entry.samples[entry.storedSampleCount] = observation.rssi;
            ++entry.storedSampleCount;
        }

        if (observation.rssi > entry.beacon.maxRssi)
        {
            entry.beacon.maxRssi = observation.rssi;
            entry.beacon.txPower = observation.txPower;
        }

        if (static_cast<uint32_t>(observation.seenAtMs - entry.beacon.seenAtMs) < 0x80000000UL)
        {
            entry.beacon.seenAtMs = observation.seenAtMs;
        }
    }

    for (size_t i = 0; i < accumulatorCount; ++i)
    {
        ResolveAccumulator &entry = accumulators[i];
        entry.beacon.averageRssi =
            entry.beacon.sampleCount == 0 ? 0 : entry.rssiSum / static_cast<int>(entry.beacon.sampleCount);
        entry.beacon.rssi = trimmedAverage(entry.samples, entry.storedSampleCount);
        frame.beacons[frame.count] = entry.beacon;
        ++frame.count;
    }

    return frame;
}
