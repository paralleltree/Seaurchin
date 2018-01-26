#pragma once

#include "AngelScriptManager.h"
#include "ScriptResource.h"
#include "Character.h"
#include "Skill.h"

#define SU_CHAR_SMALL_WIDTH 280
#define SU_CHAR_SMALL_HEIGHT 170
#define SU_CHAR_FACE_SIZE 128

class CharacterInstance final {
private:
    std::shared_ptr<AngelScript> ScriptInterface;

    std::shared_ptr<CharacterParameter> CharacterSource;
    std::shared_ptr<SkillParameter> SkillSource;

    std::vector<asIScriptObject*> Abilities;
    std::vector<asITypeInfo*> AbilityTypes;
    asIScriptContext *context;

    SImage *ImageFull;
    SImage *ImageSmall;
    SImage *ImageFace;

    CharacterInstance(std::shared_ptr<CharacterParameter> character, std::shared_ptr<SkillParameter> skill, std::shared_ptr<AngelScript>);
    ~CharacterInstance();

    void MakeCharacterImages();
    void LoadAbilities();
    asIScriptObject* LoadAbilityObject(boost::filesystem::path filepath);

public:

    static std::shared_ptr<CharacterInstance> CreateInstance(std::shared_ptr<CharacterParameter> character, std::shared_ptr<SkillParameter> skill, std::shared_ptr<AngelScript> script);
};