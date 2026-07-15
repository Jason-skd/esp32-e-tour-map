#include "beacon/BeaconScanner.h"

#include <BLEAdvertisedDevice.h>
#include <BLEBeacon.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEUtils.h>

#include <string>

class BeaconScannerCallbacks : public BLEAdvertisedDeviceCallbacks
{
public:
    explicit BeaconScannerCallbacks(BeaconScanner &scanner)
        : scanner_(scanner)
    {
    }

    void onResult(BLEAdvertisedDevice advertisedDevice) override
    {
        scanner_.handleAdvertisedDevice(advertisedDevice);
    }

private:
    BeaconScanner &scanner_;
};

namespace
{
    bool isIBeaconManufacturerData(const uint8_t *data, size_t length)
    {
        return length == 25 &&
               data[0] == 0x4C &&
               data[1] == 0x00 &&
               data[2] == 0x02 &&
               data[3] == 0x15;
    }

    uint16_t readBigEndianUint16(const uint8_t *data)
    {
        return (static_cast<uint16_t>(data[0]) << 8) | data[1];
    }
}

BeaconScanner::BeaconScanner(const BeaconScannerConfig &config)
    : config_(config),
      targetUuid_(config.targetUuid == nullptr ? "" : config.targetUuid),
      scan_(nullptr),
      callbacks_(nullptr)
{
    resetResult(BeaconScanStatus::NotStarted);
}

void BeaconScanner::begin()
{
    BLEDevice::init("");

    scan_ = BLEDevice::getScan();
    if (callbacks_ == nullptr)
    {
        callbacks_ = new BeaconScannerCallbacks(*this);
    }

    scan_->setAdvertisedDeviceCallbacks(callbacks_, config_.wantDuplicates);
    scan_->setActiveScan(config_.activeScan);
    scan_->setInterval(config_.intervalMs);
    scan_->setWindow(config_.windowMs);

    resetResult(BeaconScanStatus::Ok);
}

BeaconScanResult BeaconScanner::scanOnce(uint32_t durationSeconds)
{
    if (durationSeconds == 0)
    {
        resetResult(BeaconScanStatus::ParameterError);
        return currentResult_;
    }

    if (scan_ == nullptr)
    {
        resetResult(BeaconScanStatus::NotStarted);
        return currentResult_;
    }

    resetResult(BeaconScanStatus::Ok);
    scan_->start(durationSeconds, false);
    scan_->clearResults();

    return currentResult_;
}

void BeaconScanner::resetResult(BeaconScanStatus status)
{
    currentResult_.status = status;
    currentResult_.count = 0;
    currentResult_.droppedCount = 0;
}

void BeaconScanner::handleAdvertisedDevice(BLEAdvertisedDevice advertisedDevice)
{
    if (!advertisedDevice.haveManufacturerData())
    {
        return;
    }

    std::string manufacturerData = advertisedDevice.getManufacturerData();
    uint8_t data[25];

    if (manufacturerData.length() != sizeof(data))
    {
        return;
    }

    manufacturerData.copy(reinterpret_cast<char *>(data), sizeof(data), 0);

    if (!isIBeaconManufacturerData(data, sizeof(data)))
    {
        return;
    }

    BLEBeacon beacon;
    beacon.setData(manufacturerData);

    String uuid = beacon.getProximityUUID().toString().c_str();
    if (targetUuid_.length() > 0 && !uuid.equalsIgnoreCase(targetUuid_))
    {
        return;
    }

    BeaconObservation observation;
    observation.identity.major = readBigEndianUint16(&data[20]);
    observation.identity.minor = readBigEndianUint16(&data[22]);
    observation.rssi = advertisedDevice.getRSSI();
    observation.txPower = beacon.getSignalPower();
    observation.seenAtMs = millis();

    appendObservation(observation);
}

void BeaconScanner::appendObservation(const BeaconObservation &observation)
{
    if (currentResult_.count >= MaxBeaconObservations)
    {
        ++currentResult_.droppedCount;
        return;
    }

    currentResult_.observations[currentResult_.count] = observation;
    ++currentResult_.count;
}
