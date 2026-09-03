/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_ZONECHATACTION_H
#define PLAYERBOTS_ZONECHATACTION_H

#include "Action.h"

class PlayerbotAI;

class ZoneChatAction : public Action
{
public:
    ZoneChatAction(PlayerbotAI* botAI) : Action(botAI, "zone chatter") {}

    bool Execute(Event event) override;

    static std::string NormalizeZoneKey(std::string const& name);
};

#endif
