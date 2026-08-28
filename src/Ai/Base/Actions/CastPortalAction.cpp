/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "CastPortalAction.h"
#include "ChatHelper.h"
#include "Event.h"
#include "Helpers.h"
#include "PlayerbotAIConfig.h"
#include "PlayerbotTextMgr.h"
#include "Playerbots.h"
#include "PortalValue.h"

#include <algorithm>
#include <cctype>

bool CastPortalAction::Execute(Event event)
{
    if (!sPlayerbotAIConfig.portalEnabled || !sPlayerbotAIConfig.portalCost)
        return false;

    Player* master = event.getOwner();
    if (!master)
        return false;

    // Only mages open portals; ignore the command silently for other classes.
    if (bot->getClass() != CLASS_MAGE)
        return false;

    if (bot->IsInCombat())
    {
        botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "portal_in_combat", "I can't open a portal while in combat", {}));
        return false;
    }

    if (!bot->IsAlive())
    {
        botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "portal_bot_dead", "I can't open a portal while dead", {}));
        return false;
    }

    if (botAI->IsInVehicle() || bot->IsInFlight())
    {
        botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "portal_in_flight", "I can't open a portal while on a vehicle or in flight", {}));
        return false;
    }

    std::string city = event.getParam();
    trim(city);
    if (city.empty())
    {
        botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "portal_usage", "Tell me the city, e.g. \"portal stormwind\"", {}));
        return false;
    }

    std::string lowered = city;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    if (lowered.rfind("to ", 0) == 0)
    {
        city = city.substr(3);
        trim(city);
    }

    if (city.empty())
        return false;

    uint32 spellId = AI_VALUE2(uint32, "spell id", "portal: " + city);
    if (!spellId)
    {
        botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "portal_no_spell", "I don't know a portal to that city", {}));
        return false;
    }

    if (master->IsBeingTeleported())
        return false;

    if (bot->GetDistance(master) > sPlayerbotAIConfig.sightDistance)
    {
        botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "portal_too_far", "Come closer so you can use the portal", {}));
        return false;
    }

    if (!bot->HasItemCount(17032, 2))
    {
        botAI->TellError(PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "portal_no_reagents", "I'm out of Runes of Portals", {}));
        return false;
    }

    if (!botAI->CanCastSpell(spellId, bot))
        return false;

    uint32 cost = sPlayerbotAIConfig.portalCost;

    PortalData& portal = AI_VALUE(PortalData&, "portal");
    portal.Reset();
    portal.enabled = true;
    portal.payerGuid = master->GetGUID();
    portal.cost = cost;
    portal.spellId = spellId;
    portal.city = city;

    botAI->TellMasterNoFacing(PlayerbotTextMgr::instance().GetBotTextOrDefault(
        "portal_trade_prompt", "That'll be %cost - trade me the gold and I'll open a portal to %city",
        {{"%cost", chat->formatMoney(cost)}, {"%city", city}}));

    if (!bot->GetTrader())
    {
        WorldPacket packet(CMSG_INITIATE_TRADE);
        packet << master->GetGUID();
        bot->GetSession()->HandleInitiateTradeOpcode(packet);
    }

    return true;
}
