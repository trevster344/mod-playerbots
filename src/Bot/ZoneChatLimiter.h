/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_ZONECHATLIMITER_H
#define PLAYERBOTS_ZONECHATLIMITER_H

#include "Common.h"
#include <mutex>
#include <string>
#include <unordered_map>

class ZoneChatLimiter
{
public:
    static ZoneChatLimiter& instance()
    {
        static ZoneChatLimiter instance;
        return instance;
    }

    bool TryAllow(std::string const& zoneKey, uint32 gapSeconds);

private:
    ZoneChatLimiter() = default;
    ~ZoneChatLimiter() = default;
    ZoneChatLimiter(ZoneChatLimiter const&) = delete;
    ZoneChatLimiter& operator=(ZoneChatLimiter const&) = delete;
    ZoneChatLimiter(ZoneChatLimiter&&) = delete;
    ZoneChatLimiter& operator=(ZoneChatLimiter&&) = delete;

    std::unordered_map<std::string, time_t> lastLine;
    std::mutex mutex;
};

#endif
