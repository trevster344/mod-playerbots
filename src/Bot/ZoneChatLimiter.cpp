/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "ZoneChatLimiter.h"

bool ZoneChatLimiter::TryAllow(std::string const& zoneKey, uint32 gapSeconds)
{
    std::lock_guard<std::mutex> guard(mutex);

    time_t now = time(nullptr);
    auto it = lastLine.find(zoneKey);
    if (it != lastLine.end() && (now - it->second) < static_cast<time_t>(gapSeconds))
        return false;

    lastLine[zoneKey] = now;
    return true;
}
