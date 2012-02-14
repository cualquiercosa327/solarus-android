-- Dungeon 8 1F

-- Legend
-- RC: Rupee Chest
-- KC: Key Chest
-- KP: Key Pot
-- LD: Locked Door
-- KD: Key Door
-- DB: Door Button
-- LB: Locked Barrier
-- BB: Barrier Button
-- DS: Door Sensor

fighting_boss = false

function event_map_started(destination_point_name)
  sol.map.door_set_open("LD1", true)
  sol.map.door_set_open("LD3", true)
  sol.map.door_set_open("LD4", true)
  sol.map.npc_set_enabled("billy_npc", false)
  sol.map.door_set_open("boss_door", true)

  -- Map chest hide if not already opened
  if not sol.game.savegame_get_boolean(700) then
    sol.map.chest_set_enabled("MAP", false)
  end

  -- Big key chest hide if not already opened
  if not sol.game.savegame_get_boolean(705) then
    sol.map.chest_set_enabled("BK01", false)
  end

  -- Link has mirror shield: no laser obstacles
  if sol.game.get_ability("shield") >= 3 then
    sol.map.obstacle_set_enabled("LO1", false)
    sol.map.obstacle_set_group_enabled("LO2", false)
  end

  if destination_point_name == "from_boss" or destination_point_name == "from_hidden_room" then
    sol.map.door_set_open("LD5", true)
  end

  if destination_point_name == "from_hidden_room" then
    sol.map.enemy_remove_group("room_LD5_enemy")
  end

  -- door to Agahnim open if Billy's heart container was picked
  if sol.game.savegame_get_boolean(729) then
    sol.map.door_set_open("agahnim_door", true)
  end

  -- statues puzzle
  if sol.game.savegame_get_boolean(723) then
    sol.map.switch_set_activated("DB06", true)
  end

  -- boss key door and laser
  if sol.game.savegame_get_boolean(730) then
    sol.map.enemy_remove("boss_key_door_laser")
  end
end

function event_map_opening_transition_finished(destination_point_name)
  if destination_point_name == "from_outside" then
    sol.map.dialog_start("dungeon_8.welcome")
  end
end

function event_switch_activated(switch_name)
  if switch_name == "BB1" then
    -- LB1 room
    sol.map.camera_move(896, 1712, 250, BB1_remove_barrier)
  elseif switch_name == "BB2" then
    -- LB2 room
    sol.map.tile_set_enabled("LB2", false)
    sol.main.play_sound("secret")
  elseif switch_name == "DB4" then
    sol.map.door_open("LD4")
    sol.main.play_sound("secret")
  elseif switch_name == "DB06" then
    -- 4 statues room door opening
    sol.map.door_open("LD6")
    sol.main.play_sound("secret")
  elseif string.match(switch_name, "^RPS") then
    -- Resets position of statues
    sol.map.block_reset("STT1")
    sol.map.block_reset("STT2")
    sol.map.block_reset("STT3")
    sol.main.play_sound("warp")
  end
end

function BB1_remove_barrier()
  sol.map.tile_set_enabled("LB1", false)
  sol.main.play_sound("secret")
end

function event_hero_on_sensor(sensor_name)
  if sensor_name == "DS1" then
    -- LD1 room: when Link enters this room, door LD1 closes and enemies appear, sensor DS1 is disabled
    if not sol.map.enemy_is_group_dead("room_LD1_enemy") then
      sol.map.door_close("LD1")
      sol.map.sensor_set_enabled("DS1", false)
    end
  elseif sensor_name == "DS3" then
    if not sol.map.enemy_is_group_dead("map_enemy") then
      sol.map.door_close("LD3")
      sol.map.sensor_set_enabled("DS3", false)
    end
  elseif sensor_name == "DS4" then
    sol.map.door_close("LD4")
    sol.map.sensor_set_enabled("DS4", false)
  elseif sensor_name == "start_boss_sensor" then
    if not fighting_boss and not sol.game.savegame_get_boolean(727) then
      sol.main.play_music("none")
      sol.map.door_close("boss_door")
      sol.map.npc_set_enabled("billy_npc", true)
      sol.map.hero_freeze()
      fighting_boss = true
      sol.main.timer_start(function()
	sol.map.dialog_start("dungeon_8.billy")
      end, 1000)
    end
  end
end

function event_enemy_dead(enemy_name)
  if string.match(enemy_name, "^room_LD1_enemy") and sol.map.enemy_is_group_dead("room_LD1_enemy") then
    -- LD1 room: kill all enemies will open the doors LD1 and LD2
    if not sol.map.door_is_open("LD1") then
      sol.map.door_open("LD1")
      sol.map.door_open("LD2")
      sol.main.play_sound("secret")
    end
  elseif string.match(enemy_name, "^room_LD5_enemy") and sol.map.enemy_is_group_dead("room_LD5_enemy") and not sol.map.door_is_open("LD5") then
    -- LD5 room: kill all enemies will open the door LD5
    sol.map.door_open("LD5")
    sol.main.play_sound("secret")
  elseif string.match(enemy_name, "^map_enemy") and sol.map.enemy_is_group_dead("map_enemy") then
    -- Map chest room: kill all enemies and the chest will appear
    if not sol.game.savegame_get_boolean(700) then
      sol.map.chest_set_enabled("MAP", true)
      sol.main.play_sound("chest_appears")
    elseif not sol.map.door_is_open("LD3") then
      sol.main.play_sound("secret")
    end
    sol.map.door_open("LD3")
  elseif string.match(enemy_name, "^room_big_enemy") and sol.map.enemy_is_group_dead("room_big_enemy") then
    -- Big key chest room: kill all enemies and the chest will appear
    if not sol.game.savegame_get_boolean(705) then
      sol.map.chest_set_enabled("BK01", true)
      sol.main.play_sound("chest_appears")
    end
  end
end

function event_treasure_obtained(item_name, variant, savegame_variable)

  if item_name == "heart_container" then
    sol.main.play_music("victory.spc")
    sol.map.hero_freeze()
    sol.map.hero_set_direction(3)
    sol.main.timer_start(function()
      sol.map.door_open("boss_door")
      sol.map.door_open("agahnim_door")
      sol.main.play_sound("secret")
      sol.map.hero_unfreeze()
    end, 9000)
  end
end

function event_dialog_finished(dialog_id)

  if dialog_id == "dungeon_8.billy" then
    sol.main.play_music("boss.spc")
    sol.map.hero_unfreeze()
    sol.map.enemy_set_enabled("boss", true)
    sol.map.npc_set_enabled("billy_npc", false)
  end
end

function event_door_open(door_name)

  if door_name == "boss_key_door" then
    sol.map.enemy_remove("boss_key_door_laser")
  elseif door_name == "WW01" then
    sol.main.play_sound("secret")
  end
end

