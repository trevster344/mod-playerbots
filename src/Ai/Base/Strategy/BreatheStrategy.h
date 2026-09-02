/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_BREATHESTRATEGY_H
#define PLAYERBOTS_BREATHESTRATEGY_H

#include "Strategy.h"

class PlayerbotAI;

class BreatheStrategy : public Strategy
{
public:
    BreatheStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

    uint32 GetType() const override { return STRATEGY_TYPE_NONCOMBAT | STRATEGY_TYPE_COMBAT; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    std::string const getName() override { return "surface"; }
};

#endif
