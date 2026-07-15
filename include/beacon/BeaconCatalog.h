#pragma once

#include <Arduino.h>

#include "beacon/BeaconTypes.h"

struct BeaconDefinition
{
    BeaconIdentity identity;
    int trackNumber;
    const char *label;
};

class BeaconCatalog
{
public:
    size_t count() const;
    const BeaconDefinition *at(size_t index) const;
    const BeaconDefinition *find(const BeaconIdentity &identity) const;
};
