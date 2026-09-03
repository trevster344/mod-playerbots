/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_ZONECHATTRIGGER_H
#define PLAYERBOTS_ZONECHATTRIGGER_H

#include "Trigger.h"

class PlayerbotAI;

class ZoneChatTrigger : public Trigger
{
public:
    ZoneChatTrigger(PlayerbotAI* botAI) : Trigger(botAI, "zone chatter") {}

    bool IsActive() override;
};

#endif
