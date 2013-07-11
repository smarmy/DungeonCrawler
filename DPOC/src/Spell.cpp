#include <vector>
#include <stdexcept>

#include "Utility.h"
#include "Attack.h"
#include "logger.h"
#include "Message.h"
#include "Character.h"
#include "Spell.h"

#include "../dep/tinyxml2.h"

using namespace tinyxml2;

static std::vector<Spell> spells;

static SpellType spellTypeFromString(const std::string& type)
{
  if (type == "SPELL_NONE") return SPELL_NONE;
  if (type == "SPELL_DAMAGE") return SPELL_DAMAGE;
  if (type == "SPELL_HEAL") return SPELL_HEAL;
  if (type == "SPELL_BUFF") return SPELL_BUFF;
  if (type == "SPELL_REMOVE_STATUS") return SPELL_REMOVE_STATUS;
  if (type == "SPELL_CAUSE_STATUS") return SPELL_CAUSE_STATUS;
  if (type == "SPELL_CUSTOM") return SPELL_CUSTOM;

  TRACE("Unknown spellType=%s", type.c_str());

  return SPELL_NONE;
}

static Spell parse_spell_element(const XMLElement* spellElement)
{
  Spell spell;
  spell.battleOnly = true;
  spell.mpCost = 0;
  spell.name = "ERROR";
  spell.description = "";
  spell.power = 0;
  spell.spellType = SPELL_NONE;
  spell.target = TARGET_NONE;

  const XMLElement* nameElem = spellElement->FirstChildElement("name");
  const XMLElement* descElem = spellElement->FirstChildElement("description");
  const XMLElement* costElem = spellElement->FirstChildElement("cost");
  const XMLElement* targElem = spellElement->FirstChildElement("target");
  const XMLElement* battElem = spellElement->FirstChildElement("battleOnly");
  const XMLElement* powrElem = spellElement->FirstChildElement("power");
  const XMLElement* effeElem = spellElement->FirstChildElement("effect");

  if (nameElem)
    spell.name = nameElem->GetText();
  if (descElem)
    spell.description = descElem->GetText();
  if (costElem)
    spell.mpCost = fromString<int>(costElem->GetText());
  if (targElem)
    spell.target = targetFromString(targElem->GetText());
  if (battElem)
    spell.battleOnly = fromString<bool>(targElem->GetText());
  if (powrElem)
    spell.power = fromString<int>(powrElem->GetText());
  if (effeElem)
    spell.effect = effeElem->GetText();

  const XMLElement* typeElem = spellElement->FirstChildElement("spellType");
  if (typeElem)
  {
    for (const XMLElement* element = typeElem->FirstChildElement(); element; element = element->NextSiblingElement())
    {
      SpellType type = spellTypeFromString(element->GetText());
      spell.spellType |= (int)type;
    }
  }

  const XMLElement* statusElem = spellElement->FirstChildElement("statusChange");
  if (statusElem)
  {
    for (const XMLElement* element = statusElem->FirstChildElement(); element; element = element->NextSiblingElement())
    {
      std::string name = element->FindAttribute("name")->Value();
      int chance = fromString<int>(element->FindAttribute("chance")->Value());

      spell.causeStatus[name] = chance;
    }
  }

  const XMLElement* buffElem = spellElement->FirstChildElement("buff");
  if (buffElem)
  {
    for (const XMLElement* element = buffElem->FirstChildElement(); element; element = element->NextSiblingElement())
    {
      std::string name = element->FindAttribute("name")->Value();
      spell.attributeBuffs.push_back(name);
    }
  }

  return spell;
}

void load_spells()
{
  static const std::string database = "Resources/Spells.xml";

  XMLDocument doc;
  if (doc.LoadFile(database.c_str()) != 0)
  {
    TRACE("Unable to open spell database %s (%s)!", database.c_str(), doc.GetErrorStr1());

    throw std::runtime_error("Unable to open spell database " + database);
  }

  const XMLElement* root = doc.FirstChildElement("spells");

  for (const XMLElement* element = root->FirstChildElement(); element; element = element->NextSiblingElement())
  {
    Spell spell = parse_spell_element(element);

    TRACE("New spell %s loaded.", spell.name.c_str());

    spells.push_back(spell);
  }
}

const Spell* get_spell(const std::string& spell)
{
  for (auto it = spells.begin(); it != spells.end(); ++it)
  {
    if (it->name == spell)
    {
      return &(*it);
    }
  }

  TRACE("ERROR: Trying to get nonexisting spell %s", spell.c_str());

  return 0;
}

int cast_spell(const Spell* spell, Character* caster, Character* target)
{
  int damage = 0;

  if ((spell->spellType & SPELL_DAMAGE) || (spell->spellType & SPELL_HEAL))
  {
    damage = calculate_magical_damage(caster, target, spell);
  }

  if (spell->spellType & SPELL_CAUSE_STATUS)
  {
    for (auto it = spell->causeStatus.begin(); it != spell->causeStatus.end(); ++it)
    {
      int range = random_range(0, 100);
      if (range < it->second)
      {
        cause_status(target, it->first);
      }
      else
      {
        battle_message("No effect...");
      }
    }
  }

  if (spell->spellType & SPELL_REMOVE_STATUS)
  {
    for (auto it = spell->causeStatus.begin(); it != spell->causeStatus.end(); ++it)
    {
      cure_status(target, it->first);
    }
  }

  if (spell->spellType & SPELL_BUFF)
  {
    for (auto it = spell->attributeBuffs.begin(); it != spell->attributeBuffs.end(); ++it)
    {
      buff(target, *it, spell->power);
    }
  }

  target->takeDamage("hp", damage);

  caster->getAttribute("mp").current -= spell->mpCost;

  return damage;
}

bool can_cast_spell(const Spell* spell, Character* caster)
{
  return spell->mpCost <= caster->getAttribute("mp").current;
}
