#include <Arduino.h>

#include "app/AppConfig.h"
#include "beacon/BeaconCatalog.h"
#include "beacon/BeaconResolver.h"
#include "beacon/BeaconScanner.h"
#include "console/BeaconConsoleController.h"
#include "console/Mp3ConsoleController.h"
#include "console/SerialCommandHandler.h"
#include "mp3/Mp3Player.h"
#include "service/AudioGuideService.h"

HardwareSerial mp3Serial(2);
Mp3Player mp3Player(mp3Serial, AppConfig::Mp3RxPin, AppConfig::Mp3TxPin, AppConfig::Mp3BusyPin);
Mp3ConsoleController mp3Controller(Serial, mp3Player);

BeaconScannerConfig beaconScannerConfig = {
    AppConfig::BeaconTargetUuid,
    AppConfig::BeaconActiveScan,
    AppConfig::BeaconWantDuplicates,
    AppConfig::BeaconScanIntervalMs,
    AppConfig::BeaconScanWindowMs,
};
BeaconScanner beaconScanner(beaconScannerConfig);
BeaconCatalog beaconCatalog;
BeaconResolver beaconResolver(beaconCatalog);
BeaconConsoleController beaconController(Serial, beaconScanner, beaconResolver);

AudioGuideServiceConfig audioGuideServiceConfig = {
    AppConfig::AudioGuideScanDurationSeconds,
    AppConfig::AudioGuideScanIntervalMs,
    AppConfig::AudioGuideReplayCooldownMs,
    AppConfig::AudioGuideBeaconPresenceTimeoutMs,
    AppConfig::AudioGuideSwitchConfirmMs,
    AppConfig::AudioGuideSwitchRssiMargin,
};
AudioGuideService audioGuideService(beaconScanner, beaconResolver, beaconCatalog, mp3Player, audioGuideServiceConfig);
SerialCommandHandler commandHandler(Serial, mp3Controller, beaconController, audioGuideService);

void setup()
{
    Serial.begin(AppConfig::DebugBaud);
    mp3Player.begin(AppConfig::Mp3Baud);

    delay(AppConfig::SerialStartupDelayMs);
    commandHandler.printHelp();

    Serial.println("Waiting for MP3 module startup...");
    delay(AppConfig::Mp3StartupDelayMs);

    mp3Controller.selectTfCard();
    delay(AppConfig::SelectTfCardDelayMs);

    mp3Controller.setVolume(AppConfig::StartupVolume);
    delay(AppConfig::SetVolumeDelayMs);

    beaconScanner.begin();
    Serial.println("Audio guide service ready.");
}

void loop()
{
    commandHandler.poll();
    audioGuideService.poll();
    mp3Controller.printResponses();
}
