#pragma once

#include "AngelScriptManager.h"
#include "Result.h"


#define SU_IF_CHARACTER_METRIC "CharacterMetric"
#define SU_IF_CHARACTER_PARAM "Character"
#define SU_IF_CHARACTER_MANAGER "CharacterManager"

struct CharacterImageMetric final {
    double WholeScale;
    int FaceOrigin[2];
    int SmallRange[4];
    int FaceRange[4];

    int get_FaceOrigin(uint32_t index) { return index < 2 ? FaceOrigin[index] : 0; }
    int get_SmallRange(uint32_t index) { return index < 4 ? SmallRange[index] : 0; }
    int get_FaceRange(uint32_t index) { return index < 4 ? FaceRange[index] : 0; }
};

class CharacterParameter final {
public:
    std::string Name;
    std::string ImagePath;
    CharacterImageMetric Metric;
};

class ExecutionManager;

class CharacterManager final {
private:
    ExecutionManager *manager;
    std::vector<std::shared_ptr<CharacterParameter>> Characters;

    int Selected;

    void LoadFromToml(boost::filesystem::path file);

public:
    CharacterManager(ExecutionManager *exm);

    void LoadAllCharacters();

    void Next();
    void Previous();
    CharacterParameter* GetCharacterParameter(int relative);
    std::shared_ptr<CharacterParameter> GetCharacterParameterSafe(int relative);
};

void RegisterCharacterTypes(asIScriptEngine *engine);