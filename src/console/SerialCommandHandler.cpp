#include "console/SerialCommandHandler.h"

SerialCommandHandler::SerialCommandHandler(Stream &console, Mp3ConsoleController &mp3Controller)
    : console_(console), mp3Controller_(mp3Controller)
{
}

void SerialCommandHandler::printHelp()
{
    console_.println();
    console_.println("MP3-TF-16P test");
    console_.println("TF card file example:");
    console_.println("  /MP3/0001.mp3");
    console_.println("  /MP3/0002.mp3");
    console_.println();
    console_.println("Serial commands:");
    console_.println("  1        play /MP3/0001.mp3");
    console_.println("  2        play /MP3/0002.mp3");
    console_.println("  15       play /MP3/0015.mp3");
    console_.println("  v 20     set volume to 20");
    console_.println("  s        stop");
    console_.println("  b        show BUSY pin state");
    console_.println();
}

void SerialCommandHandler::poll()
{
    if (!console_.available())
    {
        return;
    }

    String line = console_.readStringUntil('\n');
    handleLine(line);
}

void SerialCommandHandler::handleLine(String line)
{
    line.trim();

    if (line.length() == 0)
    {
        return;
    }

    if (line == "s")
    {
        mp3Controller_.stop();
        return;
    }

    if (line == "b")
    {
        mp3Controller_.printBusyState();
        return;
    }

    if (line.startsWith("v "))
    {
        int volume = line.substring(2).toInt();
        mp3Controller_.setVolume(volume);
        return;
    }

    int trackNumber = line.toInt();
    mp3Controller_.playTrack(trackNumber);
}
