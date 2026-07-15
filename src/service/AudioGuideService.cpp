#include "service/AudioGuideService.h"

namespace
{
    constexpr BeaconIdentity EmptyBeacon = {0, 0};
}

const char *audioGuideStateToString(AudioGuideState state)
{
    switch (state)
    {
    case AudioGuideState::NoQualifiedBeacon:
        return "NoQualifiedBeacon";
    case AudioGuideState::BestLatched:
        return "BestLatched";
    case AudioGuideState::BestInSwitching:
        return "BestInSwitching";
    case AudioGuideState::BestInCooldown:
        return "BestInCooldown";
    case AudioGuideState::Mp3NotReady:
        return "Mp3NotReady";
    case AudioGuideState::ScannerNotReady:
        return "ScannerNotReady";
    case AudioGuideState::TrackNameError:
        return "TrackNameError";
    default:
        return "Unknown";
    }
}

AudioGuideService::AudioGuideService(
    BeaconScanner &scanner,
    BeaconResolver &resolver,
    const BeaconCatalog &catalog,
    Mp3Player &mp3Player,
    const AudioGuideServiceConfig &config)
    : scanner_(scanner),
      resolver_(resolver),
      catalog_(catalog),
      mp3Player_(mp3Player),
      config_(config),
      lastSeenCount_(0),
      state_(AudioGuideState::NoQualifiedBeacon),
      hasCurrentBest_(false),
      currentBest_(EmptyBeacon),
      currentTrackNumber_(0),
      hasCandidate_(false),
      candidate_(EmptyBeacon),
      candidateTrackNumber_(0),
      candidateSinceMs_(0),
      hasPlaybackContext_(false),
      playbackBeacon_(EmptyBeacon),
      playbackTrackNumber_(0),
      wasMp3Playing_(false),
      nextScanAtMs_(0),
      hasReportedBeacon_(false),
      reportedBeacon_(EmptyBeacon)
{
    loadKnownBeacons();
    resetRuntime();
}

AudioGuideUpdate AudioGuideService::poll()
{
    const uint32_t now = millis();
    updatePlaybackCompletion(now);
    expireCooldowns(now);

    if (now < nextScanAtMs_)
    {
        return makeUpdate(state_, currentBestEntry(now), Mp3Error::Ok, false);
    }

    nextScanAtMs_ = now + config_.scanIntervalMs;
    return scanAndHandle();
}

void AudioGuideService::reset()
{
    resetRuntime();
    noteState(AudioGuideState::NoQualifiedBeacon, nullptr, "reset");
}

AudioGuideState AudioGuideService::state() const
{
    return state_;
}

void AudioGuideService::printStatus(Stream &out) const
{
    const uint32_t now = millis();

    out.print("audio_guide_state,state=");
    out.print(audioGuideStateToString(state_));
    out.print(",current=");
    if (hasCurrentBest_)
    {
        out.print(currentBest_.major);
        out.print(":");
        out.print(currentBest_.minor);
        out.print(",track=");
        out.print(currentTrackNumber_);
    }
    else
    {
        out.print("none,track=0");
    }
    out.print(",candidate=");
    if (hasCandidate_)
    {
        out.print(candidate_.major);
        out.print(":");
        out.print(candidate_.minor);
        out.print(",candidate_track=");
        out.println(candidateTrackNumber_);
    }
    else
    {
        out.println("none,candidate_track=0");
    }

    out.println("last_seen_columns,major,minor,track,label,seen,present,rssi,avg_rssi,max_rssi,samples,last_seen_ms,cooldown,cooldown_remaining_ms");
    for (size_t i = 0; i < lastSeenCount_; ++i)
    {
        const LastSeenEntry &entry = lastSeen_[i];
        out.print("last_seen,");
        out.print(entry.identity.major);
        out.print(",");
        out.print(entry.identity.minor);
        out.print(",");
        out.print(entry.trackNumber);
        out.print(",");
        out.print(entry.label);
        out.print(",");
        out.print(entry.hasSeen ? 1 : 0);
        out.print(",");
        out.print(isPresent(entry, now) ? 1 : 0);
        out.print(",");
        out.print(entry.rssi);
        out.print(",");
        out.print(entry.averageRssi);
        out.print(",");
        out.print(entry.maxRssi);
        out.print(",");
        out.print(static_cast<unsigned long>(entry.sampleCount));
        out.print(",");
        out.print(entry.hasSeen ? static_cast<unsigned long>(entry.lastSeenAtMs) : 0);
        out.print(",");
        out.print(isTrackInCooldown(entry.trackNumber, now) ? 1 : 0);
        out.print(",");
        out.println(static_cast<unsigned long>(cooldownRemainingMs(entry, now)));
    }
}

void AudioGuideService::loadKnownBeacons()
{
    lastSeenCount_ = 0;
    for (size_t i = 0; i < catalog_.count() && i < MaxKnownBeacons; ++i)
    {
        const BeaconDefinition *definition = catalog_.at(i);
        if (definition == nullptr)
        {
            continue;
        }

        LastSeenEntry &entry = lastSeen_[lastSeenCount_];
        entry.identity = definition->identity;
        entry.trackNumber = definition->trackNumber;
        entry.label = definition->label;
        ++lastSeenCount_;
    }
}

void AudioGuideService::resetRuntime()
{
    for (size_t i = 0; i < lastSeenCount_; ++i)
    {
        LastSeenEntry &entry = lastSeen_[i];
        entry.hasSeen = false;
        entry.rssi = 0;
        entry.averageRssi = 0;
        entry.maxRssi = 0;
        entry.txPower = 0;
        entry.sampleCount = 0;
        entry.lastSeenAtMs = 0;
        entry.hasCooldown = false;
        entry.cooldownStartedAtMs = 0;
    }

    state_ = AudioGuideState::NoQualifiedBeacon;
    hasCurrentBest_ = false;
    currentBest_ = EmptyBeacon;
    currentTrackNumber_ = 0;
    clearCandidate();
    hasPlaybackContext_ = false;
    playbackBeacon_ = EmptyBeacon;
    playbackTrackNumber_ = 0;
    wasMp3Playing_ = false;
    nextScanAtMs_ = 0;
    hasReportedBeacon_ = false;
    reportedBeacon_ = EmptyBeacon;
}

AudioGuideUpdate AudioGuideService::scanAndHandle()
{
    BeaconScanResult scanResult = scanner_.scanOnce(config_.scanDurationSeconds);
    BeaconFrame frame = resolver_.resolve(scanResult);
    const uint32_t now = millis();

    if (frame.status == BeaconScanStatus::NotStarted)
    {
        clearCandidate();
        const bool changed = noteState(AudioGuideState::ScannerNotReady, nullptr, "scanner not started");
        return makeUpdate(AudioGuideState::ScannerNotReady, nullptr, Mp3Error::Ok, changed);
    }

    if (frame.status != BeaconScanStatus::Ok)
    {
        clearCandidate();
        const bool changed = noteState(AudioGuideState::NoQualifiedBeacon, nullptr, "scan failed");
        return makeUpdate(AudioGuideState::NoQualifiedBeacon, nullptr, Mp3Error::Ok, changed);
    }

    updateLastSeen(frame);
    expireCooldowns(now);

    LastSeenEntry *best = bestPresent(now);
    if (best == nullptr)
    {
        hasCurrentBest_ = false;
        currentBest_ = EmptyBeacon;
        currentTrackNumber_ = 0;
        clearCandidate();
        const bool changed = noteState(AudioGuideState::NoQualifiedBeacon, nullptr, "no qualified beacon");
        return makeUpdate(AudioGuideState::NoQualifiedBeacon, nullptr, Mp3Error::Ok, changed);
    }

    LastSeenEntry *anchor = hasCurrentBest_ ? findLastSeen(currentBest_) : nullptr;
    if (anchor != nullptr && !isPresent(*anchor, now))
    {
        anchor = nullptr;
    }

    if (state_ == AudioGuideState::BestLatched &&
        anchor != nullptr)
    {
        if (sameBeaconIdentity(best->identity, anchor->identity) || !outranksAnchor(*best, anchor))
        {
            clearCandidate();
            const bool changed = noteState(AudioGuideState::BestLatched, anchor, "latched");
            return makeUpdate(AudioGuideState::BestLatched, anchor, Mp3Error::Ok, changed);
        }

        return handleCandidate(*best, now);
    }

    if (state_ == AudioGuideState::BestInCooldown && anchor != nullptr)
    {
        if (isTrackInCooldown(anchor->trackNumber, now))
        {
            if (sameBeaconIdentity(best->identity, anchor->identity) || !outranksAnchor(*best, anchor))
            {
                clearCandidate();
                const bool changed = noteState(AudioGuideState::BestInCooldown, anchor, "cooldown");
                return makeUpdate(AudioGuideState::BestInCooldown, anchor, Mp3Error::Ok, changed);
            }

            return handleCandidate(*best, now);
        }

        if (sameBeaconIdentity(best->identity, anchor->identity) || !outranksAnchor(*best, anchor))
        {
            clearCandidate();
            return playBest(*anchor);
        }

        return handleCandidate(*best, now);
    }

    if (anchor != nullptr &&
        !sameBeaconIdentity(best->identity, anchor->identity) &&
        outranksAnchor(*best, anchor))
    {
        return handleCandidate(*best, now);
    }

    if (isTrackInCooldown(best->trackNumber, now))
    {
        hasCurrentBest_ = true;
        currentBest_ = best->identity;
        currentTrackNumber_ = best->trackNumber;
        clearCandidate();
        const bool changed = noteState(AudioGuideState::BestInCooldown, best, "cooldown");
        return makeUpdate(AudioGuideState::BestInCooldown, best, Mp3Error::Ok, changed);
    }

    clearCandidate();
    return playBest(*best);
}

void AudioGuideService::updatePlaybackCompletion(uint32_t now)
{
    const bool isPlaying = mp3Player_.isPlaying();

    if (wasMp3Playing_ && !isPlaying && hasPlaybackContext_)
    {
        markTrackCooldown(playbackTrackNumber_, now);
        LastSeenEntry *entry = findLastSeen(playbackBeacon_);

        Serial.print("[AudioGuide] playback completed, cooldown track=");
        Serial.println(playbackTrackNumber_);

        hasPlaybackContext_ = false;
        playbackBeacon_ = EmptyBeacon;
        playbackTrackNumber_ = 0;

        if (entry != nullptr && isPresent(*entry, now))
        {
            hasCurrentBest_ = true;
            currentBest_ = entry->identity;
            currentTrackNumber_ = entry->trackNumber;
            noteState(AudioGuideState::BestInCooldown, entry, "playback completed");
        }
    }

    wasMp3Playing_ = isPlaying;
}

void AudioGuideService::updateLastSeen(const BeaconFrame &frame)
{
    for (size_t i = 0; i < frame.count; ++i)
    {
        const ResolvedBeacon &beacon = frame.beacons[i];
        LastSeenEntry *entry = findLastSeen(beacon.identity);
        if (entry == nullptr)
        {
            continue;
        }

        entry->hasSeen = true;
        entry->rssi = beacon.rssi;
        entry->averageRssi = beacon.averageRssi;
        entry->maxRssi = beacon.maxRssi;
        entry->txPower = beacon.txPower;
        entry->sampleCount = beacon.sampleCount;
        entry->lastSeenAtMs = beacon.seenAtMs;
    }
}

void AudioGuideService::expireCooldowns(uint32_t now)
{
    for (size_t i = 0; i < lastSeenCount_; ++i)
    {
        LastSeenEntry &entry = lastSeen_[i];
        if (!entry.hasCooldown)
        {
            continue;
        }

        if (static_cast<uint32_t>(now - entry.cooldownStartedAtMs) >= config_.replayCooldownMs)
        {
            entry.hasCooldown = false;
            entry.cooldownStartedAtMs = 0;
        }
    }
}

void AudioGuideService::markTrackCooldown(int trackNumber, uint32_t now)
{
    for (size_t i = 0; i < lastSeenCount_; ++i)
    {
        LastSeenEntry &entry = lastSeen_[i];
        if (entry.trackNumber != trackNumber)
        {
            continue;
        }

        entry.hasCooldown = true;
        entry.cooldownStartedAtMs = now;
    }
}

void AudioGuideService::clearCandidate()
{
    hasCandidate_ = false;
    candidate_ = EmptyBeacon;
    candidateTrackNumber_ = 0;
    candidateSinceMs_ = 0;
}

void AudioGuideService::updateCandidate(const LastSeenEntry &candidate, uint32_t now)
{
    hasCandidate_ = true;
    candidate_ = candidate.identity;
    candidateTrackNumber_ = candidate.trackNumber;
    candidateSinceMs_ = now;
}

AudioGuideUpdate AudioGuideService::playBest(LastSeenEntry &entry)
{
    TrackNameResult trackName = mp3Player_.playTrack(entry.trackNumber);
    if (trackName.error != Mp3Error::Ok)
    {
        const bool changed = noteState(AudioGuideState::TrackNameError, &entry, "track name error");
        return makeUpdate(AudioGuideState::TrackNameError, &entry, trackName.error, changed);
    }

    hasCurrentBest_ = true;
    currentBest_ = entry.identity;
    currentTrackNumber_ = entry.trackNumber;
    hasPlaybackContext_ = true;
    playbackBeacon_ = entry.identity;
    playbackTrackNumber_ = entry.trackNumber;
    wasMp3Playing_ = mp3Player_.isPlaying();
    clearCandidate();

    const bool changed = noteState(AudioGuideState::BestLatched, &entry, "play");
    return makeUpdate(AudioGuideState::BestLatched, &entry, Mp3Error::Ok, changed);
}

AudioGuideUpdate AudioGuideService::handleCandidate(LastSeenEntry &candidate, uint32_t now)
{
    if (isTrackInCooldown(candidate.trackNumber, now))
    {
        clearCandidate();
        const bool changed = noteState(AudioGuideState::BestInCooldown, &candidate, "candidate cooldown");
        return makeUpdate(AudioGuideState::BestInCooldown, &candidate, Mp3Error::Ok, changed);
    }

    if (!hasCandidate_ || !sameBeaconIdentity(candidate_, candidate.identity))
    {
        updateCandidate(candidate, now);
    }

    if (static_cast<uint32_t>(now - candidateSinceMs_) < config_.switchConfirmMs)
    {
        const bool changed = noteState(AudioGuideState::BestInSwitching, &candidate, "switching");
        return makeUpdate(AudioGuideState::BestInSwitching, &candidate, Mp3Error::Ok, changed);
    }

    clearCandidate();
    return playBest(candidate);
}

AudioGuideUpdate AudioGuideService::makeUpdate(
    AudioGuideState state,
    const LastSeenEntry *entry,
    Mp3Error error,
    bool stateChanged) const
{
    AudioGuideUpdate update;
    update.state = state;
    update.stateChanged = stateChanged;
    update.hasBeacon = entry != nullptr;
    update.beacon = entry == nullptr ? EmptyBeacon : entry->identity;
    update.trackNumber = entry == nullptr ? 0 : entry->trackNumber;
    update.rssi = entry == nullptr ? 0 : entry->rssi;
    update.mp3Error = error;
    return update;
}

bool AudioGuideService::noteState(AudioGuideState state, const LastSeenEntry *entry, const char *reason)
{
    const bool hasEntry = entry != nullptr;
    const bool sameReportedBeacon =
        (!hasEntry && !hasReportedBeacon_) ||
        (hasEntry && hasReportedBeacon_ && sameBeaconIdentity(entry->identity, reportedBeacon_));
    const bool changed = state_ != state || !sameReportedBeacon;

    state_ = state;
    hasReportedBeacon_ = hasEntry;
    reportedBeacon_ = hasEntry ? entry->identity : EmptyBeacon;

    if (!changed)
    {
        return false;
    }

    Serial.print("[AudioGuide] state=");
    Serial.print(audioGuideStateToString(state));
    Serial.print(" reason=");
    Serial.print(reason);
    Serial.print(" beacon=");
    if (hasEntry)
    {
        Serial.print(entry->identity.major);
        Serial.print(":");
        Serial.print(entry->identity.minor);
        Serial.print(" track=");
        Serial.print(entry->trackNumber);
        Serial.print(" rssi=");
        Serial.println(entry->rssi);
    }
    else
    {
        Serial.println("none");
    }

    return true;
}

AudioGuideService::LastSeenEntry *AudioGuideService::findLastSeen(const BeaconIdentity &identity)
{
    for (size_t i = 0; i < lastSeenCount_; ++i)
    {
        if (sameBeaconIdentity(lastSeen_[i].identity, identity))
        {
            return &lastSeen_[i];
        }
    }

    return nullptr;
}

const AudioGuideService::LastSeenEntry *AudioGuideService::findLastSeen(const BeaconIdentity &identity) const
{
    for (size_t i = 0; i < lastSeenCount_; ++i)
    {
        if (sameBeaconIdentity(lastSeen_[i].identity, identity))
        {
            return &lastSeen_[i];
        }
    }

    return nullptr;
}

AudioGuideService::LastSeenEntry *AudioGuideService::bestPresent(uint32_t now)
{
    LastSeenEntry *best = nullptr;

    for (size_t i = 0; i < lastSeenCount_; ++i)
    {
        LastSeenEntry &entry = lastSeen_[i];
        if (!isPresent(entry, now))
        {
            continue;
        }

        if (best == nullptr || entry.rssi > best->rssi)
        {
            best = &entry;
        }
    }

    return best;
}

const AudioGuideService::LastSeenEntry *AudioGuideService::currentBestEntry(uint32_t now) const
{
    if (!hasCurrentBest_)
    {
        return nullptr;
    }

    const LastSeenEntry *entry = findLastSeen(currentBest_);
    if (entry == nullptr || !isPresent(*entry, now))
    {
        return nullptr;
    }

    return entry;
}

bool AudioGuideService::isPresent(const LastSeenEntry &entry, uint32_t now) const
{
    return entry.hasSeen &&
           static_cast<uint32_t>(now - entry.lastSeenAtMs) <= config_.beaconPresenceTimeoutMs;
}

bool AudioGuideService::isTrackInCooldown(int trackNumber, uint32_t now) const
{
    for (size_t i = 0; i < lastSeenCount_; ++i)
    {
        const LastSeenEntry &entry = lastSeen_[i];
        if (entry.trackNumber != trackNumber || !entry.hasCooldown)
        {
            continue;
        }

        if (static_cast<uint32_t>(now - entry.cooldownStartedAtMs) < config_.replayCooldownMs)
        {
            return true;
        }
    }

    return false;
}

bool AudioGuideService::outranksAnchor(const LastSeenEntry &candidate, const LastSeenEntry *anchor) const
{
    if (anchor == nullptr)
    {
        return true;
    }

    if (sameBeaconIdentity(candidate.identity, anchor->identity))
    {
        return false;
    }

    return candidate.rssi > anchor->rssi + config_.switchRssiMargin;
}

uint32_t AudioGuideService::cooldownRemainingMs(const LastSeenEntry &entry, uint32_t now) const
{
    if (!entry.hasCooldown)
    {
        return 0;
    }

    const uint32_t elapsed = now - entry.cooldownStartedAtMs;
    if (elapsed >= config_.replayCooldownMs)
    {
        return 0;
    }

    return config_.replayCooldownMs - elapsed;
}
