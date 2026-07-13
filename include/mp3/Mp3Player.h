#pragma once

#include <Arduino.h>

enum class Mp3Error
{
    Ok,
    InvalidVolume,
    InvalidTrackNumber,
};

const char *mp3ErrorToString(Mp3Error error);

struct TrackNameResult
{
    Mp3Error error;
    char value[14];
};

class Mp3Player
{
public:
    Mp3Player(HardwareSerial &serial, int rxPin, int txPin, int busyPin);

    void begin(uint32_t baud);
    void selectTfCard();
    Mp3Error setVolume(int volume);
    TrackNameResult formatTrackName(int trackNumber) const;
    TrackNameResult playTrack(int trackNumber);
    void stop();
    bool isPlaying() const;
    void printResponses(Stream &out);

private:
    void sendCommand(uint8_t command, uint16_t parameter, bool feedback = false);

    HardwareSerial &serial_;
    int rxPin_;
    int txPin_;
    int busyPin_;
};
