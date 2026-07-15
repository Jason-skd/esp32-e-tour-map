#include "console/BeaconConsoleController.h"

BeaconConsoleController::BeaconConsoleController(Stream &console, BeaconScanner &scanner, BeaconResolver &resolver)
    : console_(console), scanner_(scanner), resolver_(resolver)
{
}

void BeaconConsoleController::scan(uint32_t durationSeconds)
{
    console_.print("Scanning target iBeacon for ");
    console_.print(durationSeconds);
    console_.println(" seconds...");

    BeaconScanResult scanResult = scanner_.scanOnce(durationSeconds);
    BeaconFrame frame = resolver_.resolve(scanResult);

    printScanResult(scanResult);
    printFrame(frame);
}

void BeaconConsoleController::printScanResult(const BeaconScanResult &scanResult)
{
    console_.print("beacon_scan,status=");
    console_.print(statusToString(scanResult.status));
    console_.print(",raw=");
    console_.print(static_cast<unsigned long>(scanResult.count));
    console_.print(",dropped=");
    console_.println(static_cast<unsigned long>(scanResult.droppedCount));

    for (size_t i = 0; i < scanResult.count; ++i)
    {
        const BeaconObservation &observation = scanResult.observations[i];
        console_.print("beacon_raw,");
        console_.print(observation.identity.major);
        console_.print(",");
        console_.print(observation.identity.minor);
        console_.print(",rssi=");
        console_.print(observation.rssi);
        console_.print(",tx=");
        console_.print(observation.txPower);
        console_.print(",seen_ms=");
        console_.println(static_cast<unsigned long>(observation.seenAtMs));
    }
}

void BeaconConsoleController::printFrame(const BeaconFrame &frame)
{
    console_.print("beacon_frame,status=");
    console_.print(statusToString(frame.status));
    console_.print(",known=");
    console_.print(static_cast<unsigned long>(frame.count));
    console_.print(",raw=");
    console_.print(static_cast<unsigned long>(frame.rawCount));
    console_.print(",dropped=");
    console_.println(static_cast<unsigned long>(frame.droppedCount));

    const ResolvedBeacon *bestBeacon = frame.best();
    for (size_t i = 0; i < frame.count; ++i)
    {
        const ResolvedBeacon &beacon = frame.beacons[i];
        const bool isBest = bestBeacon != nullptr && sameBeaconIdentity(bestBeacon->identity, beacon.identity);

        console_.print("beacon_resolved,");
        console_.print(beacon.identity.major);
        console_.print(",");
        console_.print(beacon.identity.minor);
        console_.print(",track=");
        console_.print(beacon.trackNumber);
        console_.print(",label=");
        console_.print(beacon.label);
        console_.print(",samples=");
        console_.print(static_cast<unsigned long>(beacon.sampleCount));
        console_.print(",rssi=");
        console_.print(beacon.rssi);
        console_.print(",avg=");
        console_.print(beacon.averageRssi);
        console_.print(",max=");
        console_.print(beacon.maxRssi);
        console_.print(",best=");
        console_.println(isBest ? 1 : 0);
    }
}

const char *BeaconConsoleController::statusToString(BeaconScanStatus status) const
{
    switch (status)
    {
    case BeaconScanStatus::Ok:
        return "ok";
    case BeaconScanStatus::NotStarted:
        return "not_started";
    case BeaconScanStatus::ParameterError:
        return "parameter_error";
    default:
        return "unknown";
    }
}
