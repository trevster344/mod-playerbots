/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_PORTALVALUE_H
#define PLAYERBOTS_PORTALVALUE_H

#include "ObjectGuid.h"
#include "Value.h"
#include <string>

class PlayerbotAI;

class PortalData
{
public:
    PortalData() : enabled(false), cost(0), spellId(0) {}

    bool enabled;
    ObjectGuid payerGuid;
    uint32 cost;
    uint32 spellId;
    std::string city;

    void Reset()
    {
        enabled = false;
        payerGuid.Clear();
        cost = 0;
        spellId = 0;
        city.clear();
    }
};

class PortalValue : public ManualSetValue<PortalData&>
{
public:
    PortalValue(PlayerbotAI* botAI) : ManualSetValue<PortalData&>(botAI, data, "portal") {}

    PortalData data;
};

#endif
