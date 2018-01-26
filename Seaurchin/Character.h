#pragma once

#include "AngelScriptManager.h"
#include "Result.h"


#define SU_IF_CHARACTER_MANAGER "CharacterManager"

struct CharacterImageMetric final {
    double WholeScale;
    int FaceOrigin[2];
    int SmallRange[4];
    int FaceRange[4];
};

class CharacterParameter final {
public:
    std::string Name;
    std::string ImagePath;
    CharacterImageMetric Metric;
};



class Character final {
private:
    asIScriptObject* LoadAbility(boost::filesystem::path spath);
    void CallOnEvent(const char *decl);
    void CallOnEvent(const char *decl, CharacterNoteType type);

public:
    std::shared_ptr<CharacterInfo> Info;
    std::shared_ptr<AngelScript> ScriptInterface;
    std::shared_ptr<Result> TargetResult;
    std::vector<asIScriptObject*> Abilities;
    std::vector<asITypeInfo*> AbilityTypes;
    asIScriptContext *context;

    Character(std::shared_ptr<AngelScript> script, std::shared_ptr<Result> result, std::shared_ptr<CharacterInfo> info);
    ~Character();

    void Initialize();
    void OnStart();
    void OnJusticeCritical(CharacterNoteType type);
    void OnJustice(CharacterNoteType type);
    void OnAttack(CharacterNoteType type);
    void OnMiss(CharacterNoteType type);
    void OnFinish();

};

class ExecutionManager;

class CharacterManager final {
private:
    ExecutionManager *manager;
    std::vector<std::shared_ptr<CharacterInfo>> Characters;

    int Selected;

public:
    CharacterManager(ExecutionManager *exm);

    void Load();
    std::shared_ptr<Character> CreateCharacterInstance(std::shared_ptr<Result> result);

    void Next();
    void Previous();
    std::string GetName(int relative);
    std::string GetSkillName(int relative);
    std::string GetDescription(int relative);
    std::string GetImagePath(int relative);
};


void RegisterCharacterTypes(ExecutionManager *exm);