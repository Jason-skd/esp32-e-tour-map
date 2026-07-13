#include "beacon/BeaconRegistry.h"

namespace BeaconRegistry
{
  namespace
  {
    const BeaconTrack Tracks[] = {
        {5, 6, 1, "default beacon"},
        {1, 1, 1, "point 1"},
        {1, 2, 2, "point 2"},
        {1, 3, 3, "point 3"},
    };

    constexpr size_t TrackCount = sizeof(Tracks) / sizeof(Tracks[0]);
  }

  const BeaconTrack *findTrack(uint16_t major, uint16_t minor)
  {
    for (size_t i = 0; i < TrackCount; ++i)
    {
      if (Tracks[i].major == major && Tracks[i].minor == minor)
      {
        return &Tracks[i];
      }
    }

    return nullptr;
  }

  size_t trackCount()
  {
    return TrackCount;
  }

  const BeaconTrack *trackAt(size_t index)
  {
    if (index >= TrackCount)
    {
      return nullptr;
    }

    return &Tracks[index];
  }
}
