/*
 * Copyright (C) 2009-2011 Christopho, Solarus - http://www.solarus-engine.org
 *
 * Solarus is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Solarus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "lua/Script.h"
#include "Game.h"
#include "Map.h"
#include "DialogBox.h"
#include "Treasure.h"
#include "entities/MapEntities.h"
#include "entities/Door.h"
#include "entities/NPC.h"
#include "entities/Sensor.h"
#include "entities/Chest.h"
#include "entities/DynamicTile.h"
#include "entities/Stairs.h"
#include "entities/CustomObstacle.h"
#include "entities/Block.h"
#include "entities/Switch.h"
#include "entities/Crystal.h"
#include "entities/Teletransporter.h"
#include "entities/Hero.h"
#include "entities/PickableItem.h"
#include "entities/DestructibleItem.h"
#include "entities/Bomb.h"
#include "entities/Fire.h"
#include "lowlevel/Sound.h"
#ifdef ANDROID
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}
#else
#include <lua.hpp>
#endif

/**
 * @brief Shows the dialog box and starts displaying a dialog.
 *
 * If the dialog contains a variable,
 * then you have to call sol.map.dialog_set_variable() to specify its value.
 *
 * - Argument 1 (string): id of the dialog to display
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_dialog_start(lua_State* l) {

  Script& script = get_script(l, 1);
  const std::string& dialog_id = luaL_checkstring(l, 1);

  script.get_game().get_dialog_box().start_dialog(dialog_id, &script);

  return 0;
}

/**
 * @brief Sets the value of the variable in a dialog.
 *
 * - Argument 1 (string): id of the dialog containing the variable
 * - Argument 2 (string): value of the variable
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_dialog_set_variable(lua_State* l) {

  Script& script = get_script(l, 2);
  const std::string& dialog_id = luaL_checkstring(l, 1);
  const std::string& value = luaL_checkstring(l, 2);

  script.get_game().get_dialog_box().set_variable(dialog_id, value);

  return 0;
}

/**
 * @brief Changes the style of the future dialog boxes.
 *
 * TODO: use a string instead
 *
 * - Argument 1 (integer): the style to set (see the DialogBox::Style enum)
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_dialog_set_style(lua_State *l) {

  Script& script = get_script(l, 1);
  int style = luaL_checkinteger(l, 1);

  script.get_game().get_dialog_box().set_style(DialogBox::Style(style));

  return 0;
}

/**
 * @brief Enables or disables the head up display.
 *
 * - Argument 1 (boolean): true to enable it, false to disable it
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_hud_set_enabled(lua_State *l) {

  Script& script = get_script(l, 1);
  bool enabled = lua_toboolean(l, 1) != 0;

  script.get_game().set_hud_enabled(enabled);
  return 0;
}

/**
 * @brief Sets whether the player can pause the game.
 *
 * - Argument 1 (boolean): true to enable the pause key
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_hud_set_pause_enabled(lua_State *l) {

  Script& script = get_script(l, 1);
  bool pause_key_available = lua_toboolean(l, 1) != 0;

  script.get_game().set_pause_key_available(pause_key_available);

  return 0;
}

/**
 * @brief Returns the current level of light of the map.
 *
 * - Return value (integer): 0 for no light, a positive value if there is some light
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_light_get(lua_State *l) {

  Script& script = get_script(l, 0);

  int light = script.get_map().get_light();

  lua_pushinteger(l, light);

  return 1;
}

/**
 * @brief Sets the level of light of the map.
 *
 * - Argument 1 (integer): the level of light
 * (0: no light, a positive value: full light)
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_light_set(lua_State *l) {

  Script& script = get_script(l, 1);

  int light = luaL_checkinteger(l, 1);

  script.get_map().set_light(light);

  return 0;
}

/**
 * @brief Moves the camera towards a target point, calls a function and restores the camera.
 *
 * This function returns immediately but the game is suspended during the whole sequence.
 *
 * - Argument 1 (integer): x coordinate of the target point
 * - Argument 2 (integer): y coordinate of the target point
 * - Argument 3 (integer): speed of the camera movement in pixels per second (250 is a usual speed)
 * - Argument 4 (function): a Lua function to call when the camera reaches its target
 * - Optional argument 5 (integer): delay in milliseconds before calling the function (default 1000)
 * - Optional argument 6 (integer): delay in milliseconds after calling the function (default 1000)
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_camera_move(lua_State* l) {

  Script& script = get_script(l, 4, 6);
  int x = luaL_checkinteger(l, 1);
  int y = luaL_checkinteger(l, 2);
  int speed = luaL_checkinteger(l, 3);
  luaL_checktype(l, 4, LUA_TFUNCTION);

  uint32_t delay_before = 1000;
  uint32_t delay_after = 1000;
  if (lua_gettop(l) >= 5) {
    delay_before = luaL_checkinteger(l, 5);
    if (lua_gettop(l) >= 6) {
      delay_after = luaL_checkinteger(l, 6);
    }
  }
  lua_settop(l, 4); // let the function on top of the stack

  // store the function and the delays
  lua_setfield(l, LUA_REGISTRYINDEX, "sol.camera_function");
  lua_pushinteger(l, delay_before);
  lua_setfield(l, LUA_REGISTRYINDEX, "sol.camera_delay_before");
  lua_pushinteger(l, delay_after);
  lua_setfield(l, LUA_REGISTRYINDEX, "sol.camera_delay_after");

  // start the camera
  script.get_game().get_current_map().move_camera(x, y, speed);

  return 0;
}

/**
 * @brief Gives a treasure to the hero.
 *
 * If the treasure comes from a chest, you don't have to call this function:
 * the treasure will be given to the player automatically when he opens the chest.
 * You can use this function to make a non-playing character
 * give a treasure to the player.
 *
 * - Argument 1 (integer): name of the item to give (according to the item list of items.dat)
 * - Argument 2 (integer): variant of this item (1 if the item has only one variant)
 * - Argument 3 (integer): index of the savegame boolean variable that stores
 * the possession state of the treasure (or -1 if you don't want to save this treasure)
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_treasure_give(lua_State *l) {

  Script& script = get_script(l, 3);
  const std::string &item_name = luaL_checkstring(l, 1);
  int variant = luaL_checkinteger(l, 2);
  int savegame_variable = luaL_checkinteger(l, 3);

  script.get_game().get_hero().start_treasure(
      Treasure(script.get_game(), item_name, variant, savegame_variable));

  return 0;
}

/**
 * @brief Displays a sprite on the map surface.
 *
 * - Argument 1 (sprite): the sprite to display
 * - Argument 2 (integer): x coordinate on the map
 * - Argument 3 (integer): y coordinate on the map
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_sprite_display(lua_State *l) {

  Script& script = get_script(l, 3);
  int sprite_handle = luaL_checkinteger(l, 1);
  int x = luaL_checkinteger(l, 2);
  int y = luaL_checkinteger(l, 3);

  Sprite& sprite = script.get_sprite(sprite_handle);
  script.get_map().display_sprite(sprite, x, y);

  return 0;
}

/**
 * @brief Returns the id of the tileset used to draw the map.
 *
 * - Return value (integer): id of the tileset
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_tileset_get(lua_State *l) {

  Script& script = get_script(l, 0);

  TilesetId id = script.get_map().get_tileset_id();

  lua_pushinteger(l, id);

  return 1;
}

/**
 * @brief Changes the tileset used to draw this map.
 *
 * Every tile of this old tileset must exist in the new one.
 *
 * - Argument 1 (integer): id of the new tileset
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_tileset_set(lua_State *l) {

  Script& script = get_script(l, 1);

  TilesetId id = luaL_checkinteger(l, 1);

  script.get_map().set_tileset(id);

  return 0;
}

/**
 * @brief Prevents the player from moving until hero_unfreeze() is called.
 * @param l the Lua context that is calling this function
 */
int Script::map_api_hero_freeze(lua_State *l) {

  Script& script = get_script(l, 0);

  script.get_game().get_hero().start_freezed();

  return 0;
}

/**
 * @brief Allows the player to move again after a call to hero_freeze().
 * @param l the Lua context that is calling this function
 */
int Script::map_api_hero_unfreeze(lua_State *l) {

  Script& script = get_script(l, 0);

  script.get_game().get_hero().start_free();

  return 0;
}

/**
 * @brief Sends the hero to a map.
 *
 * - Argument 1 (int): id of the destination map (can be the same one)
 * - Argument 2 (string): name of the destination point on that map
 * - Argument 3 (int): type of transition to play (must match Transition::Style)
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_hero_set_map(lua_State *l) {

  Script& script = get_script(l, 3);

  MapId map_id = luaL_checkinteger(l, 1);
  const std::string &destination_point_name = luaL_checkstring(l, 2);
  Transition::Style transition_style = Transition::Style(luaL_checkinteger(l, 3));

  script.get_game().set_current_map(map_id, destination_point_name, transition_style);

  return 0;
}

/**
 * @brief Hides or shows the hero.
 *
 * Hiding the hero does not disable its movements, so when using this function
 * you will usually also need to freeze the hero.
 * - Argument 1 (boolean): true to make the hero visible
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_hero_set_visible(lua_State *l) {

  Script& script = get_script(l, 1);

  bool visible = lua_toboolean(l, 1) != 0;

  script.get_game().get_hero().set_visible(visible);

  return 0;
}

/**
 * @brief Returns the current direction of the hero's sprite.
 *
 * - Return value (integer): the direction between 0 and 3
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_hero_get_direction(lua_State *l) {

  Script& script = get_script(l, 0);

  int direction = script.get_game().get_hero().get_animation_direction();
  lua_pushinteger(l, direction);

  return 1;
}

/**
 * @brief Sets the direction of the hero's sprite.
 *
 * - Argument 1 (integer): the direction between 0 and 3
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_hero_set_direction(lua_State *l) {

  Script& script = get_script(l, 1);

  int direction = luaL_checkinteger(l, 1);

  script.get_game().get_hero().set_animation_direction(direction);

  return 0;
}

/**
 * @brief Returns the position of the hero.
 *
 * - Return value 1 (integer): x coordinate
 * - Return value 2 (integer): y coordinate
 * - Return value 3 (integer): layer (0 to 2)
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_hero_get_position(lua_State *l) {

  Script& script = get_script(l, 0);

  Hero &hero = script.get_game().get_hero();

  lua_pushinteger(l, hero.get_x());
  lua_pushinteger(l, hero.get_y());
  lua_pushinteger(l, hero.get_layer());

  return 3;
}

/**
 * @brief Sets the position of the hero.
 *
 * - Argument 1 (integer): x coordinate
 * - Argument 2 (integer): y coordinate
 * - Optional argument 3 (integer): layer (if unspecified,
 * the layer will be unchanged)
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_hero_set_position(lua_State* l) {

  Script& script = get_script(l, 2, 3);

  int x = luaL_checkinteger(l, 1);
  int y = luaL_checkinteger(l, 2);
  int layer = -1;
  if (lua_gettop(l) >= 3) {
    layer = luaL_checkinteger(l, 3);
  }

  Hero& hero = script.get_game().get_hero();
  hero.set_xy(x, y);

  if (layer != -1) {
    script.get_map().get_entities().set_entity_layer(hero, Layer(layer));
  }
  hero.check_position();

  return 0;
}

/**
 * @brief Places the hero on the exact position of a sensor's name.
 *
 * - Argument 1 (string): name of the sensor
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_hero_align_on_sensor(lua_State *l) {

  Script& script = get_script(l, 1);

  const std::string &name = luaL_checkstring(l, 1);

  MapEntities &entities = script.get_map().get_entities();
  Sensor *sensor = (Sensor*) entities.get_entity(SENSOR, name);
  script.get_game().get_hero().set_xy(sensor->get_xy());

  return 0;
}

/**
 * @brief Memorizes a position to go back to if the hero falls into a hole or
 * other bad ground.
 *
 * The parameters are optional: by default, they correspond to the current
 * position of the hero.
 *
 * - Optional argument 1 (integer): x coordinate
 * - Optional argument 2 (integer): y coordinate
 * - Optional argument 3 (integer): layer
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_hero_save_solid_ground(lua_State* l) {

  Script& script = get_script(l, 0, 3);
  Hero& hero = script.get_game().get_hero();

  int x, y, layer;
  if (lua_gettop(l) >= 3) {
    x = luaL_checkinteger(l, 1);
    y = luaL_checkinteger(l, 2);
    layer = luaL_checkinteger(l, 3);
  }
  else {
    x = hero.get_x();
    y = hero.get_y();
    layer = hero.get_layer();
  }

  hero.set_target_solid_ground_coords(Rectangle(x, y), Layer(layer));

  return 0;
}

/**
 * @brief Forgets the memorized position to go back to if the hero falls into
 * a hole or other bad ground.
 * @param l the Lua context that is calling this function
 */
int Script::map_api_hero_reset_solid_ground(lua_State* l) {

  Script& script = get_script(l, 0);
  Hero& hero = script.get_game().get_hero();

  hero.reset_target_solid_ground_coords();

  return 0;
}

/**
 * @brief Makes the hero walk with respect to a path.
 *
 * - Argument 1 (string): the path (each character is a direction between '0' and '7')
 * - Argument 2 (boolean): true to make the movement loop
 * - Argument 3 (boolean): true to make the movement sensible to obstacles
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_hero_walk(lua_State *l) {

  Script& script = get_script(l, 3);

  const std::string &path = luaL_checkstring(l, 1);
  bool loop = lua_toboolean(l, 2) != 0;
  bool ignore_obstacles = lua_toboolean(l, 3) != 0;

  script.get_game().get_hero().start_forced_walking(path, loop, ignore_obstacles);

  return 0;
}

/**
 * @brief Makes the hero jump into a direction.
 *
 * - Argument 1 (integer): the jump direction, between 0 and 7
 * - Argument 2 (integer): the jump length in pixels
 * - Argument 3 (boolean): true to enable the collisions
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_hero_start_jumping(lua_State *l) {

  Script& script = get_script(l, 3);

  int direction = luaL_checkinteger(l, 1);
  int length = luaL_checkinteger(l, 2);
  bool ignore_obstacles = lua_toboolean(l, 3) != 0;

  script.get_game().get_hero().start_jumping(direction, length, ignore_obstacles, false);

  return 0;
}

/**
 * @brief Makes the hero brandish his sword meaning a victory
 * and plays the corresponding sound.
 * @param l the Lua context that is calling this function
 */
int Script::map_api_hero_start_victory_sequence(lua_State *l) {

  Script& script = get_script(l, 0);

  script.get_game().get_hero().start_victory();

  return 0;
}

/**
 * @brief Makes the hero shoot a boomerang.
 *
 * - Argument 1 (integer): maximum distance of the movement in pixels
 * - Argument 2 (integer): speed of the movement in pixels per second
 * - Argument 3 (string): animation name of the hero's tunic sprite
 * when preparing the boomerang
 * - Argument 3 (string): animation set id that represents the boomerang
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_hero_start_boomerang(lua_State* l) {

  Script& script = get_script(l, 4);

  int max_distance = luaL_checkinteger(l, 1);
  int speed = luaL_checkinteger(l, 2);
  const std::string& tunic_preparing_animation = luaL_checkstring(l, 3);
  const SpriteAnimationSetId& sprite_name = luaL_checkstring(l, 4);

  script.get_game().get_hero().start_boomerang(max_distance, speed,
      tunic_preparing_animation, sprite_name);

  return 0;
}

/**
 * @brief Makes the hero shoot an arrow with a bow.
 * @param l the Lua context that is calling this function
 */
int Script::map_api_hero_start_bow(lua_State *l) {

  Script& script = get_script(l, 0);

  script.get_game().get_hero().start_bow();

  return 0;
}

/**
 * @brief Makes the hero thow his hookshot.
 * @param l the Lua context that is calling this function
 */
int Script::map_api_hero_start_hookshot(lua_State *l) {

  Script& script = get_script(l, 0);

  script.get_game().get_hero().start_hookshot();

  return 0;
}

/**
 * @brief Makes the hero run.
 * @param l the Lua context that is calling this function
 */
int Script::map_api_hero_start_running(lua_State *l) {

  Script& script = get_script(l, 0);

  script.get_game().get_hero().start_running();

  return 0;
}

/**
 * @brief Makes the hero be hurt.
 *
 * - Argument 1 (integer): x coordinate of whatever is hurting the hero
 * - Argument 2 (integer): y coordinate of whatever is hurting the hero
 * - Argument 3 (integer): number of life points to remove (possibly zero,
 * and may be reduced by the tunic)
 * - Argument 4 (integer): number of magic points to remove (possibly zero)
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_hero_start_hurt(lua_State *l) {

  Script& script = get_script(l, 4);

  int source_x = luaL_checkinteger(l, 1);
  int source_y = luaL_checkinteger(l, 2);
  int life_points = luaL_checkinteger(l, 3);
  int magic_points = luaL_checkinteger(l, 4);

  script.get_game().get_hero().hurt(Rectangle(source_x, source_y),
      life_points, magic_points);

  return 0;
}

/**
 * @brief Returns whether an NPC is enabled.
 *
 * - Argument 1 (string): name of the NPC
 * - Return value (boolean): true if this NPC is enabled
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_npc_is_enabled(lua_State* l) {

  Script& script = get_script(l, 1);

  const std::string& name = luaL_checkstring(l, 1);

  MapEntities& entities = script.get_map().get_entities();
  MapEntity* npc = entities.get_entity(NON_PLAYING_CHARACTER, name);
  lua_pushboolean(l, npc->is_enabled());

  return 1;
}

/**
 * @brief Enables or disables an NPC.
 *
 * - Argument 1 (string): name of the NPC
 * - Argument 2 (boolean): true to enable the NPC, false to disable it
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_npc_set_enabled(lua_State* l) {

  Script& script = get_script(l, 2);

  const std::string& name = luaL_checkstring(l, 1);
  bool enabled = lua_toboolean(l, 2);

  MapEntities& entities = script.get_map().get_entities();
  MapEntity* npc = entities.get_entity(NON_PLAYING_CHARACTER, name);
  npc->set_enabled(enabled);

  return 0;
}

/**
 * @brief Enables or disables a set of NPCs.
 *
 * - Argument 1 (string): prefix of the name of the NPCs
 * - Argument 2 (boolean): true to enable them, false to disable them
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_npc_set_group_enabled(lua_State* l) {

  Script& script = get_script(l, 2);

  const std::string& prefix = luaL_checkstring(l, 1);
  bool enable = lua_toboolean(l, 2);

  std::list<MapEntity*> entities =
      script.get_map().get_entities().get_entities_with_prefix(NON_PLAYING_CHARACTER, prefix);

  std::list<MapEntity*>::iterator it;
  for (it = entities.begin(); it != entities.end(); it++) {
    (*it)->set_enabled(enable);
  }

  return 0;
}

/**
 * @brief Returns the position of an NPC.
 *
 * - Argument 1 (string): name of the NPC
 * - Return value 1 (integer): x position
 * - Return value 2 (integer): y position
 * - Return value 3 (integer): layer
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_npc_get_position(lua_State* l) {

  Script& script = get_script(l, 1);

  const std::string& name = luaL_checkstring(l, 1);

  MapEntities &entities = script.get_map().get_entities();
  NPC* npc = (NPC*) entities.get_entity(NON_PLAYING_CHARACTER, name);

  lua_pushinteger(l, npc->get_x());
  lua_pushinteger(l, npc->get_y());
  lua_pushinteger(l, npc->get_layer());

  return 3;
}

/**
 * @brief Sets the position of an NPC.
 *
 * - Argument 1 (string): name of the NPC
 * - Argument 2 (integer): x position to set
 * - Argument 3 (integer): y position to set
 * - Optional argument 4 (integer): layer to set (if unspecified,
 * it will be unchanged)
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_npc_set_position(lua_State* l) {

  Script& script = get_script(l, 3, 4);

  const std::string& name = luaL_checkstring(l, 1);
  int x = luaL_checkinteger(l, 2);
  int y = luaL_checkinteger(l, 3);
  int layer = -1;
  if (lua_gettop(l) >= 4) {
    layer = luaL_checkinteger(l, 4);
  }

  MapEntities& entities = script.get_map().get_entities();
  NPC* npc = (NPC*) entities.get_entity(NON_PLAYING_CHARACTER, name);
  npc->set_xy(x, y);

  if (layer != -1) {
    MapEntities& entities = script.get_map().get_entities();
    entities.set_entity_layer(*npc, Layer(layer));
  }

  return 0;
}

/**
 * @brief Makes an NPC move.
 *
 * - Argument 1 (string): name of the NPC to make move
 * - Argument 2 (movement): the movement to set
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_npc_start_movement(lua_State *l) {

  Script& script = get_script(l, 2);

  const std::string& name = luaL_checkstring(l, 1);
  int movement_handle = luaL_checkinteger(l, 2);

  MapEntities& entities = script.get_map().get_entities();
  NPC* npc = (NPC*) entities.get_entity(NON_PLAYING_CHARACTER, name);
  Movement& movement = script.start_movement(movement_handle);

  npc->clear_movement();
  npc->set_movement(&movement);

  return 0;
}

/**
 * @brief Stops the movement of an NPC.
 *
 * - Argument 1 (string): name of the NPC to stop
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_npc_stop_movement(lua_State* l) {

  Script& script = get_script(l, 1);

  const std::string& name = luaL_checkstring(l, 1);

  MapEntities& entities = script.get_map().get_entities();
  NPC* npc = (NPC*) entities.get_entity(NON_PLAYING_CHARACTER, name);

  npc->clear_movement();

  return 0;
}

/**
 * @brief Makes the sprite of an NPC accessible from the script.
 *
 * - Argument 1 (string): name of the NPC
 * - Return value (sprite): the sprite of this NPC (your script can then pass it as a parameter
 * to all sol.main.sprite_* functions)
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_npc_get_sprite(lua_State* l) {

  Script& script = get_script(l, 1);

  const std::string& entity_name = luaL_checkstring(l, 1);

  MapEntities& entities = script.get_map().get_entities();
  NPC* npc = (NPC*) entities.get_entity(NON_PLAYING_CHARACTER, entity_name);

  int handle = script.create_sprite_handle(npc->get_sprite());
  lua_pushinteger(l, handle);

  return 1;
}

/**
 * @brief Removes an NPC from the map.
 *
 * - Argument 1 (string): name of the NPC
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_npc_remove(lua_State* l) {

  Script& script = get_script(l, 1);

  const std::string& name = luaL_checkstring(l, 1);

  MapEntities& entities = script.get_map().get_entities();
  entities.remove_entity(NON_PLAYING_CHARACTER, name);

  return 0;
}

/**
 * @brief Returns whether an NPC exists on the map.
 *
 * - Argument 1 (string): name of the NPC to check
 * - Return value (boolean): true if an NPC with this name exists on the map
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_npc_exists(lua_State* l) {

  Script& script = get_script(l, 1);

  const std::string& name = luaL_checkstring(l, 1);

  MapEntities& entities = script.get_map().get_entities();
  NPC* npc = (NPC*) entities.find_entity(NON_PLAYING_CHARACTER, name);

  lua_pushboolean(l, npc != NULL);

  return 1;
}

/**
 * @brief Returns whether a chest is open.
 *
 * This function returns true as soon as the hero starts opening the chest,
 * even if the treasure has not yet been brandished (there is a small delay).
 *
 * - Argument 1 (string): name of the chest
 * - Return value (boolean): true if this chest is open
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_chest_is_open(lua_State* l) {

  Script& script = get_script(l, 1);

  const std::string& name = luaL_checkstring(l, 1);

  MapEntities& entities = script.get_map().get_entities();
  Chest* chest = (Chest*) entities.get_entity(CHEST, name);
  lua_pushboolean(l, chest->is_open());

  return 1;
}

/**
 * @brief Sets a chest open or closed.
 *
 * Only the chest sprite is affected (use give_treasure to give a treasure to the player).
 * This function is useful for chests whose content is managed by the script.
 * - Argument 1 (string): name of the chest
 * - Argument 2 (boolean): true to make the chest open, false to make it closed
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_chest_set_open(lua_State *l) {

  Script& script = get_script(l, 2);

  const std::string &name = luaL_checkstring(l, 1);
  bool open = lua_toboolean(l, 2) != 0;

  MapEntities &entities = script.get_map().get_entities();
  Chest *chest = (Chest*) entities.get_entity(CHEST, name);
  chest->set_open(open);

  return 0;
}

/**
 * @brief Returns whether a chest is enabled.
 *
 * - Argument 1 (string): name of the chest
 * - Return value (boolean): true if this chest is enabled
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_chest_is_enabled(lua_State* l) {

  Script& script = get_script(l, 1);

  const std::string& name = luaL_checkstring(l, 1);

  MapEntities& entities = script.get_map().get_entities();
  MapEntity* chest = entities.get_entity(CHEST, name);
  lua_pushboolean(l, chest->is_visible() ? 1 : 0);

  return 1;
}

/**
 * @brief Enables or disables a chest.
 *
 * If the chest is already open, disabling it has not effect.
 * - Argument 1 (string): name of the chest
 * - Argument 2 (boolean): true to make the chest appear, false to make it disappear
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_chest_set_enabled(lua_State* l) {

  Script& script = get_script(l, 2);

  const std::string& name = luaL_checkstring(l, 1);
  bool enabled = lua_toboolean(l, 2) != 0;

  MapEntities& entities = script.get_map().get_entities();
  MapEntity* chest = entities.get_entity(CHEST, name);
  chest->set_visible(enabled);

  return 0;
}

/**
 * @brief Enables or disables a set of chests.
 *
 * - Argument 1 (string): prefix of the name of the chests
 * - Argument 2 (boolean): true to enable them, false to disable them
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_chest_set_group_enabled(lua_State* l) {

  Script& script = get_script(l, 2);

  const std::string& prefix = luaL_checkstring(l, 1);
  bool enable = lua_toboolean(l, 2) != 0;

  std::list<MapEntity*> entities =
      script.get_map().get_entities().get_entities_with_prefix(CHEST, prefix);

  std::list<MapEntity*>::iterator it;
  for (it = entities.begin(); it != entities.end(); it++) {
    (*it)->set_enabled(enable);
  }

  return 0;
}

/**
 * @brief Returns whether a dynamic tile is enabled.
 *
 * - Argument 1 (string): name of the dynamic tile
 * - Return value (boolean): true if this tile is enabled
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_tile_is_enabled(lua_State* l) {

  Script& script = get_script(l, 1);

  const std::string& name = luaL_checkstring(l, 1);

  MapEntities& entities = script.get_map().get_entities();
  MapEntity* dynamic_tile = entities.get_entity(DYNAMIC_TILE, name);
  lua_pushboolean(l, dynamic_tile->is_enabled() ? 1 : 0);

  return 1;
}

/**
 * @brief Enables or disables a dynamic tile.
 *
 * - Argument 1 (string): name of the dynamic tile
 * - Argument 2 (boolean): true to enable it, false to disable it
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_tile_set_enabled(lua_State* l) {

  Script& script = get_script(l, 2);

  const std::string& name = luaL_checkstring(l, 1);
  bool enable = lua_toboolean(l, 2) != 0;

  MapEntities& entities = script.get_map().get_entities();
  MapEntity* dynamic_tile = entities.get_entity(DYNAMIC_TILE, name);
  dynamic_tile->set_enabled(enable);

  return 0;
}

/**
 * @brief Enables or disables a set of dynamic tiles.
 *
 * - Argument 1 (string): prefix of the name of the dynamic tiles
 * - Argument 2 (boolean): true to enable them, false to disable them
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_tile_set_group_enabled(lua_State* l) {

  Script& script = get_script(l, 2);

  const std::string& prefix = luaL_checkstring(l, 1);
  bool enable = lua_toboolean(l, 2) != 0;

  std::list<MapEntity*> entities =
      script.get_map().get_entities().get_entities_with_prefix(DYNAMIC_TILE, prefix);

  std::list<MapEntity*>::iterator it;
  for (it = entities.begin(); it != entities.end(); it++) {
    (*it)->set_enabled(enable);
  }

  return 0;
}

/**
 * @brief Returns whether some stairs are enabled.
 *
 * - Argument 1 (string): name of the stairs
 * - Return value (boolean): true if these stairs are enabled
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_stairs_is_enabled(lua_State *l) {

  Script& script = get_script(l, 1);

  const std::string &name = luaL_checkstring(l, 1);

  MapEntities &entities = script.get_map().get_entities();
  Stairs *stairs = (Stairs*) entities.get_entity(STAIRS, name);
  lua_pushboolean(l, stairs->is_enabled() ? 1 : 0);

  return 1;
}

/**
 * @brief Enables or disables some stairs.
 *
 * All dynamic tiles whose prefix is "<stairsname>_enabled"
 * and "<stairsame>_disabled" will be updated depending on the stairs state
 * (where <stairsname> is the name of the stairs).
 * - Argument 1 (string): name of the stairs
 * - Argument 2 (boolean): true to enable them, false to disable them
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_stairs_set_enabled(lua_State *l) {

  Script& script = get_script(l, 2);

  const std::string &name = luaL_checkstring(l, 1);
  bool enable = lua_toboolean(l, 2) != 0;

  MapEntities &entities = script.get_map().get_entities();
  Stairs *stairs = (Stairs*) entities.get_entity(STAIRS, name);
  stairs->set_enabled(enable);

  return 0;
}

/**
 * @brief Enables or disables a set of stairs.
 *
 * - Argument 1 (string): prefix of the name of the stairs
 * - Argument 2 (boolean): true to enable them, false to disable them
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_stairs_set_group_enabled(lua_State* l) {

  Script& script = get_script(l, 2);

  const std::string& prefix = luaL_checkstring(l, 1);
  bool enable = lua_toboolean(l, 2) != 0;

  std::list<MapEntity*> entities =
      script.get_map().get_entities().get_entities_with_prefix(STAIRS, prefix);

  std::list<MapEntity*>::iterator it;
  for (it = entities.begin(); it != entities.end(); it++) {
    (*it)->set_enabled(enable);
  }

  return 0;
}

/**
 * @brief Returns whether a custom obstacle is enabled.
 *
 * - Argument 1 (string): name of the custom obstacle
 * - Return value (boolean): true if this custom obstacle is enabled
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_obstacle_is_enabled(lua_State *l) {

  Script& script = get_script(l, 1);

  const std::string &name = luaL_checkstring(l, 1);

  MapEntities &entities = script.get_map().get_entities();
  CustomObstacle *obstacle = (CustomObstacle*) entities.get_entity(CUSTOM_OBSTACLE, name);
  lua_pushboolean(l, obstacle->is_enabled() ? 1 : 0);

  return 1;
}

/**
 * @brief Enables or disables a custom obstacle.
 *
 * - Argument 1 (string): name of the custom obstacle
 * - Argument 2 (boolean): true to enable it, false to disable it
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_obstacle_set_enabled(lua_State *l) {

  Script& script = get_script(l, 2);

  const std::string &name = luaL_checkstring(l, 1);
  bool enable = lua_toboolean(l, 2) != 0;

  MapEntities &entities = script.get_map().get_entities();
  CustomObstacle *obstacle = (CustomObstacle*) entities.get_entity(CUSTOM_OBSTACLE, name);
  obstacle->set_enabled(enable);

  return 0;
}

/**
 * @brief Enables or disables a set of custom obstacles.
 *
 * - Argument 1 (string): prefix of the name of the custom obstacles
 * - Argument 2 (boolean): true to enable them, false to disable them
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_obstacle_set_group_enabled(lua_State* l) {

  Script& script = get_script(l, 2);

  const std::string& prefix = luaL_checkstring(l, 1);
  bool enable = lua_toboolean(l, 2) != 0;

  std::list<MapEntity*> entities =
      script.get_map().get_entities().get_entities_with_prefix(CUSTOM_OBSTACLE, prefix);

  std::list<MapEntity*>::iterator it;
  for (it = entities.begin(); it != entities.end(); it++) {
    (*it)->set_enabled(enable);
  }

  return 0;
}

/**
 * @brief Returns whether a sensor is enabled.
 *
 * - Argument 1 (string): name of the sensor
 * - Return value (boolean): true if this sensor is enabled
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_sensor_is_enabled(lua_State *l) {

  Script& script = get_script(l, 1);

  const std::string &name = luaL_checkstring(l, 1);

  MapEntities &entities = script.get_map().get_entities();
  Sensor *sensor = (Sensor*) entities.get_entity(SENSOR, name);
  lua_pushboolean(l, sensor->is_enabled() ? 1 : 0);

  return 1;
}

/**
 * @brief Enables or disables a sensor.
 *
 * - Argument 1 (string): name of the sensor
 * - Argument 2 (boolean): true to enable it, false to disable it
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_sensor_set_enabled(lua_State *l) {

  Script& script = get_script(l, 2);

  const std::string &name = luaL_checkstring(l, 1);
  bool enable = lua_toboolean(l, 2) != 0;

  MapEntities &entities = script.get_map().get_entities();
  Sensor *sensor = (Sensor*) entities.get_entity(SENSOR, name);
  sensor->set_enabled(enable);

  return 0;
}

/**
 * @brief Enables or disables a set of sensors.
 *
 * - Argument 1 (string): prefix of the name of the sensors
 * - Argument 2 (boolean): true to enable them, false to disable them
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_sensor_set_group_enabled(lua_State* l) {

  Script& script = get_script(l, 2);

  const std::string& prefix = luaL_checkstring(l, 1);
  bool enable = lua_toboolean(l, 2) != 0;

  std::list<MapEntity*> entities =
      script.get_map().get_entities().get_entities_with_prefix(SENSOR, prefix);

  std::list<MapEntity*>::iterator it;
  for (it = entities.begin(); it != entities.end(); it++) {
    (*it)->set_enabled(enable);
  }

  return 0;
}

/**
 * @brief Returns whether a jumper is enabled.
 *
 * - Argument 1 (string): name of the jumper
 * - Return value (boolean): true if this jumper is enabled
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_jumper_is_enabled(lua_State* l) {

  Script& script = get_script(l, 1);

  const std::string& name = luaL_checkstring(l, 1);

  MapEntities& entities = script.get_map().get_entities();
  MapEntity* jumper = entities.get_entity(JUMPER, name);
  lua_pushboolean(l, jumper->is_enabled() ? 1 : 0);

  return 1;
}

/**
 * @brief Enables or disables a jumper.
 *
 * - Argument 1 (string): name of the jumper
 * - Argument 2 (boolean): true to enable it, false to disable it
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_jumper_set_enabled(lua_State* l) {

  Script& script = get_script(l, 2);

  const std::string& name = luaL_checkstring(l, 1);
  bool enable = lua_toboolean(l, 2) != 0;

  MapEntities& entities = script.get_map().get_entities();
  MapEntity* jumper = entities.get_entity(JUMPER, name);
  jumper->set_enabled(enable);

  return 0;
}

/**
 * @brief Enables or disables a set of jumpers.
 *
 * - Argument 1 (string): prefix of the name of the jumpers
 * - Argument 2 (boolean): true to enable them, false to disable them
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_jumper_set_group_enabled(lua_State* l) {

  Script& script = get_script(l, 2);

  const std::string& prefix = luaL_checkstring(l, 1);
  bool enable = lua_toboolean(l, 2) != 0;

  std::list<MapEntity*> entities =
      script.get_map().get_entities().get_entities_with_prefix(JUMPER, prefix);

  std::list<MapEntity*>::iterator it;
  for (it = entities.begin(); it != entities.end(); it++) {
    (*it)->set_enabled(enable);
  }

  return 0;
}

/**
 * @brief Returns whether a crystal is enabled.
 *
 * - Argument 1 (string): name of the crystal
 * - Return value (boolean): true if this crystal are enabled
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_crystal_is_enabled(lua_State* l) {

  Script& script = get_script(l, 1);

  const std::string& name = luaL_checkstring(l, 1);

  MapEntities& entities = script.get_map().get_entities();
  MapEntity* crystal = entities.get_entity(CRYSTAL, name);
  lua_pushboolean(l, crystal->is_enabled() ? 1 : 0);

  return 1;
}

/**
 * @brief Enables or disables a crystal.
 * @param l the Lua context that is calling this function
 */
int Script::map_api_crystal_set_enabled(lua_State* l) {

  Script& script = get_script(l, 2);

  const std::string& name = luaL_checkstring(l, 1);
  bool enable = lua_toboolean(l, 2) != 0;

  MapEntities& entities = script.get_map().get_entities();
  MapEntity* crystal = entities.get_entity(CRYSTAL, name);
  crystal->set_enabled(enable);

  return 0;
}

/**
 * @brief Enables or disables a set of crystals.
 *
 * - Argument 1 (string): prefix of the name of the crystals
 * - Argument 2 (boolean): true to enable them, false to disable them
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_crystal_set_group_enabled(lua_State* l) {

  Script& script = get_script(l, 2);

  const std::string& prefix = luaL_checkstring(l, 1);
  bool enable = lua_toboolean(l, 2) != 0;

  std::list<MapEntity*> entities =
      script.get_map().get_entities().get_entities_with_prefix(CRYSTAL, prefix);

  std::list<MapEntity*>::iterator it;
  for (it = entities.begin(); it != entities.end(); it++) {
    (*it)->set_enabled(enable);
  }

  return 0;
}

/**
 * @brief Returns the current state of crystals in the map.
 *
 * - Return value (boolean): true if the state of crystals is modified,
 * false if it is the normal (initial) state
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_crystal_get_state(lua_State *l) {

  Script& script = get_script(l, 0);

  Game& game = script.get_game();
  lua_pushboolean(l, game.get_crystal_state());

  return 1;
}

/**
 * @brief Sets the current state of crystals in the map.
 *
 * - Parameter 1 (boolean): true to make the state modified,
 * false to make it normal (initial)
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_crystal_set_state(lua_State *l) {

  Script& script = get_script(l, 1);
  bool state = lua_toboolean(l, 1);

  Game& game = script.get_game();
  if (game.get_crystal_state() != state) {
    game.change_crystal_state();
  }

  return 0;
}

/**
 * @brief Inverts the current state of crystals in the map.
 * @param l the Lua context that is calling this function
 */
int Script::map_api_crystal_change_state(lua_State *l) {

  Script& script = get_script(l, 0);

  Game& game = script.get_game();
  game.change_crystal_state();

  return 0;
}

/**
 * @brief Returns whether a teletransporter is enabled.
 *
 * - Argument 1 (string): name of the teletransporter
 * - Return value (boolean): true if this teletransporter is enabled
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_teletransporter_is_enabled(lua_State* l) {

  Script& script = get_script(l, 1);

  const std::string& name = luaL_checkstring(l, 1);

  MapEntities& entities = script.get_map().get_entities();
  MapEntity* teletransporter = entities.get_entity(TELETRANSPORTER, name);
  lua_pushboolean(l, teletransporter->is_enabled() ? 1 : 0);

  return 1;
}

/**
 * @brief Enables or disables a teletransporter.
 *
 * - Argument 1 (string): name of the teletransporter
 * - Argument 2 (boolean): true to enable it, false to disable it
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_teletransporter_set_enabled(lua_State* l) {

  Script& script = get_script(l, 2);

  const std::string& name = luaL_checkstring(l, 1);
  bool enable = lua_toboolean(l, 2) != 0;

  MapEntities& entities = script.get_map().get_entities();
  MapEntity* teletransporter = entities.get_entity(TELETRANSPORTER, name);
  teletransporter->set_enabled(enable);

  return 0;
}

/**
 * @brief Enables or disables a set of teletransporters.
 *
 * - Argument 1 (string): prefix of the name of the teletransporters
 * - Argument 2 (boolean): true to enable them, false to disable them
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_teletransporter_set_group_enabled(lua_State* l) {

  Script& script = get_script(l, 2);

  const std::string& prefix = luaL_checkstring(l, 1);
  bool enable = lua_toboolean(l, 2) != 0;

  std::list<MapEntity*> entities =
      script.get_map().get_entities().get_entities_with_prefix(TELETRANSPORTER, prefix);

  std::list<MapEntity*>::iterator it;
  for (it = entities.begin(); it != entities.end(); it++) {
    (*it)->set_enabled(enable);
  }

  return 0;
}

/**
 * @brief Returns whether a block is enabled.
 *
 * - Argument 1 (string): name of the block
 * - Return value (boolean): true if this block is enabled
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_block_is_enabled(lua_State* l) {

  Script& script = get_script(l, 1);

  const std::string& name = luaL_checkstring(l, 1);

  MapEntities& entities = script.get_map().get_entities();
  MapEntity* block = entities.get_entity(BLOCK, name);
  lua_pushboolean(l, block->is_enabled() ? 1 : 0);

  return 1;
}

/**
 * @brief Enables or disables a block.
 *
 * - Argument 1 (string): name of the block
 * - Argument 2 (boolean): true to enable it, false to disable it
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_block_set_enabled(lua_State* l) {

  Script& script = get_script(l, 2);

  const std::string& name = luaL_checkstring(l, 1);
  bool enable = lua_toboolean(l, 2) != 0;

  MapEntities& entities = script.get_map().get_entities();
  MapEntity* block = entities.get_entity(BLOCK, name);
  block->set_enabled(enable);

  return 0;
}

/**
 * @brief Enables or disables a set of blocks.
 *
 * - Argument 1 (string): prefix of the name of the blocks
 * - Argument 2 (boolean): true to enable them, false to disable them
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_block_set_group_enabled(lua_State* l) {

  Script& script = get_script(l, 2);

  const std::string& prefix = luaL_checkstring(l, 1);
  bool enable = lua_toboolean(l, 2) != 0;

  std::list<MapEntity*> entities =
      script.get_map().get_entities().get_entities_with_prefix(BLOCK, prefix);

  std::list<MapEntity*>::iterator it;
  for (it = entities.begin(); it != entities.end(); it++) {
    (*it)->set_enabled(enable);
  }

  return 0;
}

/**
 * @brief Replaces a block at its initial position.
 *
 * - Argument 1 (string): name of the block to reset
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_block_reset(lua_State *l) {

  Script& script = get_script(l, 1);

  const std::string& name = luaL_checkstring(l, 1);

  MapEntities& entities = script.get_map().get_entities();
  Block* block = (Block*) entities.get_entity(BLOCK, name);
  block->reset();

  return 0;
}

/**
 * @brief Replaces all blocks of the map at their initial position.
 * @param l the Lua context that is calling this function
 */
int Script::map_api_block_reset_all(lua_State *l) {

  Script& script = get_script(l, 0);

  MapEntities &entities = script.get_map().get_entities();
  std::list<MapEntity*> blocks = entities.get_entities(BLOCK);

  std::list<MapEntity*>::iterator i;
  for (i = blocks.begin(); i != blocks.end(); i++) {
    ((Block*) (*i))->reset();
  }

  return 0;
}

/**
 * @brief Returns the position of a block.
 *
 * - Argument 1 (string): name of the block
 * - Return value 1 (integer): x position
 * - Return value 2 (integer): y position
 * - Return value 3 (integer): layer
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_block_get_position(lua_State* l) {

  Script& script = get_script(l, 1);

  const std::string& name = luaL_checkstring(l, 1);

  MapEntities& entities = script.get_map().get_entities();
  Block* block = (Block*) entities.get_entity(BLOCK, name);

  lua_pushinteger(l, block->get_x());
  lua_pushinteger(l, block->get_y());
  lua_pushinteger(l, block->get_layer());

  return 3;
}

/**
 * @brief Sets the position of a block.
 *
 * - Argument 1 (string): name of the block
 * - Argument 2 (integer): x position
 * - Argument 3 (integer): y position
 * - Optional argument 4 (integer): layer to set (if unspecified,
 * it will be unchanged)
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_block_set_position(lua_State* l) {

  Script& script = get_script(l, 3, 4);

  const std::string& name = luaL_checkstring(l, 1);
  int x = luaL_checkinteger(l, 2);
  int y = luaL_checkinteger(l, 3);
  int layer = -1;
  if (lua_gettop(l) >= 4) {
    layer = luaL_checkinteger(l, 4);
  }

  MapEntities& entities = script.get_map().get_entities();
  Block* block = (Block*) entities.get_entity(BLOCK, name);
  block->set_xy(x, y);

  if (layer != -1) {
    MapEntities& entities = script.get_map().get_entities();
    entities.set_entity_layer(*block, Layer(layer));
  }
  block->check_collision_with_detectors(false);

  return 0;
}

/**
 * @brief Returns whether a shop item exists.
 *
 * A shop item does not exist anymore if it was bought or if the script called
 * sol.map.shop_item_remove() on it.
 *
 * - Argument 1 (string): name of the shop item
 * - Return value (boolean): true if a shop item with this name exists
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_shop_item_exists(lua_State* l) {

  Script& script = get_script(l, 1);

  const std::string& name = luaL_checkstring(l, 1);

  bool exists = script.get_map().get_entities().find_entity(SHOP_ITEM, name) != NULL;
  lua_pushboolean(l, exists);

  return 1;
}

/**
 * @brief Removes a shop item from the map if it exists.
 *
 * - Argument 1 (string): name of the shop item
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_shop_item_remove(lua_State *l) {

  Script& script = get_script(l, 1);

  const std::string &name = luaL_checkstring(l, 1);

  MapEntities& entities = script.get_map().get_entities();
  MapEntity* shop_item = entities.find_entity(SHOP_ITEM, name);
  if (shop_item != NULL) {
    entities.remove_entity(shop_item);
  }

  return 0;
}

/**
 * @brief Returns whether a switch is currently activated.
 *
 * - Argument 1 (string): name of the switch
 * - Return value (boolean): true if the switch is activated
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_switch_is_activated(lua_State *l) {

  Script& script = get_script(l, 1);

  const std::string &name = luaL_checkstring(l, 1);

  MapEntities &entities = script.get_map().get_entities();
  Switch *sw = (Switch*) entities.get_entity(SWITCH, name);

  lua_pushboolean(l, sw->is_activated());

  return 1;
}

/**
 * @brief Activates or inactivates a switch.
 *
 * - Argument 1 (string): name of the switch
 * - Argument 2 (boolean): true to activate the switch, false to inactivate it
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_switch_set_activated(lua_State *l) {

  Script& script = get_script(l, 2);

  const std::string &name = luaL_checkstring(l, 1);
  bool activate = lua_toboolean(l, 2) != 0;

  MapEntities &entities = script.get_map().get_entities();
  Switch *sw = (Switch*) entities.get_entity(SWITCH, name);
  sw->set_activated(activate);

  return 0;
}

/**
 * @brief Locks a switch in its current state or unlocks it.
 *
 * - Argument 1 (string): name of the switch
 * - Argument 2 (boolean): true to lock the switch, false to unlock it
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_switch_set_locked(lua_State *l) {

  Script& script = get_script(l, 2);

  const std::string &name = luaL_checkstring(l, 1);
  bool lock = lua_toboolean(l, 2) != 0;

  MapEntities &entities = script.get_map().get_entities();
  Switch *sw = (Switch*) entities.get_entity(SWITCH, name);
  sw->set_locked(lock);

  return 0;
}

/**
 * @brief Returns whether a switch is enabled (i.e. visible).
 *
 * - Argument 1 (string): name of the switch
 * - Return value (boolean): true if this switch is enabled (i.e. visible)
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_switch_is_enabled(lua_State* l) {

  Script& script = get_script(l, 1);

  const std::string& name = luaL_checkstring(l, 1);

  MapEntities& entities = script.get_map().get_entities();
  MapEntity* sw = entities.get_entity(SWITCH, name);
  lua_pushboolean(l, sw->is_enabled() ? 1 : 0);

  return 1;
}

/**
 * @brief Enables or disables a switch.
 *
 * - Argument 1 (string): name of the switch
 * - Argument 2 (boolean): true to enable it, false to disable it
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_switch_set_enabled(lua_State* l) {

  Script& script = get_script(l, 2);

  const std::string& name = luaL_checkstring(l, 1);
  bool enable = lua_toboolean(l, 2) != 0;

  MapEntities& entities = script.get_map().get_entities();
  MapEntity* sw = entities.get_entity(SWITCH, name);
  sw->set_enabled(enable);

  return 0;
}

/**
 * @brief Enables or disables a set of switches.
 *
 * - Argument 1 (string): prefix of the name of the switches
 * - Argument 2 (boolean): true to enable them, false to disable them
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_switch_set_group_enabled(lua_State* l) {

  Script& script = get_script(l, 2);

  const std::string& prefix = luaL_checkstring(l, 1);
  bool enable = lua_toboolean(l, 2) != 0;

  std::list<MapEntity*> entities =
      script.get_map().get_entities().get_entities_with_prefix(SWITCH, prefix);

  std::list<MapEntity*>::iterator it;
  for (it = entities.begin(); it != entities.end(); it++) {
    (*it)->set_enabled(enable);
  }

  return 0;
}

/**
 * @brief Opens one or several doors having a common prefix.
 *
 * The doors must be controlled by the script (not doors for keys or
 * explosions).
 * If some doors with this prefix are already open, nothing happens for them.
 * - Argument 1 (string): prefix of the name of the doors to open
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_door_open(lua_State *l) {

  Script& script = get_script(l, 1);

  const std::string& prefix = luaL_checkstring(l, 1);

  bool done = false;
  MapEntities &entities = script.get_map().get_entities();
  std::list<MapEntity*> doors = entities.get_entities_with_prefix(DOOR, prefix);
  std::list<MapEntity*>::iterator it;
  for (it = doors.begin(); it != doors.end(); it++) {
    Door* door = (Door*) (*it);
    if (!door->is_open() || door->is_changing()) {
      door->open();
      done = true;
    }
  }

  // make sure the sound is played only once even if the script calls
  // this function repeatedly while the door is still changing
  if (done) {
    Sound::play("door_open");
  }

  return 0;
}

/**
 * @brief Closes one or several doors having a common prefix.
 *
 * The doors must be controlled by the script (not doors for keys or
 * explosions).
 * If some doors with this prefix are already closed, nothing happens for them.
 * - Argument 1 (string): prefix of the name of the doors to close
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_door_close(lua_State *l) {

  Script& script = get_script(l, 1);

  const std::string& prefix = luaL_checkstring(l, 1);

  bool done = false;
  MapEntities& entities = script.get_map().get_entities();
  std::list<MapEntity*> doors = entities.get_entities_with_prefix(DOOR, prefix);
  std::list<MapEntity*>::iterator it;
  for (it = doors.begin(); it != doors.end(); it++) {
    Door* door = (Door*) (*it);
    if (door->is_open() || door->is_changing()) {
      door->close();
      done = true;
    }
  }

  // make sure the sound is played only once even if the script calls this function repeatedly while the door is still changing
  if (done) {
    Sound::play("door_closed");
  }

  return 0;
}

/**
 * @brief Returns whether a door is open.
 *
 * - Argument 1 (string): name of the door
 * - Return value (boolean): true if this door is open
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_door_is_open(lua_State *l) {

  Script& script = get_script(l, 1);
  const std::string &name = luaL_checkstring(l, 1);

  MapEntities &entities = script.get_map().get_entities();
  Door *door = (Door*) entities.get_entity(DOOR, name);
  lua_pushboolean(l, door->is_open() ? 1 : 0);

  return 1;
}

/**
 * @brief Makes one or several doors open or closed.
 *
 * - Argument 1 (string): prefix of the name of the doors to close
 * - Argument 2 (boolean): true to make them open, false to make them closed
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_door_set_open(lua_State *l) {

  Script& script = get_script(l, 2);

  const std::string &prefix = luaL_checkstring(l, 1);
  bool open = lua_toboolean(l, 2) != 0;

  MapEntities &entities = script.get_map().get_entities();
  std::list<MapEntity*> doors = entities.get_entities_with_prefix(DOOR, prefix);
  std::list<MapEntity*>::iterator it;
  for (it = doors.begin(); it != doors.end(); it++) {
    Door *door = (Door*) (*it);
    door->set_open(open);
  }

  return 0;
}

/**
 * @brief Places a new pickable item on the map.
 *
 * - Argument 1 (string): name of the item to create (according to items.dat)
 * - Argument 2 (integer): variant of the item
 * - Argument 3 (integer): savegame variable (-1: not saved)
 * - Argument 4 (integer): x
 * - Argument 5 (integer): y
 * - Argument 6 (integer): layer
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_pickable_item_create(lua_State *l) {

  Script& script = get_script(l, 6);

  const std::string &item_name = luaL_checkstring(l, 1);
  int variant = luaL_checkinteger(l, 2);
  int savegame_variable = luaL_checkinteger(l, 3);
  int x = luaL_checkinteger(l, 4);
  int y = luaL_checkinteger(l, 5);
  Layer layer = Layer(luaL_checkinteger(l, 6));

  Game &game = script.get_game();
  MapEntities &entities = script.get_map().get_entities();
  entities.add_entity(PickableItem::create(
      game, layer, x, y,
      Treasure(game, item_name, variant, savegame_variable),
      FALLING_MEDIUM, false
      ));

  return 0;
}

/**
 * @brief Places a new destructible item on the map.
 *
 * - Argument 1 (integer): subtype of destructible item ("pot", "bush",
 * "white_stone", "black_stone", "grass" or "bomb_flower
 * - Argument 2 (integer): x
 * - Argument 3 (integer): y
 * - Argument 4 (integer): layer
 * - Optional argument 5 (table): properties (possible keys are
 * treasure_item, treasure_variant, treasure_savegame_variable,
 * destruction_callback)
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_destructible_item_create(lua_State* l) {

  Script& script = get_script(l, 4, 5);

  const std::string& subtype_name = luaL_checkstring(l, 1);
  int x = luaL_checkinteger(l, 2);
  int y = luaL_checkinteger(l, 3);
  Layer layer = Layer(luaL_checkinteger(l, 4));

  // default properties
  std::string treasure_item = "_random";
  int treasure_variant = 1;
  int treasure_savegame_variable = -1;
  int destruction_callback_ref = LUA_REFNIL;

  if (lua_gettop(l) >= 5) {
    luaL_checktype(l, 5, LUA_TTABLE);

    // traverse the table, looking for properties
    lua_pushnil(l); // first key
    while (lua_next(l, 5) != 0) {

      const std::string& key = luaL_checkstring(l, -2);
      if (key == "treasure_item") {
        treasure_item = luaL_checkstring(l, -1);
      }
      else if (key == "treasure_variant") {
        treasure_variant = luaL_checkinteger(l, -1);
      }
      else if (key == "treasure_savegame_variable") {
        treasure_savegame_variable = lua_toboolean(l, -1) != 0;
      }
      else if (key == "destruction_callback") {
        // store the callback into the registry
        luaL_argcheck(l, lua_isfunction(l, -1), 5,
            "destruction_callback should be a function");
        destruction_callback_ref = luaL_ref(l, LUA_REGISTRYINDEX);
        lua_pushnil(l);
      }
      lua_pop(l, 1); // pop the value, let the key for the iteration
    }
  }

  DestructibleItem::Subtype subtype =
      DestructibleItem::get_subtype_by_name(subtype_name);
  MapEntities& entities = script.get_map().get_entities();
  DestructibleItem* destructible_item =
      new DestructibleItem(layer, x, y, subtype, Treasure(script.get_game(),
          treasure_item, treasure_variant, treasure_savegame_variable));
  destructible_item->set_destruction_callback(destruction_callback_ref);
  entities.add_entity(destructible_item);

  return 0;
}

/**
 * @brief Places a new block on the map.
 *
 * - Argument 1 (integer): x
 * - Argument 2 (integer): y
 * - Argument 3 (integer): layer
 * - Argument 4 (integer): name
 * - Optional argument 5 (table): properties (possible keys are direction,
 * sprite_name, can_be_pushed, can_be_pulled, maximum_moves)
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_block_create(lua_State* l) {

  Script& script = get_script(l, 4, 5);

  int x = luaL_checkinteger(l, 1);
  int y = luaL_checkinteger(l, 2);
  Layer layer = Layer(luaL_checkinteger(l, 3));
  const std::string& name = luaL_checkstring(l, 4);

  // default properties
  int direction = -1;
  SpriteAnimationSetId sprite_name = "entities/block";
  bool can_be_pushed = true;
  bool can_be_pulled = false;
  int maximum_moves = 1;

  if (lua_gettop(l) >= 5) {
    luaL_checktype(l, 5, LUA_TTABLE);

    // traverse the table, looking for properties
    lua_pushnil(l); // first key
    while (lua_next(l, 5) != 0) {

      const std::string& key = luaL_checkstring(l, -2);
      if (key == "direction") {
        direction = luaL_checkinteger(l, -1);
      }
      else if (key == "sprite_name") {
        sprite_name = luaL_checkstring(l, -1);
      }
      else if (key == "can_be_pushed") {
        can_be_pushed = lua_toboolean(l, -1) != 0;
      }
      else if (key == "can_be_pulled") {
        can_be_pulled = lua_toboolean(l, -1) != 0;
      }
      else if (key == "maximum_moves") {
        maximum_moves = luaL_checkinteger(l, -1);
      }
      lua_pop(l, 1); // pop the value, let the key for the iteration
    }
  }

  MapEntities& entities = script.get_map().get_entities();
  Block* block = new Block(name, layer, x, y, direction, sprite_name,
      can_be_pushed, can_be_pulled, maximum_moves);
  entities.add_entity(block);
  block->check_collision_with_detectors(false);

  return 0;
}

/**
 * @brief Places a bomb on the map.
 *
 * - Argument 1 (integer): x
 * - Argument 2 (integer): y
 * - Argument 3 (integer): layer
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_bomb_create(lua_State *l) {

  Script& script = get_script(l, 3);

  int x = luaL_checkinteger(l, 1);
  int y = luaL_checkinteger(l, 2);
  Layer layer = Layer(luaL_checkinteger(l, 3));

  MapEntities &entities = script.get_map().get_entities();
  entities.add_entity(new Bomb(layer, x, y));

  return 0;
}

/**
 * @brief Creates an explosion on the map.
 *
 * - Argument 1 (integer): x
 * - Argument 2 (integer): y
 * - Argument 3 (integer): layer
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_explosion_create(lua_State *l) {

  Script& script = get_script(l, 3);

  int x = luaL_checkinteger(l, 1);
  int y = luaL_checkinteger(l, 2);
  Layer layer = Layer(luaL_checkinteger(l, 3));

  MapEntities &entities = script.get_map().get_entities();
  entities.add_entity(new Explosion(layer, Rectangle(x, y), true));

  return 0;
}

/**
 * @brief Creates some fire on the map.
 *
 * - Argument 1 (integer): x
 * - Argument 2 (integer): y
 * - Argument 3 (integer): layer
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_fire_create(lua_State *l) {

  Script& script = get_script(l, 3);

  int x = luaL_checkinteger(l, 1);
  int y = luaL_checkinteger(l, 2);
  Layer layer = Layer(luaL_checkinteger(l, 3));

  MapEntities &entities = script.get_map().get_entities();
  entities.add_entity(new Fire(layer, Rectangle(x, y)));

  return 0;
}

/**
 * @brief Removes any arrow currently thrown from the map.
 *
 * This function can be used if you want to destroy an arrow sooner than usual.
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_arrow_remove(lua_State *l) {

  Script& script = get_script(l, 0);

  MapEntities &entities = script.get_map().get_entities();
  entities.remove_arrows();

  return 0;
}

/**
 * @brief Creates an enemy on the map.
 *
 * - Argument 1 (string): name of the enemy to create
 * - Argument 2 (string): breed of the enemy to create
 * - Argument 3 (integer): layer on the map
 * - Argument 4 (int): x x coordinate on the map
 * - Argument 5 (int): y y coordinate on the map
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_enemy_create(lua_State *l) {

  Script& script = get_script(l, 5);

  const std::string& name = luaL_checkstring(l, 1);
  const std::string& breed = luaL_checkstring(l, 2);
  int layer = luaL_checkinteger(l, 3);
  int x = luaL_checkinteger(l, 4);
  int y = luaL_checkinteger(l, 5);

  MapEntities& entities = script.get_map().get_entities();
  Treasure treasure = Treasure(script.get_game(), "_random", 1, -1);
  Enemy* enemy = (Enemy*) Enemy::create(script.get_game(), breed, Enemy::RANK_NORMAL, -1,
      name, Layer(layer), x, y, 0, treasure);
  entities.add_entity(enemy);
  enemy->restart();

  return 0;
}

/**
 * @brief Removes an enemy from the map.
 *
 * - Argument 1 (string): name of the enemy to remove
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_enemy_remove(lua_State *l) {

  Script& script = get_script(l, 1);

  const std::string& name = luaL_checkstring(l, 1);

  MapEntities& entities = script.get_map().get_entities();
  entities.remove_entity(ENEMY, name);

  return 0;
}

/**
 * @brief Removes a set of enemies from the map.
 *
 * - Argument 1 (string): prefix of the name of the enemies to remove
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_enemy_remove_group(lua_State *l) {

  Script& script = get_script(l, 1);

  const std::string& prefix = luaL_checkstring(l, 1);

  MapEntities& entities = script.get_map().get_entities();
  entities.remove_entities_with_prefix(ENEMY, prefix);

  return 0;
}

/**
 * @brief Returns whether an enemy is enabled.
 *
 * - Argument 1 (string): name of the enemy
 * - Return value (boolean): true if this enemy is enabled
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_enemy_is_enabled(lua_State* l) {

  Script& script = get_script(l, 1);

  const std::string& name = luaL_checkstring(l, 1);

  MapEntities& entities = script.get_map().get_entities();
  MapEntity* enemy = entities.get_entity(ENEMY, name);
  lua_pushboolean(l, enemy->is_enabled());

  return 1;
}

/**
 * @brief Enables or disables an enemy.
 *
 * A normal enemy is enabled by default. A boss or a miniboss is disabled by default.
 * - Argument 1 (string): name of the enemy
 * - Argument 2 (boolean): true to enable it, false to disable it
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_enemy_set_enabled(lua_State* l) {

  Script& script = get_script(l, 2);

  const std::string& name = luaL_checkstring(l, 1);
  bool enable = lua_toboolean(l, 2);

  MapEntities& entities = script.get_map().get_entities();
  MapEntity* enemy = entities.get_entity(ENEMY, name);
  enemy->set_enabled(enable);

  return 0;
}

/**
 * @brief Enables or disables a set of enemies.
 *
 * - Argument 1 (string): prefix of the name of the enemies
 * - Argument 2 (boolean): true to enable them, false to disable them
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_enemy_set_group_enabled(lua_State* l) {

  Script& script = get_script(l, 2);

  const std::string& prefix = luaL_checkstring(l, 1);
  bool enable = lua_toboolean(l, 2) != 0;

  std::list<MapEntity*> entities =
      script.get_map().get_entities().get_entities_with_prefix(ENEMY, prefix);

  std::list<MapEntity*>::iterator it;
  for (it = entities.begin(); it != entities.end(); it++) {
    (*it)->set_enabled(enable);
  }

  return 0;
}

/**
 * @brief Returns whether an enemy is dead.
 *
 * An enemy is considered as dead if it is not present on the map.
 * - Argument 1 (string): name of the enemy
 * - Return value (boolean): true if the enemy is not on the map, false if it is alive
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_enemy_is_dead(lua_State *l) {

  Script& script = get_script(l, 1);

  const std::string &name = luaL_checkstring(l, 1);

  MapEntities &entities = script.get_map().get_entities();
  Enemy *enemy = (Enemy*) entities.find_entity(ENEMY, name);

  lua_pushboolean(l, (enemy == NULL));

  return 1;
}

/**
 * @brief Returns whether a set of enemies are dead.
 *
 * An enemy is considered as dead if it is not present on the map.
 * - Argument 1 (string): prefix of the name of the enemies to check
 * - Return value (boolean): true if there is no enemy left with this prefix on the map,
 * false if at least one of them is alive
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_enemy_is_group_dead(lua_State *l) {

  Script& script = get_script(l, 1);

  const std::string &prefix = luaL_checkstring(l, 1);

  MapEntities &entities = script.get_map().get_entities();
  std::list<MapEntity*> enemies = entities.get_entities_with_prefix(ENEMY, prefix);

  lua_pushboolean(l, enemies.empty());

  return 1;
}

/**
 * @brief Returns the number of living enemies whose name starts with the specified prefix.
 *
 * - Argument 1 (string): prefix of the name of the enemies to count
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_enemy_get_group_count(lua_State *l) {

  Script& script = get_script(l, 1);

  const std::string& prefix = luaL_checkstring(l, 1);

  MapEntities& entities = script.get_map().get_entities();
  std::list<MapEntity*> enemies = entities.get_entities_with_prefix(ENEMY, prefix);

  lua_pushinteger(l, enemies.size());

  return 1;
}

/**
 * @brief Returns the position of an enemy.
 *
 * - Argument 1 (string): name of the enemy
 * - Return value 1 (integer): x position
 * - Return value 2 (integer): y position
 * - Return value 3 (integer): layer (0 to 2)
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_enemy_get_position(lua_State* l) {

  Script& script = get_script(l, 1);

  const std::string& name = luaL_checkstring(l, 1);

  MapEntities& entities = script.get_map().get_entities();
  Enemy* enemy = (Enemy*) entities.get_entity(ENEMY, name);
  const Rectangle& coordinates = enemy->get_xy();

  lua_pushinteger(l, coordinates.get_x());
  lua_pushinteger(l, coordinates.get_y());
  lua_pushinteger(l, enemy->get_layer());

  return 3;
}

/**
 * @brief Sets the position of an enemy.
 *
 * - Argument 1 (string): name of the enemy
 * - Argument 2 (integer): x position to set
 * - Argument 3 (integer): y position to set
 * - Optional argument 4 (integer): layer to set (if unspecified,
 * it will be unchanged)
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_enemy_set_position(lua_State* l) {

  Script& script = get_script(l, 3, 4);

  const std::string& name = luaL_checkstring(l, 1);
  int x = luaL_checkinteger(l, 2);
  int y = luaL_checkinteger(l, 3);
  int layer = -1;
  if (lua_gettop(l) >= 4) {
    layer = luaL_checkinteger(l, 4);
  }

  MapEntities& entities = script.get_map().get_entities();
  Enemy* enemy = (Enemy*) entities.get_entity(ENEMY, name);
  enemy->set_xy(x, y);

  if (layer != -1) {
    MapEntities& entities = script.get_map().get_entities();
    entities.set_entity_layer(*enemy, Layer(layer));
  }

  return 0;
}

/**
 * @brief Sets the treasure dropped by an enemy.
 *
 * - Argument 1 (string): name of the enemy
 * - Argument 2 (string): name of the item (possibly "_random" or "_none")
 * - Argument 3 (integer): variant of the item
 * - Argument 4 (integer): savegame variable of the treasure
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_enemy_set_treasure(lua_State *l) {

  Script& script = get_script(l, 4);

  const std::string& enemy_name = luaL_checkstring(l, 1);
  const std::string& item_name = luaL_checkstring(l, 2);
  int variant = luaL_checkinteger(l, 3);
  int savegame_variable = luaL_checkinteger(l, 4);

  MapEntities& entities = script.get_map().get_entities();
  Enemy* enemy = (Enemy*) entities.get_entity(ENEMY, enemy_name);
  Treasure treasure(enemy->get_game(), item_name, variant, savegame_variable);
  enemy->set_treasure(treasure);

  return 0;
}

/**
 * @brief Sets the treasure dropped by an enemy as empty.
 *
 * - Argument 1 (string): name of the enemy
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_enemy_set_no_treasure(lua_State *l) {

  Script& script = get_script(l, 1);

  const std::string& enemy_name = luaL_checkstring(l, 1);

  MapEntities& entities = script.get_map().get_entities();
  Enemy* enemy = (Enemy*) entities.get_entity(ENEMY, enemy_name);
  Treasure treasure(enemy->get_game(), "_none", 1, -1);
  enemy->set_treasure(treasure);

  return 0;
}

/**
 * @brief Sets the treasure dropped by an enemy as random.
 *
 * - Argument 1 (string): name of the enemy
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_enemy_set_random_treasure(lua_State *l) {

  Script& script = get_script(l, 1);

  const std::string& enemy_name = luaL_checkstring(l, 1);

  MapEntities& entities = script.get_map().get_entities();
  Enemy* enemy = (Enemy*) entities.get_entity(ENEMY, enemy_name);
  Treasure treasure(enemy->get_game(), "_random", 1, -1);
  enemy->set_treasure(treasure);

  return 0;
}

/**
 * @brief Returns the first sprite of an enemy.
 *
 * If the enemy has several sprites, the first one created is returned.
 *
 * - Argument 1 (string): name of the enemy
 * - Return value (sprite): the sprite of this enemy
 *
 * @param l the Lua context that is calling this function
 */
int Script::map_api_enemy_get_sprite(lua_State *l) {

  Script& script = get_script(l, 1);

  const std::string& enemy_name = luaL_checkstring(l, 1);

  MapEntities& entities = script.get_map().get_entities();
  Enemy* enemy = (Enemy*) entities.get_entity(ENEMY, enemy_name);

  int handle = script.create_sprite_handle(enemy->get_sprite());
  lua_pushinteger(l, handle);

  return 1;
}

