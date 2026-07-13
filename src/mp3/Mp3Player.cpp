#include "mp3/Mp3Player.h"

#include <stdio.h>

const char *mp3ErrorToString(Mp3Error error)
{
    switch (error)
    {
    case Mp3Error::Ok:
        return "ok";
    case Mp3Error::InvalidVolume:
        return "invalid volume";
    case Mp3Error::InvalidTrackNumber:
        return "invalid track number";
    default:
        return "unknown error";
    }
}

Mp3Player::Mp3Player(HardwareSerial &serial, int rxPin, int txPin, int busyPin)
    : serial_(serial), rxPin_(rxPin), txPin_(txPin), busyPin_(busyPin)
{
}

void Mp3Player::begin(uint32_t baud)
{
    pinMode(busyPin_, INPUT_PULLUP);
    serial_.begin(baud, SERIAL_8N1, rxPin_, txPin_);
}

void Mp3Player::selectTfCard()
{
    // 0x09 = select playback device, 0x0002 = TF / SD card.
    sendCommand(0x09, 0x0002);
}

Mp3Error Mp3Player::setVolume(int volume)
{
    if (volume < 0 || volume > 30)
    {
        return Mp3Error::InvalidVolume;
    }

    // 0x06 = set volume.
    sendCommand(0x06, static_cast<uint16_t>(volume));
    return Mp3Error::Ok;
}

TrackNameResult Mp3Player::formatTrackName(int trackNumber) const
{
    TrackNameResult result;
    result.error = Mp3Error::Ok;
    result.value[0] = '\0';

    if (trackNumber < 1 || trackNumber > 9999)
    {
        result.error = Mp3Error::InvalidTrackNumber;
        return result;
    }

    snprintf(result.value, sizeof(result.value), "/MP3/%04d.mp3", trackNumber);
    return result;
}

TrackNameResult Mp3Player::playTrack(int trackNumber)
{
    TrackNameResult trackName = formatTrackName(trackNumber);
    if (trackName.error != Mp3Error::Ok)
    {
        return trackName;
    }

    // 0x12 = play track from the MP3 folder.
    sendCommand(0x12, static_cast<uint16_t>(trackNumber));
    return trackName;
}

void Mp3Player::stop()
{
    // 0x16 = stop playback.
    sendCommand(0x16, 0);
}

bool Mp3Player::isPlaying() const
{
    return digitalRead(busyPin_) == LOW;
}

void Mp3Player::printResponses(Stream &out)
{
    while (serial_.available())
    {
        uint8_t value = serial_.read();

        out.print("MP3 response: 0x");
        if (value < 0x10)
        {
            out.print("0");
        }
        out.println(value, HEX);
    }
}

void Mp3Player::sendCommand(uint8_t command, uint16_t parameter, bool feedback)
{
    uint8_t frame[10];

    frame[0] = 0x7E;
    frame[1] = 0xFF;
    frame[2] = 0x06;
    frame[3] = command;
    frame[4] = feedback ? 0x01 : 0x00;
    frame[5] = (parameter >> 8) & 0xFF;
    frame[6] = parameter & 0xFF;

    uint16_t checksum = 0;
    for (int i = 1; i <= 6; ++i)
    {
        checksum += frame[i];
    }
    checksum = 0 - checksum;

    frame[7] = (checksum >> 8) & 0xFF;
    frame[8] = checksum & 0xFF;
    frame[9] = 0xEF;

    serial_.write(frame, sizeof(frame));
}
