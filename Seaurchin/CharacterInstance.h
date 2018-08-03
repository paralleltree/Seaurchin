#pragma once

#include "AngelScriptManager.h"
#include "Character.h"
#include "Skill.h"
#include "Result.h"

#define SU_CHAR_SMALL_WIDTH 280
#define SU_CHAR_SMALL_HEIGHT 170
#define SU_CHAR_FACE_SIZE 128

#define SU_IF_CHARACTER_INSTANCE "CharacterInstance"
#define SU_IF_JUDGE_CALLBACK "JudgeCallback"

struct AbilityFunctions {
    asIScriptFunction *OnStart;
    asIScriptFunction *OnFinish;
    asIScriptFunction *OnJusticeCritical;
    asIScriptFunction *OnJustice;
    asIScriptFunction *OnAttack;
    asIScriptFunction *OnMiss;
};

class CharacterInstance final {
private:
    int reference = 0;

    std::shared_ptr<AngelScript> ScriptInterface;

    std::shared_ptr<CharacterParameter> CharacterSource;
    std::shared_ptr<SkillParameter> SkillSource;
    CharacterImageSet *ImageSet;
    std::shared_ptr<SkillIndicators> Indicators;
    std::shared_ptr<Result> TargetResult;

    std::vector<asIScriptObject*> Abilities;
    std::vector<asITypeInfo*> AbilityTypes;
    std::vector<AbilityFunctions> AbilityEvents;
    asIScriptContext *context;
    CallbackObject *JudgeCallback;

    void LoadAbilities();
    void CreateImageSet();

    asIScriptObject* LoadAbilityObject(boost::filesystem::path filepath);

    void CallEventFunction(asIScriptObject *obj, asIScriptFunction* func) const;
    void CallEventFunction(asIScriptObject *obj, asIScriptFunction* func, AbilityNoteType type) const;
    void CallJudgeCallback(AbilityJudgeType judge, AbilityNoteType type) const;

public:
    void AddRef() { reference++; }
    void Release() { if (--reference == 0) delete this; }

    CharacterInstance(std::shared_ptr<CharacterParameter> character, std::shared_ptr<SkillParameter> skill, std::shared_ptr<AngelScript> script, std::shared_ptr<Result> result);
    ~CharacterInstance();

    void OnStart();
    void OnFinish();
    void OnJusticeCritical(AbilityNoteType type);
    void OnJustice(AbilityNoteType type);
    void OnAttack(AbilityNoteType type);
    void OnMiss(AbilityNoteType type);
    void SetCallback(asIScriptFunction *func);

    CharacterParameter* GetCharacterParameter() const;
    CharacterImageSet* GetCharacterImages() const;
    SkillParameter* GetSkillParameter() const;
    SkillIndicators* GetSkillIndicators() const;

    static std::shared_ptr<CharacterInstance> CreateInstance(std::shared_ptr<CharacterParameter> character, std::shared_ptr<SkillParameter> skill, std::shared_ptr<AngelScript> script, std::shared_ptr<Result> result);
};

void RegisterCharacterSkillTypes(asIScriptEngine *engine);