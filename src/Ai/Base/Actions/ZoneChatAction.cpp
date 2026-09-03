/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "ZoneChatAction.h"
#include "Common.h"
#include "DBCStores.h"
#include "Event.h"
#include "PlayerbotAI.h"
#include "PlayerbotTextMgr.h"
#include "Playerbots.h"
#include <cctype>
#include <map>

std::string ZoneChatAction::NormalizeZoneKey(std::string const& name)
{
    std::string out;
    bool lastSeparator = false;

    for (char raw : name)
    {
        unsigned char ch = static_cast<unsigned char>(raw);
        if (std::isalnum(ch))
        {
            out += static_cast<char>(std::tolower(ch));
            lastSeparator = false;
        }
        else if (!out.empty() && !lastSeparator)
        {
            out += '_';
            lastSeparator = true;
        }
    }

    if (!out.empty() && out.back() == '_')
        out.pop_back();

    return out;
}

bool ZoneChatAction::Execute(Event /*event*/)
{
    AreaTableEntry const* zone = botAI->GetCurrentZone();
    if (!zone)
        return false;

    std::string const zoneName = PlayerbotAI::GetLocalizedAreaName(zone);
    std::string const zoneKey = NormalizeZoneKey(std::string(zone->area_name[LOCALE_enUS]));

    std::map<std::string, std::string> placeholders;
    placeholders["%zone"] = zoneName;

    std::string text;
    std::string const groupName = "zonechat_" + zoneKey;
    if (PlayerbotTextMgr::instance().HasText(groupName))
        text = PlayerbotTextMgr::instance().GetBotText(groupName, placeholders);

    if (text.empty() && PlayerbotTextMgr::instance().HasText("zonechat_generic"))
        text = PlayerbotTextMgr::instance().GetBotText("zonechat_generic", placeholders);

    if (text.empty())
        return false;

    return botAI->SayToChannel(text, ChatChannelId::GENERAL);
}
