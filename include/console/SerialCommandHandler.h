#pragma once

#include <Arduino.h>

#include "console/BeaconConsoleController.h"
#include "console/Mp3ConsoleController.h"
#include "service/AudioGuideService.h"

class SerialCommandHandler
{
public:
    SerialCommandHandler(
        Stream &console,
        Mp3ConsoleController &mp3Controller,
        BeaconConsoleController &beaconController,
        AudioGuideService &audioGuideService);

    void printHelp();
    void poll();

private:
    void handleLine(String line);

    Stream &console_;
    Mp3ConsoleController &mp3Controller_;
    BeaconConsoleController &beaconController_;
    AudioGuideService &audioGuideService_;
};
