#pragma once

#include <Arduino.h>

#include "console/Mp3ConsoleController.h"

class SerialCommandHandler
{
public:
    SerialCommandHandler(Stream &console, Mp3ConsoleController &mp3Controller);

    void printHelp();
    void poll();

private:
    void handleLine(String line);

    Stream &console_;
    Mp3ConsoleController &mp3Controller_;
};
