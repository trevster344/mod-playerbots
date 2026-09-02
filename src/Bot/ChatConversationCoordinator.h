/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_CHATCONVERSATIONCOORDINATOR_H
#define PLAYERBOTS_CHATCONVERSATIONCOORDINATOR_H

#include "Common.h"
#include <mutex>
#include <string>
#include <unordered_map>

struct ChatConversationRecord
{
    uint32 speakerGuid;
    time_t claimedAt;
};

class ChatConversationCoordinator
{
public:
    static ChatConversationCoordinator& instance()
    {
        static ChatConversationCoordinator instance;
        return instance;
    }

    static std::string MakeKey(uint32 senderGuid, uint32 type, std::string const& message);

    bool TryClaim(std::string const& key, uint32 botGuid);
    bool IsSpeaker(std::string const& key, uint32 botGuid);

private:
    ChatConversationCoordinator() = default;
    ~ChatConversationCoordinator() = default;
    ChatConversationCoordinator(ChatConversationCoordinator const&) = delete;
    ChatConversationCoordinator& operator=(ChatConversationCoordinator const&) = delete;
    ChatConversationCoordinator(ChatConversationCoordinator&&) = delete;
    ChatConversationCoordinator& operator=(ChatConversationCoordinator&&) = delete;

    void PurgeExpired();

    enum
    {
        RECORD_TTL_SECONDS = 30
    };

    std::unordered_map<std::string, ChatConversationRecord> records;
    std::mutex mutex;
};

#endif
