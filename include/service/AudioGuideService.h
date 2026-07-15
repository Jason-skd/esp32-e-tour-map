#pragma once

#include <Arduino.h>

#include "beacon/BeaconCatalog.h"
#include "beacon/BeaconResolver.h"
#include "beacon/BeaconScanner.h"
#include "beacon/BeaconTypes.h"
#include "mp3/Mp3Player.h"

struct AudioGuideServiceConfig
{
    uint32_t scanDurationSeconds;
    uint32_t scanIntervalMs;
    uint32_t replayCooldownMs;
    uint32_t beaconPresenceTimeoutMs;
    uint32_t switchConfirmMs;
    int switchRssiMargin;
};

enum class AudioGuideState
{
    NoQualifiedBeacon,
    BestLatched,
    BestInSwitching,
    BestInCooldown,
    Mp3NotReady,
    ScannerNotReady,
    TrackNameError,
};

const char *audioGuideStateToString(AudioGuideState state);

struct AudioGuideUpdate
{
    AudioGuideState state;
    bool stateChanged;
    bool hasBeacon;
    BeaconIdentity beacon;
    int trackNumber;
    int rssi;
    Mp3Error mp3Error;
};

class AudioGuideService
{
public:
    AudioGuideService(
        BeaconScanner &scanner,
        BeaconResolver &resolver,
        const BeaconCatalog &catalog,
        Mp3Player &mp3Player,
        const AudioGuideServiceConfig &config);

    AudioGuideUpdate poll();
    void reset();
    AudioGuideState state() const;
    void printStatus(Stream &out) const;

private:
    struct LastSeenEntry
    {
        BeaconIdentity identity;
        int trackNumber;
        const char *label;
        bool hasSeen;
        int rssi;
        int averageRssi;
        int maxRssi;
        int txPower;
        size_t sampleCount;
        uint32_t lastSeenAtMs;
        bool hasCooldown;
        uint32_t cooldownStartedAtMs;
    };

    void loadKnownBeacons();
    void resetRuntime();
    AudioGuideUpdate scanAndHandle();
    void updatePlaybackCompletion(uint32_t now);
    void updateLastSeen(const BeaconFrame &frame);
    void expireCooldowns(uint32_t now);
    void markTrackCooldown(int trackNumber, uint32_t now);
    void clearCandidate();
    void updateCandidate(const LastSeenEntry &candidate, uint32_t now);

    AudioGuideUpdate playBest(LastSeenEntry &entry);
    AudioGuideUpdate handleCandidate(LastSeenEntry &candidate, uint32_t now);
    AudioGuideUpdate makeUpdate(AudioGuideState state, const LastSeenEntry *entry, Mp3Error error, bool stateChanged) const;
    bool noteState(AudioGuideState state, const LastSeenEntry *entry, const char *reason);

    LastSeenEntry *findLastSeen(const BeaconIdentity &identity);
    const LastSeenEntry *findLastSeen(const BeaconIdentity &identity) const;
    LastSeenEntry *bestPresent(uint32_t now);
    const LastSeenEntry *currentBestEntry(uint32_t now) const;
    bool isPresent(const LastSeenEntry &entry, uint32_t now) const;
    bool isTrackInCooldown(int trackNumber, uint32_t now) const;
    bool outranksAnchor(const LastSeenEntry &candidate, const LastSeenEntry *anchor) const;
    uint32_t cooldownRemainingMs(const LastSeenEntry &entry, uint32_t now) const;

    BeaconScanner &scanner_;
    BeaconResolver &resolver_;
    const BeaconCatalog &catalog_;
    Mp3Player &mp3Player_;
    AudioGuideServiceConfig config_;

    LastSeenEntry lastSeen_[MaxKnownBeacons];
    size_t lastSeenCount_;

    AudioGuideState state_;
    bool hasCurrentBest_;
    BeaconIdentity currentBest_;
    int currentTrackNumber_;

    bool hasCandidate_;
    BeaconIdentity candidate_;
    int candidateTrackNumber_;
    uint32_t candidateSinceMs_;

    bool hasPlaybackContext_;
    BeaconIdentity playbackBeacon_;
    int playbackTrackNumber_;
    bool wasMp3Playing_;
    uint32_t nextScanAtMs_;

    bool hasReportedBeacon_;
    BeaconIdentity reportedBeacon_;
};
