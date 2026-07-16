#include "beacon/BeaconCatalog.h"

namespace
{
    const BeaconDefinition KnownBeacons[] = {
        {{1, 1}, 1, "point 1"},
        {{1, 2}, 2, "point 2"},
        {{1, 3}, 3, "point 3"},
        {{1, 4}, 4, "point 4"},
        {{1, 5}, 5, "point 5"},
        {{1, 6}, 6, "point 6"},
        {{1, 7}, 7, "point 7"},
    };

    constexpr size_t KnownBeaconCount = sizeof(KnownBeacons) / sizeof(KnownBeacons[0]);
}

size_t BeaconCatalog::count() const
{
    return KnownBeaconCount;
}

const BeaconDefinition *BeaconCatalog::at(size_t index) const
{
    if (index >= KnownBeaconCount)
    {
        return nullptr;
    }

    return &KnownBeacons[index];
}

const BeaconDefinition *BeaconCatalog::find(const BeaconIdentity &identity) const
{
    for (size_t i = 0; i < KnownBeaconCount; ++i)
    {
        if (sameBeaconIdentity(KnownBeacons[i].identity, identity))
        {
            return &KnownBeacons[i];
        }
    }

    return nullptr;
}
