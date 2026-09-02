/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "BreatheStrategy.h"
#include "Playerbots.h"

void BreatheStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    // Priority above normal movement (follow, move to target, formation, reach
    // spell) but below interrupts/emergency actions so combat keeps functioning.
    triggers.push_back(
        new TriggerNode("low air", { NextAction("surface to breathe", ACTION_MOVE + 10) }));
}
