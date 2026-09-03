/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "ZoneChatTrigger.h"
#include "Common.h"
#include "DBCStores.h"
#include "Map.h"
#include "PlayerbotAI.h"
#include "PlayerbotAIConfig.h"
#include "Playerbots.h"
#include "Random.h"
#include "ZoneChatAction.h"
#include "ZoneChatLimiter.h"

bool ZoneChatTrigger::IsActive()
{
    if (!sPlayerbotAIConfig.zoneChat)
        return false;

    if (!botAI->AllowActivity())
        return false;

    if (botAI->HasStrategy("silent", BotState::BOT_STATE_NON_COMBAT))
        return false;

    if (!bot || !bot->GetSession() || !bot->GetMap())
        return false;

    if (bot->IsInCombat() || bot->IsInBattleground())
        return false;

    Map* map = bot->GetMap();
    if (map->IsDungeon() || map->IsBattleground() || map->IsBattleArena())
        return false;

    AreaTableEntry const* zone = botAI->GetCurrentZone();
    if (!zone)
        return false;

    std::string const zoneKey = ZoneChatAction::NormalizeZoneKey(std::string(zone->area_name[LOCALE_enUS]));

    time_t nextSay = botAI->GetAiObjectContext()->GetValue<time_t>("last said", "zone chatter")->Get();
    if (time(nullptr) < nextSay)
        return false;

    if (!ZoneChatLimiter::instance().TryAllow(zoneKey, sPlayerbotAIConfig.zoneChatMinGapSeconds))
        return false;

    botAI->GetAiObjectContext()->GetValue<time_t>("last said", "zone chatter")->Set(
        time(nullptr) + urand(sPlayerbotAIConfig.zoneChatMinInterval, sPlayerbotAIConfig.zoneChatMaxInterval));

    return true;
}
