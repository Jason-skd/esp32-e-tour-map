#pragma once

#include <Arduino.h>

#include "Mp3Player.h"

class Mp3ConsoleController
{
public:
    Mp3ConsoleController(Stream &console, Mp3Player &mp3Player);

    void selectTfCard();
    void setVolume(int volume);
    void playTrack(int trackNumber);
    void playStartupTrack(int trackNumber);
    void stop();
    void printBusyState();
    void printResponses();

private:
    void printError(Mp3Error error);

    Stream &console_;
    Mp3Player &mp3Player_;
};
