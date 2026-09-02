/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "SayAction.h"
#include "AiFactory.h"
#include "CastCustomSpellAction.h"
#include "ChatConversationCoordinator.h"
#include "Event.h"
#include "InviteToGroupAction.h"
#include "PlayerbotTextMgr.h"
#include "Playerbots.h"
#include "ServerFacade.h"
#include "SharedDefines.h"
#include "TradeAction.h"
#include <cctype>
#include <iterator>
#include <string>
#include <vector>

static const std::unordered_set<std::string> noReplyMsgs = {
    "join",
    "leave",
    "follow",
    "attack",
    "pull",
    "flee",
    "reset",
    "reset ai",
    "all ?",
    "talents",
    "talents list",
    "talents auto",
    "talk",
    "stay",
    "stats",
    "who",
    "items",
    "leave",
    "join",
    "repair",
    "summon",
    "nc ?",
    "co ?",
    "de ?",
    "dead ?",
    "follow",
    "los",
    "guard",
    "do accept invitation",
    "stats",
    "react ?",
    "reset strats",
    "home",
};
static const std::unordered_set<std::string> noReplyMsgParts = {
    "+", "-", "@", "follow target", "focus heal", "cast ", "accept [", "e [", "destroy [", "go zone"};
static const std::unordered_set<std::string> noReplyMsgStarts = {"e ", "accept ", "cast ", "destroy "};

SayAction::SayAction(PlayerbotAI* botAI) : Action(botAI, "say"), Qualified() {}

bool SayAction::Execute(Event /*event*/)
{
    std::string text = "";
    std::map<std::string, std::string> placeholders;
    Unit* target = AI_VALUE(Unit*, "tank target");
    if (!target)
        target = AI_VALUE(Unit*, "current target");

    // set replace strings
    if (target)
        placeholders["<target>"] = target->GetName();
    placeholders["<randomfaction>"] = IsAlliance(bot->getRace()) ? "Alliance" : "Horde";
    if (qualifier == "low ammo" || qualifier == "no ammo")
    {
        if (Item* const pItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_RANGED))
        {
            switch (pItem->GetTemplate()->SubClass)
            {
                case ITEM_SUBCLASS_WEAPON_GUN:
                    placeholders["<ammo>"] = "bullets";
                    break;
                case ITEM_SUBCLASS_WEAPON_BOW:
                case ITEM_SUBCLASS_WEAPON_CROSSBOW:
                    placeholders["<ammo>"] = "arrows";
                    break;
            }
        }
    }

    if (bot->GetMap())
    {
        if (AreaTableEntry const* zone = sAreaTableStore.LookupEntry(bot->GetMap()->GetZoneId(bot->GetPhaseMask(), bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ())))
            placeholders["<subzone>"] = zone->area_name[sWorld->GetDefaultDbcLocale()];
    }

    // set delay before next say
    uint32 nextTime = time(nullptr) + urand(1, 30);
    botAI->GetAiObjectContext()->GetValue<time_t>("last said", qualifier)->Set(nextTime);

    Group* group = bot->GetGroup();
    if (group)
    {
        std::vector<Player*> members;
        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
        {
            Player* member = ref->GetSource();
            PlayerbotAI* memberAi = GET_PLAYERBOT_AI(member);
            if (memberAi)
                members.push_back(member);
        }

        uint32 count = members.size();
        if (count > 1)
        {
            for (uint32 i = 0; i < count * 5; i++)
            {
                int i1 = urand(0, count - 1);
                int i2 = urand(0, count - 1);

                Player* item = members[i1];
                members[i1] = members[i2];
                members[i2] = item;
            }
        }

        int index = 0;
        for (auto& member : members)
        {
            PlayerbotAI* memberAi = GET_PLAYERBOT_AI(member);
            if (memberAi)
                memberAi->GetAiObjectContext()
                    ->GetValue<time_t>("last said", qualifier)
                    ->Set(nextTime + (20 * ++index) + urand(1, 15));
        }
    }

    // load text based on chance
    if (!PlayerbotTextMgr::instance().GetBotText(qualifier, text, placeholders))
        return false;

    if (text.find("/y ") == 0)
        bot->Yell(text.substr(3), (bot->GetTeamId() == TEAM_ALLIANCE ? LANG_COMMON : LANG_ORCISH));
    else
        bot->Say(text, (bot->GetTeamId() == TEAM_ALLIANCE ? LANG_COMMON : LANG_ORCISH));

    return true;
}

bool SayAction::isUseful()
{
    if (!botAI->AllowActivity())
        return false;

    if (botAI->HasStrategy("silent", BotState::BOT_STATE_NON_COMBAT))
        return false;

    time_t lastSaid = AI_VALUE2(time_t, "last said", qualifier);
    return (time(nullptr) - lastSaid) > 30;
}

void ChatReplyAction::ChatReplyDo(Player* bot, uint32& type, uint32& guid1, std::string& msg, std::string& chanName, std::string& name)
{
    std::string respondsText = "";

    // if we're just commanding bots around, don't respond...
    // first one is for exact word matches
    if (noReplyMsgs.find(msg) != noReplyMsgs.end())
    {
        /*std::ostringstream out;
        out << "DEBUG ChatReplyDo decided to ignore exact blocklist match" << msg;
        bot->Say(out.str(), LANG_UNIVERSAL);*/
        return;
    }

    // second one is for partial matches like + or - where we change strats
    if (std::any_of(noReplyMsgParts.begin(), noReplyMsgParts.end(),
                    [&msg](const std::string& part) { return msg.find(part) != std::string::npos; }))
    {
        /*std::ostringstream out;
        out << "DEBUG ChatReplyDo decided to ignore partial blocklist match" << msg;
        bot->Say(out.str(), LANG_UNIVERSAL);*/
        return;
    }

    if (std::any_of(noReplyMsgStarts.begin(), noReplyMsgStarts.end(),
                    [&msg](const std::string& start)
                    {
                        return msg.find(start) == 0;  // Check if the start matches the beginning of msg
                    }))
    {
        /*std::ostringstream out;
        out << "DEBUG ChatReplyDo decided to ignore start blocklist match" << msg;
        bot->Say(out.str(), LANG_UNIVERSAL);*/
        return;
    }

    ChatChannelSource chatChannelSource = GET_PLAYERBOT_AI(bot)->GetChatChannelSource(bot, type, chanName);
    if ((msg.starts_with("LFG") || msg.starts_with("LFM")) && HandleLFGQuestsReply(bot, chatChannelSource, msg, name))
    {
        return;
    }

    if (msg.starts_with("WTB") && HandleWTBItemsReply(bot, chatChannelSource, msg, name))
    {
        return;
    }

    //toxic links
    if (msg.starts_with(sPlayerbotAIConfig.toxicLinksPrefix)
        && (GET_PLAYERBOT_AI(bot)->GetChatHelper()->ExtractAllItemIds(msg).size() > 0 || GET_PLAYERBOT_AI(bot)->GetChatHelper()->ExtractAllQuestIds(msg).size() > 0))
    {
        HandleToxicLinksReply(bot, chatChannelSource);
        return;
    }

    //thunderfury
    if (GET_PLAYERBOT_AI(bot)->GetChatHelper()->ExtractAllItemIds(msg).count(19019))
    {
        HandleThunderfuryReply(bot, chatChannelSource);
        return;
    }

    // Functional requests: invite, buff, conjured food/water for the real player who asked.
    // Restricted to say/yell so we don't double-act on whisper/party messages, which are already
    // routed through the bot's chat-command pipeline (invite, buff list, trade, ...).
    if (sPlayerbotAIConfig.conversationActions && (type == CHAT_MSG_SAY || type == CHAT_MSG_YELL))
    {
        uint32 const requestFlags = ChatReplyAction::ClassifyConversationRequest(msg);
        if (requestFlags && ChatReplyAction::HandleConversationAction(bot, requestFlags, msg, type, guid1, name,
                                                                      chatChannelSource))
            return;
    }

    // Only the claimed speaker answers an unaddressed conversation (say/yell/party/raid...).
    // Whisper and whole-name mentions are exempt: they target one bot directly.
    if (sPlayerbotAIConfig.chatReplySingleSpeaker && type != CHAT_MSG_WHISPER &&
        !PlayerbotAI::IsBotMentioned(bot, msg) &&
        !ChatConversationCoordinator::instance().IsSpeaker(
            ChatConversationCoordinator::MakeKey(guid1, type, msg), bot->GetGUID().GetCounter()))
    {
        return;
    }

    auto messageRepy = GenerateReplyMessage(bot, msg, guid1, name);
    SendGeneralResponse(bot, chatChannelSource, messageRepy, name);
}

bool ChatReplyAction::HandleThunderfuryReply(Player* bot, ChatChannelSource chatChannelSource)
{
    std::map<std::string, std::string> placeholders;
    const auto thunderfury = sObjectMgr->GetItemTemplate(19019);
    placeholders["%thunderfury_link"] = GET_PLAYERBOT_AI(bot)->GetChatHelper()->FormatItem(thunderfury);

    std::string responseMessage = PlayerbotTextMgr::instance().GetBotText("thunderfury_spam", placeholders);

    switch (chatChannelSource)
    {
        case ChatChannelSource::SRC_WORLD:
        {
            GET_PLAYERBOT_AI(bot)->SayToWorld(responseMessage);
            break;
        }
        case ChatChannelSource::SRC_GENERAL:
        {
            GET_PLAYERBOT_AI(bot)->SayToChannel(responseMessage, ChatChannelId::GENERAL);
            break;
        }
        default:
            break;
    }

    GET_PLAYERBOT_AI(bot)->GetAiObjectContext()->GetValue<time_t>("last said", "chat")->Set(time(0) + urand(5, 25));
    return true;
}

bool ChatReplyAction::HandleToxicLinksReply(Player* bot, ChatChannelSource chatChannelSource)
{
    //quests
    std::vector<uint32> incompleteQuests;
    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = bot->GetQuestSlotQuestId(slot);
        if (!questId)
            continue;

        QuestStatus status = bot->GetQuestStatus(questId);
        if (status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_NONE)
            incompleteQuests.push_back(questId);
    }

    //items
    std::vector<Item*> botItems = GET_PLAYERBOT_AI(bot)->GetInventoryAndEquippedItems();

    std::map<std::string, std::string> placeholders;
    placeholders["%random_inventory_item_link"] = botItems.size() > 0 ? GET_PLAYERBOT_AI(bot)->GetChatHelper()->FormatItem(botItems[rand() % botItems.size()]->GetTemplate()) : PlayerbotTextMgr::instance().GetBotText("string_empty_link");
    placeholders["%prefix"] = sPlayerbotAIConfig.toxicLinksPrefix;

    if (incompleteQuests.size() > 0)
    {
        Quest const* quest = sObjectMgr->GetQuestTemplate(incompleteQuests[rand() % incompleteQuests.size()]);
        placeholders["%random_taken_quest_or_item_link"] = GET_PLAYERBOT_AI(bot)->GetChatHelper()->FormatQuest(quest);
    }
    else
    {
        placeholders["%random_taken_quest_or_item_link"] = placeholders["%random_inventory_item_link"];
    }

    placeholders["%my_role"] = ChatHelper::FormatClass(bot, AiFactory::GetPlayerSpecTab(bot));
    AreaTableEntry const* current_area = GET_PLAYERBOT_AI(bot)->GetCurrentArea();
    AreaTableEntry const* current_zone = GET_PLAYERBOT_AI(bot)->GetCurrentZone();
    placeholders["%area_name"] = current_area ? GET_PLAYERBOT_AI(bot)->GetLocalizedAreaName(current_area) : PlayerbotTextMgr::instance().GetBotText("string_unknown_area");
    placeholders["%zone_name"] = current_zone ? GET_PLAYERBOT_AI(bot)->GetLocalizedAreaName(current_zone) : PlayerbotTextMgr::instance().GetBotText("string_unknown_area");
    placeholders["%my_class"] = GET_PLAYERBOT_AI(bot)->GetChatHelper()->FormatClass(bot->getClass());
    placeholders["%my_race"] = GET_PLAYERBOT_AI(bot)->GetChatHelper()->FormatRace(bot->getRace());
    placeholders["%my_level"] = std::to_string(bot->GetLevel());

    switch (chatChannelSource)
    {
        case ChatChannelSource::SRC_WORLD:
        {
            GET_PLAYERBOT_AI(bot)->SayToWorld(PlayerbotTextMgr::instance().GetBotText("suggest_toxic_links", placeholders));
            break;
        }
        case ChatChannelSource::SRC_GENERAL:
        {
            GET_PLAYERBOT_AI(bot)->SayToChannel(PlayerbotTextMgr::instance().GetBotText("suggest_toxic_links", placeholders), ChatChannelId::GENERAL);
            break;
        }
        case ChatChannelSource::SRC_GUILD:
        {
            GET_PLAYERBOT_AI(bot)->SayToGuild(PlayerbotTextMgr::instance().GetBotText("suggest_toxic_links", placeholders));
            break;
        }
        default:
            break;
    }

    GET_PLAYERBOT_AI(bot)->GetAiObjectContext()->GetValue<time_t>("last said", "chat")->Set(time(0) + urand(5, 60));

    return true;
}
bool ChatReplyAction::HandleWTBItemsReply(Player* bot, ChatChannelSource chatChannelSource, std::string& msg, std::string& name)
{
    auto messageItemIds = GET_PLAYERBOT_AI(bot)->GetChatHelper()->ExtractAllItemIds(msg);

    if (messageItemIds.empty())
    {
        return false;
    }

    std::set<uint32> matchingItemIds;

    for (auto messageItemId : messageItemIds)
    {
        if (GET_PLAYERBOT_AI(bot)->HasItemInInventory(messageItemId))
        {
            matchingItemIds.insert(messageItemId);
        }
    }

    if (!matchingItemIds.empty())
    {
        std::map<std::string, std::string> placeholders;
        placeholders["%other_name"] = name;
        AreaTableEntry const* current_area = GET_PLAYERBOT_AI(bot)->GetCurrentArea();
        AreaTableEntry const* current_zone = GET_PLAYERBOT_AI(bot)->GetCurrentZone();
        placeholders["%area_name"] = current_area ? GET_PLAYERBOT_AI(bot)->GetLocalizedAreaName(current_area) : PlayerbotTextMgr::instance().GetBotText("string_unknown_area");
        placeholders["%zone_name"] = current_zone ? GET_PLAYERBOT_AI(bot)->GetLocalizedAreaName(current_zone) : PlayerbotTextMgr::instance().GetBotText("string_unknown_area");
        placeholders["%my_class"] = GET_PLAYERBOT_AI(bot)->GetChatHelper()->FormatClass(bot->getClass());
        placeholders["%my_race"] = GET_PLAYERBOT_AI(bot)->GetChatHelper()->FormatRace(bot->getRace());
        placeholders["%my_level"] = std::to_string(bot->GetLevel());
        placeholders["%my_role"] = ChatHelper::FormatClass(bot, AiFactory::GetPlayerSpecTab(bot));
        placeholders["%formatted_item_links"] = "";

        for (auto matchingItemId : matchingItemIds)
        {
            ItemTemplate const* proto = sObjectMgr->GetItemTemplate(matchingItemId);
            placeholders["%formatted_item_links"] += GET_PLAYERBOT_AI(bot)->GetChatHelper()->FormatItem(proto, GET_PLAYERBOT_AI(bot)->GetInventoryItemsCountWithId(matchingItemId));
            placeholders["%formatted_item_links"] += " ";
        }

        switch (chatChannelSource)
        {
            case ChatChannelSource::SRC_WORLD:
            {
                //may reply to the same channel or whisper
                if (urand(0, 1))
                {
                    std::string responseMessage = PlayerbotTextMgr::instance().GetBotText("response_wtb_items_channel", placeholders);
                    GET_PLAYERBOT_AI(bot)->SayToWorld(responseMessage);
                }
                else
                {
                    std::string responseMessage = PlayerbotTextMgr::instance().GetBotText("response_wtb_items_whisper", placeholders);
                    GET_PLAYERBOT_AI(bot)->Whisper(responseMessage, name);
                }
                break;
            }
            case ChatChannelSource::SRC_GENERAL:
            {
                //may reply to the same channel or whisper
                if (urand(0, 1))
                {
                    std::string responseMessage = PlayerbotTextMgr::instance().GetBotText("response_wtb_items_channel", placeholders);
                    GET_PLAYERBOT_AI(bot)->SayToChannel(responseMessage, ChatChannelId::GENERAL);
                }
                else
                {
                    std::string responseMessage = PlayerbotTextMgr::instance().GetBotText("response_wtb_items_whisper", placeholders);
                    GET_PLAYERBOT_AI(bot)->Whisper(responseMessage, name);
                }
                break;
            }
            case ChatChannelSource::SRC_TRADE:
            {
                //may reply to the same channel or whisper
                if (urand(0, 1))
                {
                    std::string responseMessage = PlayerbotTextMgr::instance().GetBotText("response_wtb_items_channel", placeholders);
                    GET_PLAYERBOT_AI(bot)->SayToChannel(responseMessage, ChatChannelId::TRADE);
                }
                else
                {
                    std::string responseMessage = PlayerbotTextMgr::instance().GetBotText("response_wtb_items_whisper", placeholders);
                    GET_PLAYERBOT_AI(bot)->Whisper(responseMessage, name);
                }
                break;
            }
            default:
            break;
        }
        GET_PLAYERBOT_AI(bot)->GetAiObjectContext()->GetValue<time_t>("last said", "chat")->Set(time(0) + urand(5, 60));
    }

    return true;
}
bool ChatReplyAction::HandleLFGQuestsReply(Player* bot, ChatChannelSource chatChannelSource, std::string& msg, std::string& name)
{
    auto messageQuestIds = GET_PLAYERBOT_AI(bot)->GetChatHelper()->ExtractAllQuestIds(msg);

    if (messageQuestIds.empty())
    {
        return false;
    }

    auto botQuestIds = GET_PLAYERBOT_AI(bot)->GetAllCurrentQuestIds();
    std::set<uint32> matchingQuestIds;
    for (auto botQuestId : botQuestIds)
    {
        if (messageQuestIds.count(botQuestId) != 0)
        {
            matchingQuestIds.insert(botQuestId);
        }
    }

    if (!matchingQuestIds.empty())
    {
        std::map<std::string, std::string> placeholders;
        placeholders["%other_name"] = name;
        AreaTableEntry const* current_area = GET_PLAYERBOT_AI(bot)->GetCurrentArea();
        AreaTableEntry const* current_zone = GET_PLAYERBOT_AI(bot)->GetCurrentZone();
        placeholders["%area_name"] = current_area ? GET_PLAYERBOT_AI(bot)->GetLocalizedAreaName(current_area) : PlayerbotTextMgr::instance().GetBotText("string_unknown_area");
        placeholders["%zone_name"] = current_zone ? GET_PLAYERBOT_AI(bot)->GetLocalizedAreaName(current_zone) : PlayerbotTextMgr::instance().GetBotText("string_unknown_area");
        placeholders["%my_class"] = GET_PLAYERBOT_AI(bot)->GetChatHelper()->FormatClass(bot->getClass());
        placeholders["%my_race"] = GET_PLAYERBOT_AI(bot)->GetChatHelper()->FormatRace(bot->getRace());
        placeholders["%my_level"] = std::to_string(bot->GetLevel());
        placeholders["%my_role"] = ChatHelper::FormatClass(bot, AiFactory::GetPlayerSpecTab(bot));
        placeholders["%quest_links"] = "";
        for (auto matchingQuestId : matchingQuestIds)
        {
            Quest const* quest = sObjectMgr->GetQuestTemplate(matchingQuestId);
            placeholders["%quest_links"] += GET_PLAYERBOT_AI(bot)->GetChatHelper()->FormatQuest(quest);
        }

        switch (chatChannelSource)
        {
            case ChatChannelSource::SRC_WORLD:
            {
                //may reply to the same channel or whisper
                if (urand(0, 1))
                {
                    std::string responseMessage = PlayerbotTextMgr::instance().GetBotText("response_lfg_quests_channel", placeholders);
                    GET_PLAYERBOT_AI(bot)->SayToWorld(responseMessage);
                }
                else
                {
                    std::string responseMessage = PlayerbotTextMgr::instance().GetBotText("response_lfg_quests_whisper", placeholders);
                    GET_PLAYERBOT_AI(bot)->Whisper(responseMessage, name);
                }
                break;
            }
            case ChatChannelSource::SRC_GENERAL:
            {
                //may reply to the same channel or whisper
                if (urand(0, 1))
                {
                    std::string responseMessage = PlayerbotTextMgr::instance().GetBotText("response_lfg_quests_channel", placeholders);
                    GET_PLAYERBOT_AI(bot)->SayToChannel(responseMessage, ChatChannelId::GENERAL);
                }
                else
                {
                    std::string responseMessage = PlayerbotTextMgr::instance().GetBotText("response_lfg_quests_whisper", placeholders);
                    GET_PLAYERBOT_AI(bot)->Whisper(responseMessage, name);
                }
                break;
            }
            case ChatChannelSource::SRC_LOOKING_FOR_GROUP:
            {
                //do not reply to the chat
                //may whisper
                std::string responseMessage = PlayerbotTextMgr::instance().GetBotText("response_lfg_quests_whisper", placeholders);
                GET_PLAYERBOT_AI(bot)->Whisper(responseMessage, name);
                break;
            }
            default:
            break;
        }
        GET_PLAYERBOT_AI(bot)->GetAiObjectContext()->GetValue<time_t>("last said", "chat")->Set(time(0) + urand(5, 25));
    }

    return true;
}

bool ChatReplyAction::SendGeneralResponse(Player* bot, ChatChannelSource chatChannelSource, std::string& responseMessage, std::string& name)
{
    // send responds
    switch (chatChannelSource)
    {
        case ChatChannelSource::SRC_WORLD:
        {
            //may reply to the same channel or whisper
            GET_PLAYERBOT_AI(bot)->SayToWorld(responseMessage);
            break;
        }
        case ChatChannelSource::SRC_GENERAL:
        {
            //may reply to the same channel 80% or whisper
            if (urand(0, 100) < 80)
                GET_PLAYERBOT_AI(bot)->SayToChannel(responseMessage, ChatChannelId::GENERAL);
            else
                GET_PLAYERBOT_AI(bot)->Whisper(responseMessage, name);
            break;
        }
        case ChatChannelSource::SRC_TRADE:
        {
            //do not reply to the chat
            //may whisper
            break;
        }
        case ChatChannelSource::SRC_LOCAL_DEFENSE:
        {
            //may reply to the same channel or whisper
            GET_PLAYERBOT_AI(bot)->SayToChannel(responseMessage, ChatChannelId::LOCAL_DEFENSE);
            break;
        }
        case ChatChannelSource::SRC_WORLD_DEFENSE:
        {
            //may whisper
            break;
        }
        case ChatChannelSource::SRC_LOOKING_FOR_GROUP:
        {
            //do not reply to the chat
            break;
        }
        case ChatChannelSource::SRC_GUILD_RECRUITMENT:
        {
            //do not reply to the chat
            break;
        }
        case ChatChannelSource::SRC_WHISPER:
        {
            GET_PLAYERBOT_AI(bot)->Whisper(responseMessage, name);
            break;
        }
        case ChatChannelSource::SRC_SAY:
        {
            GET_PLAYERBOT_AI(bot)->Say(responseMessage);
            break;
        }
        case ChatChannelSource::SRC_YELL:
        {
            GET_PLAYERBOT_AI(bot)->Yell(responseMessage);
            break;
        }
        case ChatChannelSource::SRC_GUILD:
        {
            GET_PLAYERBOT_AI(bot)->SayToGuild(responseMessage);
            break;
        }
        case ChatChannelSource::SRC_PARTY:
        {
            GET_PLAYERBOT_AI(bot)->SayToParty(responseMessage);
            break;
        }
        case ChatChannelSource::SRC_RAID:
        {
            GET_PLAYERBOT_AI(bot)->SayToRaid(responseMessage);
            break;
        }
        default:
            break;
    }
    GET_PLAYERBOT_AI(bot)->GetAiObjectContext()->GetValue<time_t>("last said", "chat")->Set(time(0) + urand(5, 25));

    return true;
}

namespace
{
    std::string DialogToLower(std::string text)
    {
        for (char& ch : text)
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        return text;
    }

    std::vector<std::string> DialogWords(std::string const& text)
    {
        std::vector<std::string> words;
        std::string word;
        for (char raw : text)
        {
            unsigned char ch = static_cast<unsigned char>(raw);
            if (std::isalnum(ch))
                word += static_cast<char>(std::tolower(ch));
            else if (!word.empty())
            {
                words.push_back(word);
                word.clear();
            }
        }

        if (!word.empty())
            words.push_back(word);

        return words;
    }

    bool HasAnyWord(std::string const& text, std::initializer_list<std::string> const& wanted)
    {
        std::vector<std::string> const words = DialogWords(text);
        for (std::string const& word : words)
        {
            for (std::string const& want : wanted)
            {
                if (word == want)
                    return true;
            }
        }

        return false;
    }

    bool HasPhrase(std::string const& text, std::initializer_list<std::string> const& phrases)
    {
        for (std::string const& phrase : phrases)
        {
            if (text.find(phrase) != std::string::npos)
                return true;
        }

        return false;
    }

    std::string RandomLine(std::initializer_list<std::string> const& lines)
    {
        auto it = lines.begin();
        std::advance(it, urand(0, lines.size() - 1));
        return *it;
    }

    std::string GetDialogText(std::string const& textName,
                              std::map<std::string, std::string> const& placeholders,
                              std::initializer_list<std::string> const& fallbacks)
    {
        std::string text;
        if (PlayerbotTextMgr::instance().HasText(textName))
            text = PlayerbotTextMgr::instance().GetBotText(textName, placeholders);

        if (text.empty())
        {
            text = RandomLine(fallbacks);
            for (auto const& placeholder : placeholders)
                PlayerbotTextMgr::replaceAll(text, placeholder.first, placeholder.second);
        }

        return text;
    }

    std::map<std::string, std::string> BuildDialogPlaceholders(Player* bot, std::string const& speakerName)
    {
        std::map<std::string, std::string> placeholders;
        PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
        placeholders["%s"] = speakerName;
        placeholders["%other"] = speakerName;
        placeholders["%name"] = bot->GetName();
        placeholders["%class"] = botAI->GetChatHelper()->FormatClass(bot->getClass());
        placeholders["%race"] = botAI->GetChatHelper()->FormatRace(bot->getRace());
        placeholders["%level"] = std::to_string(bot->GetLevel());
        placeholders["%role"] = ChatHelper::FormatClass(bot, AiFactory::GetPlayerSpecTab(bot));
        AreaTableEntry const* zone = botAI->GetCurrentZone();
        placeholders["%zone"] = zone ? PlayerbotAI::GetLocalizedAreaName(zone) : "this zone";
        return placeholders;
    }
}

std::string ChatReplyAction::GenerateReplyMessage(Player* bot, std::string& incomingMessage, uint32& guid1, std::string& name)
{
    std::map<std::string, std::string> const placeholders = BuildDialogPlaceholders(bot, name);
    std::string const lower = DialogToLower(incomingMessage);
    bool const isQuestion = lower.find('?') != std::string::npos;

    if (Player* plr = ObjectAccessor::FindPlayer(ObjectGuid(HighGuid::Player, guid1)))
    {
        if (plr->isGMChat())
        {
            return GetDialogText("dialog_admin", placeholders,
                                 {"yes, %s", "of course, %s", "as you wish, %s", "right away, %s"});
        }
    }

    if (HasAnyWord(lower, {"hi", "hello", "hey", "hiya", "yo", "sup", "hail", "howdy", "greetings", "wazzup",
                           "wassup", "morning", "afternoon", "evening"}))
    {
        return GetDialogText("dialog_greeting", placeholders,
                             {"hello %s", "hey there %s", "hi %s, good to see you", "greetings, %s",
                              "hello! hows it going %s", "hi %s, what brings you around", "well hello there %s",
                              "hey %s, fancy meeting you here"});
    }

    if (HasPhrase(lower, {"how are you", "how r u", "how you doing", "how are you doing", "whats up",
                          "what's up", "hows it going", "how is it going"}))
    {
        return GetDialogText("dialog_howareyou", placeholders,
                             {"im doing well %s, and you?", "cant complain %s, how about you?", "pretty good, you?",
                              "hanging in there %s", "all the better for seeing you %s", "better now that you asked %s"});
    }

    if (HasAnyWord(lower, {"thanks", "thank", "thx", "ty", "tyvm", "thnx"}) || HasPhrase(lower, {"thank you"}))
    {
        return GetDialogText("dialog_thanks", placeholders,
                             {"anytime %s", "youre welcome %s", "no problem at all %s", "happy to help %s",
                              "dont mention it %s", "it was nothing %s"});
    }

    if (HasAnyWord(lower, {"bye", "cya", "goodbye", "goodnight", "farewell", "later", "laters", "gn", "peace"}) ||
        HasPhrase(lower, {"good night", "see you", "see ya", "gotta go"}))
    {
        return GetDialogText("dialog_farewell", placeholders,
                             {"take care %s", "see you around %s", "bye %s, stay safe", "until next time %s",
                              "farewell %s", "dont be a stranger %s", "good luck out there %s"});
    }

    if (HasPhrase(lower, {"where are you"}))
    {
        return GetDialogText("dialog_where", placeholders,
                             {"im in %zone right now", "around %zone %s", "somewhere in %zone, you?", "im at %zone, want to group up?"});
    }

    if (HasPhrase(lower, {"what class", "which class"}))
    {
        return GetDialogText("dialog_class", placeholders,
                             {"im a %class %s", "a %class, why do you ask?", "im a proud %class", "i play a %class"});
    }

    if (HasPhrase(lower, {"what level", "which level", "your level"}))
    {
        return GetDialogText("dialog_level", placeholders,
                             {"level %level %s", "im level %level now", "%level, almost there",
                              "level %level, what about you %s?"});
    }

    if (HasPhrase(lower, {"what race", "which race"}))
    {
        return GetDialogText("dialog_race", placeholders,
                             {"im a %race %s", "a %race, born and raised", "%race through and through",
                              "im %race, and proud of it"});
    }

    bool const insulted = HasAnyWord(lower, {"noob", "idiot", "stupid", "loser", "dumb", "dummy", "moron", "shut",
                                             "trash", "suck", "lame"}) ||
                          HasPhrase(lower, {"shut up", "be quiet", "you suck"});

    if (insulted && PlayerbotAI::IsBotMentioned(bot, incomingMessage))
    {
        return GetDialogText("dialog_grudge", placeholders,
                             {"thats not very nice %s", "rude %s, very rude", "i try my best %s",
                              "excuse me? %s", "no need for that %s", "i will pretend i didnt hear that %s"});
    }

    if (isQuestion)
    {
        return GetDialogText("dialog_question", placeholders,
                             {"hmm, hard to say %s", "i dont really know %s", "good question %s",
                              "beats me %s", "who can say %s", "not sure about that one %s",
                              "you tell me %s", "maybe ask someone smarter than me %s"});
    }

    if (PlayerbotAI::IsBotMentioned(bot, incomingMessage))
    {
        return GetDialogText("dialog_callout", placeholders,
                             {"yes %s?", "did you call me %s?", "im listening %s", "what do you need %s?",
                              "you wanted me %s?", "right here %s"});
    }

    return GetDialogText("dialog_chatter", placeholders,
                         {"i see %s", "interesting %s", "yeah %s", "tell me more %s", "oh really %s",
                          "right %s", "i know what you mean %s", "fair enough %s", "sure %s"});
}

namespace
{
    bool IsNear(Player* bot, Player* target)
    {
        return target && bot && target->GetMapId() == bot->GetMapId() &&
               ServerFacade::instance().GetDistance2d(bot, target) <= sPlayerbotAIConfig.sightDistance;
    }

    std::string BuffAliasMatch(std::string const& lower)
    {
        static const std::vector<std::pair<std::string, std::string>> aliases = {
            {"arcane intellect", "arcane intellect"}, {"blessing of might", "blessing of might"},
            {"blessing of wisdom", "blessing of wisdom"}, {"blessing of kings", "blessing of kings"},
            {"power word: fortitude", "power word: fortitude"}, {"mark of the wild", "mark of the wild"},
            {"battle shout", "battle shout"}, {"commanding shout", "commanding shout"},
            {"horn of winter", "horn of winter"}, {"intellect", "arcane intellect"},
            {"might", "blessing of might"}, {"wisdom", "blessing of wisdom"}, {"kings", "blessing of kings"},
            {"fortitude", "power word: fortitude"}, {"mark", "mark of the wild"}, {"thorns", "thorns"}};

        for (auto const& alias : aliases)
        {
            if (lower.find(alias.first) != std::string::npos)
                return alias.second;
        }

        return "";
    }

    std::vector<std::string> BestBuffCandidates(Player* bot, Player* target)
    {
        std::vector<std::string> result;
        if (!target)
            return result;

        uint8 const cls = target->getClass();
        bool const caster = cls == CLASS_MAGE || cls == CLASS_PRIEST || cls == CLASS_WARLOCK;
        bool const melee = cls == CLASS_WARRIOR || cls == CLASS_PALADIN || cls == CLASS_ROGUE ||
                           cls == CLASS_HUNTER || cls == CLASS_DEATH_KNIGHT;

        switch (bot->getClass())
        {
            case CLASS_PRIEST:
                result.push_back("power word: fortitude");
                break;
            case CLASS_MAGE:
                if (caster)
                    result.push_back("arcane intellect");
                break;
            case CLASS_DRUID:
                result.push_back("mark of the wild");
                if (melee)
                    result.push_back("thorns");
                break;
            case CLASS_PALADIN:
                if (melee)
                {
                    result.push_back("blessing of might");
                    result.push_back("blessing of kings");
                }
                else if (caster)
                {
                    result.push_back("blessing of wisdom");
                    result.push_back("blessing of kings");
                }
                else
                {
                    result.push_back("blessing of kings");
                    result.push_back("blessing of might");
                    result.push_back("blessing of wisdom");
                }
                break;
            default:
                break;
        }

        return result;
    }

    bool CastBuffOnPlayer(PlayerbotAI* botAI, Player* target, std::string const& spellName)
    {
        CastCustomSpellAction action(botAI);
        return action.Execute(Event("cast custom spell", spellName + " on " + target->GetName()));
    }

    bool ConjureAndTradeTo(PlayerbotAI* botAI, Player* target, std::string const& conjureSpell,
                           std::string const& itemName)
    {
        if (!botAI->HasSpell(conjureSpell))
            return false;

        if (!botAI->CastSpell(conjureSpell, botAI->GetBot()))
            return false;

        TradeAction trade(botAI);
        return trade.TradeWith(target, itemName);
    }

    bool SendConversationAck(Player* bot, ChatChannelSource chatChannelSource, std::string const& ackName,
                             std::string const& name, std::initializer_list<std::string> const& fallbacks,
                             std::map<std::string, std::string> const& extra = {})
    {
        std::map<std::string, std::string> placeholders = BuildDialogPlaceholders(bot, name);
        for (auto const& placeholder : extra)
            placeholders[placeholder.first] = placeholder.second;

        std::string ack = GetDialogText(ackName, placeholders, fallbacks);
        if (ack.empty())
            return false;

        std::string receiver = name;
        return ChatReplyAction::SendGeneralResponse(bot, chatChannelSource, ack, receiver);
    }
}

uint32 ChatReplyAction::ClassifyConversationRequest(std::string const& message)
{
    std::string const lower = DialogToLower(message);
    uint32 flags = ConversationRequestFlags::NONE;

    if (HasPhrase(lower, {"invite me", "invite please", "invite us", "add me to your group", "add me to your party",
                          "add me to your raid", "can you invite", "could you invite", "let me join your group",
                          "let me join your party"}))
    {
        flags |= ConversationRequestFlags::INVITE;
    }

    if (HasPhrase(lower, {"buff me", "buff please", "buff me please", "give me a buff", "give me buff",
                          "need a buff", "want a buff", "any buffs", "buffs please"}) ||
        (HasPhrase(lower, {"give me", "need", "want", "cast", "could you", "can you", "please"}) &&
         HasPhrase(lower, {"buff", "might", "wisdom", "kings", "fortitude", "intellect", "arcane intellect",
                           "mark of the wild", "thorns", "blessing of might", "blessing of wisdom",
                           "blessing of kings", "power word: fortitude", "battle shout", "commanding shout",
                           "horn of winter"})))
    {
        flags |= ConversationRequestFlags::BUFF;
    }

    if (HasPhrase(lower, {"food please", "give me food", "give us food", "need food", "some food", "want food",
                          "conjure food", "conjured food", "any food", "food me"}))
    {
        flags |= ConversationRequestFlags::FOOD;
    }

    if (HasPhrase(lower, {"water please", "give me water", "give us water", "need water", "some water",
                          "want water", "conjure water", "conjured water", "any water", "give me a drink",
                          "a drink", "drink please"}))
    {
        flags |= ConversationRequestFlags::WATER;
    }

    return flags;
}

bool ChatReplyAction::HandleConversationAction(Player* bot, uint32 flags, std::string& msg, uint32& /*type*/,
                                               uint32& guid1, std::string& name, ChatChannelSource chatChannelSource)
{
    Player* requester = ObjectAccessor::FindPlayer(ObjectGuid(HighGuid::Player, guid1));
    if (!requester || requester == bot || !IsRealPlayer(requester))
        return false;

    PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
    if (!botAI)
        return false;

    if (flags & ConversationRequestFlags::INVITE)
    {
        if (!IsNear(bot, requester))
        {
            SendConversationAck(bot, chatChannelSource, "dialog_too_far", name,
                                {"come a bit closer %s", "i cant reach you from here %s", "youre too far away %s"});
            return true;
        }

        if (!bot->IsInCombat())
        {
            InviteToGroupAction action(botAI);
            Event event("invite", "", requester);
            action.Execute(event);
        }

        SendConversationAck(bot, chatChannelSource, "dialog_invite_ok", name,
                            {"welcome to the group %s", "sending you an invite %s", "sure, check your invite %s"});
        return true;
    }

    if (flags & ConversationRequestFlags::BUFF)
    {
        if (bot->IsInCombat() || !IsNear(bot, requester))
        {
            SendConversationAck(bot, chatChannelSource, "dialog_cannot_buff", name,
                                {"not right now %s", "i cant do that at the moment %s", "a bit busy %s"});
            return true;
        }

        std::string spellName = BuffAliasMatch(DialogToLower(msg));
        if (spellName.empty() || !botAI->HasSpell(spellName))
        {
            spellName.clear();
            for (std::string const& candidate : BestBuffCandidates(bot, requester))
            {
                if (botAI->HasSpell(candidate) && botAI->CanCastSpell(candidate, requester))
                {
                    spellName = candidate;
                    break;
                }
            }
        }

        if (spellName.empty())
        {
            SendConversationAck(bot, chatChannelSource, "dialog_cannot_buff", name,
                                {"sorry %s, i dont have a buff for you", "i cant buff you %s",
                                 "afraid i have nothing for you %s"});
            return true;
        }

        CastBuffOnPlayer(botAI, requester, spellName);
        SendConversationAck(bot, chatChannelSource, "dialog_buff_done", name,
                            {"there you go %s", "enjoy %s", "buffs are up %s"}, {{"%spell", spellName}});
        return true;
    }

    if (flags & (ConversationRequestFlags::FOOD | ConversationRequestFlags::WATER))
    {
        bool const wantFood = (flags & ConversationRequestFlags::FOOD) != 0;
        bool const wantWater = (flags & ConversationRequestFlags::WATER) != 0;

        if (bot->IsInCombat() || !IsNear(bot, requester) || bot->getClass() != CLASS_MAGE)
        {
            SendConversationAck(bot, chatChannelSource, "dialog_cannot_conjure", name,
                                {"sorry %s, i cant conjure that", "i dont conjure those %s",
                                 "not a mage, sorry %s"});
            return true;
        }

        bool acted = false;
        if (wantFood)
            acted = ConjureAndTradeTo(botAI, requester, "conjure food", "conjured food") || acted;

        if (wantWater)
            acted = ConjureAndTradeTo(botAI, requester, "conjure water", "conjured water") || acted;

        if (acted)
        {
            SendConversationAck(bot, chatChannelSource, "dialog_conjured_done", name,
                                {"there you go %s", "freshly conjured %s", "enjoy %s", "here you go %s"});
        }
        else
        {
            SendConversationAck(bot, chatChannelSource, "dialog_cannot_conjure", name,
                                {"sorry %s, conjuring didnt work", "give me a moment %s, conjuring failed"});
        }
        return true;
    }

    return false;
}
