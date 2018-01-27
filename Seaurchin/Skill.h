#pragma once

#define SU_IF_ABILITY "Ability"
#define SU_IF_SKILL "Skill"
#define SU_IF_SKILL_MANAGER "SkillManager"
#define SU_IF_NOTETYPE "NoteType"

class ExecutionManager;

class AbilityParameter final {
public:
    std::string Name;
    std::unordered_map<std::string, boost::any> Arguments;
};

class SkillParameter final {
public:
    std::string Name;
    std::string Description;
    std::vector<AbilityParameter> Abilities;
};

enum class AbilityNoteType {
    Tap = 1,
    ExTap,
    Flick,
    Air,
    HellTap,
    Hold,
    Slide,
    AirAction,
};

class SkillManager final {
private:
    ExecutionManager *manager;
    std::vector<std::shared_ptr<SkillParameter>> Skills;
    int Selected;

    void LoadFromToml(boost::filesystem::path file);

public:
    SkillManager(ExecutionManager *exm);

    void LoadAllSkills();

    void Next();
    void Previous();
    SkillParameter* GetSkillParameter(int relative);
    std::shared_ptr<SkillParameter> GetSkillParameterSafe(int relative);
};

void RegisterSkillTypes(asIScriptEngine *engine);