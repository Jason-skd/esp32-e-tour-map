#include <Arduino.h>

#include "AppConfig.h"
#include "Mp3ConsoleController.h"
#include "Mp3Player.h"
#include "SerialCommandHandler.h"

HardwareSerial mp3Serial(2);
Mp3Player mp3Player(mp3Serial, AppConfig::Mp3RxPin, AppConfig::Mp3TxPin, AppConfig::Mp3BusyPin);
Mp3ConsoleController mp3Controller(Serial, mp3Player);
SerialCommandHandler commandHandler(Serial, mp3Controller);

void setup()
{
    Serial.begin(AppConfig::DebugBaud);
    mp3Player.begin(AppConfig::Mp3Baud);

    delay(500);
    commandHandler.printHelp();

    Serial.println("Waiting for MP3 module startup...");
    delay(AppConfig::Mp3StartupDelayMs);

    mp3Controller.selectTfCard();
    delay(AppConfig::SelectTfCardDelayMs);

    mp3Controller.setVolume(AppConfig::StartupVolume);
    delay(100);

    mp3Controller.playStartupTrack(AppConfig::StartupTrack);
}

void loop()
{
    commandHandler.poll();
    mp3Controller.printResponses();
}
