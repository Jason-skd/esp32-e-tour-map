#include "console/Mp3ConsoleController.h"

Mp3ConsoleController::Mp3ConsoleController(Stream &console, Mp3Player &mp3Player)
    : console_(console), mp3Player_(mp3Player)
{
}

void Mp3ConsoleController::selectTfCard()
{
    console_.println("Selecting TF card...");
    mp3Player_.selectTfCard();
}

void Mp3ConsoleController::setVolume(int volume)
{
    console_.print("Setting volume to ");
    console_.print(volume);
    console_.println("...");

    Mp3Error error = mp3Player_.setVolume(volume);
    if (error != Mp3Error::Ok)
    {
        printError(error);
    }
}

void Mp3ConsoleController::playTrack(int trackNumber)
{
    TrackNameResult trackName = mp3Player_.playTrack(trackNumber);
    if (trackName.error != Mp3Error::Ok)
    {
        printError(trackName.error);
        return;
    }

    console_.print("Play command sent: ");
    console_.println(trackName.value);
}

void Mp3ConsoleController::playStartupTrack(int trackNumber)
{
    TrackNameResult trackName = mp3Player_.playTrack(trackNumber);
    if (trackName.error != Mp3Error::Ok)
    {
        printError(trackName.error);
        return;
    }

    console_.print("Playing initial file: ");
    console_.println(trackName.value);
}

void Mp3ConsoleController::stop()
{
    mp3Player_.stop();
    console_.println("Stop command sent.");
}

void Mp3ConsoleController::printBusyState()
{
    console_.print("BUSY pin state: ");
    if (mp3Player_.isPlaying())
    {
        console_.println("LOW, playing");
    }
    else
    {
        console_.println("HIGH, idle");
    }
}

void Mp3ConsoleController::printResponses()
{
    mp3Player_.printResponses(console_);
}

void Mp3ConsoleController::printError(Mp3Error error)
{
    console_.print("Error: ");
    console_.println(mp3ErrorToString(error));
}
