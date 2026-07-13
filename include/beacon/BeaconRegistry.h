#pragma once

#include <Arduino.h>

namespace BeaconRegistry
{
  struct BeaconTrack
  {
    uint16_t major;
    uint16_t minor;
    int trackNumber;
    const char *label;
  };

  const BeaconTrack *findTrack(uint16_t major, uint16_t minor);
  size_t trackCount();
  const BeaconTrack *trackAt(size_t index);
}
