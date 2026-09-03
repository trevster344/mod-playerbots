/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "ChatConversationCoordinator.h"
#include <cctype>
#include <sstream>

std::string ChatConversationCoordinator::MakeKey(uint32 senderGuid, uint32 type, std::string const& message)
{
    std::ostringstream key;
    key << senderGuid << ':' << type << ':';

    bool pendingSpace = false;
    for (char raw : message)
    {
        unsigned char ch = static_cast<unsigned char>(raw);
        if (std::isspace(ch))
        {
            pendingSpace = true;
            continue;
        }

        if (pendingSpace && key.tellp() > 0)
            key << ' ';

        pendingSpace = false;
        key << static_cast<char>(std::tolower(ch));
    }

    return key.str();
}

void ChatConversationCoordinator::PurgeExpired()
{
    time_t now = time(nullptr);
    for (auto it = records.begin(); it != records.end();)
    {
        if (now - it->second.claimedAt > RECORD_TTL_SECONDS)
            it = records.erase(it);
        else
            ++it;
    }
}

bool ChatConversationCoordinator::TryClaim(std::string const& key, uint32 botGuid)
{
    std::lock_guard<std::mutex> guard(mutex);
    PurgeExpired();

    if (records.find(key) != records.end())
        return false;

    records[key] = {botGuid, time(nullptr)};
    return true;
}

bool ChatConversationCoordinator::IsSpeaker(std::string const& key, uint32 botGuid)
{
    std::lock_guard<std::mutex> guard(mutex);
    PurgeExpired();

    auto it = records.find(key);
    return it != records.end() && it->second.speakerGuid == botGuid;
}
