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
#ifndef SOLARUS_DESTRUCTIBLE_ITEM_H
#define SOLARUS_DESTRUCTIBLE_ITEM_H

#include "Common.h"
#include "entities/Detector.h"
#include "entities/Ground.h"
#include "Treasure.h"

/**
 * @brief An entity that the hero can destroy and that may contain a pickable item.
 *
 * Some destructible items can be lifted and thrown (a pot, a stone, etc.),
 * some of them can be cut with the sword (for example some grass)
 * and others have both behaviors (for example a bush).
 * Some others can explode when they are lifted (a bomb or a bomb flower).
 * When the hero lifts an item, it is removed from the map and replaced by an instance of CarriedItem
 * that is attached to the hero.
 */
class DestructibleItem: public Detector {

  public:

    /**
     * Subtypes of destructible items.
     */
    enum Subtype {

      POT               = 0,
      DEPRECATED_1      = 1,
      BUSH              = 2,
      STONE_SMALL_WHITE = 3,
      STONE_SMALL_BLACK = 4,
      GRASS             = 5,
      BOMB_FLOWER       = 6,
      SUBTYPE_NUMBER    = 7
    };

  private:

    /**
     * This structure defines the properties of a destructible item type.
     */
    struct Features {
      std::string name;                          /**< name of this subtype of destructible item */
      SpriteAnimationSetId animation_set_id;     /**< animation set for the sprite */
      SoundId destruction_sound_id;              /**< sound played when the item is destroyed */
      bool can_be_lifted;                        /**< indicates that this item is an obstacle and can be lifted */
      bool can_be_cut;                           /**< indicates that this item can be cut with the sword */
      bool can_explode;                          /**< indicates that this item explodes after a delay */
      bool can_regenerate;                       /**< indicates that this item regenerates once lifted */
      int weight;                                /**< for liftable items: weight of the item (corresponds to the level
                                                  * of "lift" ability required) */
      int damage_on_enemies;                     /**< damage the item can cause to enemies */
      Ground special_ground;                     /**< for a non-obstacle item, indicates a special ground to display */
    };

    Subtype subtype;                             /**< the subtype of destructible item */
    Treasure treasure;                           /**< the pickable item that appears when the item is lifted or cut */

    bool is_being_cut;                           /**< indicates that the item is being cut */
    uint32_t regeneration_date;                  /**< date when the item starts regenerating */
    bool is_regenerating;                        /**< indicates that the item is currently regenerating */
    int destruction_callback_ref;                /**< Lua registry ref of a function to call when the item is destroyed */

    static const Features features[];
    static const std::string subtype_names[];

    void play_destroy_animation();
    void create_pickable_item();
    void destruction_callback();

  public:

    // creation and destruction
    DestructibleItem(Layer layer, int x, int y, Subtype subtype, const Treasure &treasure);
    ~DestructibleItem();
    static CreationFunction parse;

    EntityType get_type();
    bool is_displayed_in_y_order();

    const std::string& get_animation_set_id();
    const SoundId& get_destruction_sound_id();
    int get_damage_on_enemies();
    bool has_special_ground();
    Ground get_special_ground();
    void explode();
    bool can_explode();
    bool is_disabled();
    void set_destruction_callback(int destroy_callback_ref);

    bool is_obstacle_for(MapEntity &other);
    bool test_collision_custom(MapEntity &entity);
    void notify_collision(MapEntity &entity_overlapping, CollisionMode collision_mode);
    void notify_collision(MapEntity &other_entity, Sprite &other_sprite, Sprite &this_sprite);
    void notify_collision_with_hero(Hero &hero, CollisionMode collision_mode);
    void action_key_pressed();

    void set_suspended(bool suspended);
    void update();

    static const std::string& get_subtype_name(Subtype subtype);
    static Subtype get_subtype_by_name(const std::string& subtype_name);
};

#endif

