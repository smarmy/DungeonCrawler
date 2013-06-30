#ifndef ATTACK_H
#define ATTACK_H

#include "Character.h"
#include "Spell.h"
#include "Item.h"

int calculate_physical_damage(Character* attacker, Character* target, Item* weapon = 0);
int calculate_magical_damage(Character* attacker, Character* target, Spell* spell);

#endif
