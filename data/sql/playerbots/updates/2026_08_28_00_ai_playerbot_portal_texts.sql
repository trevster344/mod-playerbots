-- Portal command texts (paid mage portals)
DELETE FROM ai_playerbot_texts WHERE name IN (
    'portal_in_combat',
    'portal_bot_dead',
    'portal_in_flight',
    'portal_usage',
    'portal_no_spell',
    'portal_too_far',
    'portal_no_reagents',
    'portal_not_enough_money',
    'portal_cast_failed',
    'portal_trade_prompt',
    'portal_trade_wrong_amount',
    'portal_success'
);

INSERT INTO ai_playerbot_texts (name, text, say_type, reply_type, text_loc1, text_loc2, text_loc3, text_loc4, text_loc5, text_loc6, text_loc7, text_loc8) VALUES
('portal_in_combat', 'I can''t open a portal while in combat', 0, 0, '', '', '', '', '', '', '', ''),
('portal_bot_dead', 'I can''t open a portal while dead', 0, 0, '', '', '', '', '', '', '', ''),
('portal_in_flight', 'I can''t open a portal while on a vehicle or in flight', 0, 0, '', '', '', '', '', '', '', ''),
('portal_usage', 'Tell me the city, e.g. "portal stormwind"', 0, 0, '', '', '', '', '', '', '', ''),
('portal_no_spell', 'I don''t know a portal to that city', 0, 0, '', '', '', '', '', '', '', ''),
('portal_too_far', 'Come closer so you can use the portal', 0, 0, '', '', '', '', '', '', '', ''),
('portal_no_reagents', 'I''m out of Runes of Portals', 0, 0, '', '', '', '', '', '', '', ''),
('portal_cast_failed', 'Something went wrong while opening the portal', 0, 0, '', '', '', '', '', '', '', ''),
('portal_trade_prompt', 'That''ll be %cost - trade me the gold and I''ll open a portal to %city', 0, 0, '', '', '', '', '', '', '', ''),
('portal_trade_wrong_amount', 'Please trade exactly %cost for the portal', 0, 0, '', '', '', '', '', '', '', ''),
('portal_success', 'Here''s your portal to %city (%cost)', 0, 0, '', '', '', '', '', '', '', '');
