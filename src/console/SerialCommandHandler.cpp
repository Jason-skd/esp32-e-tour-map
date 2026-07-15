#include "console/SerialCommandHandler.h"

#include "app/AppConfig.h"

SerialCommandHandler::SerialCommandHandler(
    Stream &console,
    Mp3ConsoleController &mp3Controller,
    BeaconConsoleController &beaconController,
    AudioGuideService &audioGuideService)
    : console_(console),
      mp3Controller_(mp3Controller),
      beaconController_(beaconController),
      audioGuideService_(audioGuideService)
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
    console_.println("  scan     scan target iBeacon");
    console_.println("  scan 5   scan target iBeacon for 5 seconds");
    console_.println("  state    show audio guide state and last_seen table");
    console_.println("  reset    reset audio guide state");
    console_.println("  h        show help");
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

    if (line == "h")
    {
        printHelp();
        return;
    }

    if (line == "scan")
    {
        beaconController_.scan(AppConfig::BeaconDefaultScanSeconds);
        return;
    }

    if (line.startsWith("scan "))
    {
        long durationSeconds = line.substring(5).toInt();
        beaconController_.scan(durationSeconds <= 0 ? 0 : static_cast<uint32_t>(durationSeconds));
        return;
    }

    if (line == "state")
    {
        audioGuideService_.printStatus(console_);
        return;
    }

    if (line == "reset")
    {
        audioGuideService_.reset();
        console_.println("Audio guide reset.");
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
