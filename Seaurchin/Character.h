#pragma once

#include "AngelScriptManager.h"
#include "Result.h"

#define SU_IF_ABILITY "Ability"
#define SU_IF_NOTETYPE "NoteType"
#define SU_IF_CHARACTER_MANAGER "CharacterManager"

struct AbilityInfo final {
    std::string Name;
    std::unordered_map<std::string, boost::any> Arguments;
};

struct CharacterInfo final {
    std::string Name;
    std::string Description;
    std::string SkillName;
    boost::filesystem::path ImagePath;
    std::vector<AbilityInfo> Abilities;

    static std::shared_ptr<CharacterInfo> LoadFromToml(const boost::filesystem::path &path);
};

enum class CharacterNoteType {
    Tap = 1,
    ExTap,
    Flick,
    Air,
    HellTap,
    Hold,
    Slide,
    AirAction,
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