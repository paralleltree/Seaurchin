#pragma once

#include "SusAnalyzer.h"
#include "Controller.h"
#include "ScriptResource.h"
#include "Character.h"

enum class NoteAttribute {
    Invisible = 0,
    Finished,
    HellChecking,
    Completed,
    Activated,
};

class ScenePlayer;
class ScoreProcessor {
public:
    static std::vector<std::shared_ptr<SusDrawableNoteData>> DefaultDataValue;

    virtual void Reset() = 0;
    virtual void Update(std::vector<std::shared_ptr<SusDrawableNoteData>> &notes) = 0;
    virtual void MovePosition(double relative) = 0;
    virtual void Draw() = 0;
    virtual bool ShouldJudge(std::shared_ptr<SusDrawableNoteData> note) = 0;
};

class PlayableProcessor : public ScoreProcessor {
protected:
    ScenePlayer *Player;
    std::shared_ptr<ControlState> CurrentState;
    std::vector<std::shared_ptr<SusDrawableNoteData>> &data = DefaultDataValue;
    bool isInHold = false, isInSlide = false, wasInHold = false, wasInSlide = false;
    SImage *imageHoldLight;
    double judgeWidthAttack;
    double judgeWidthJustice;
    double judgeWidthJusticeCritical;
    double judgeAdjustSlider;
    double judgeAdjustAirString;
    double judgeMultiplierSlider;
    double judgeMultiplierAir;
    bool isAutoAir = false;

    void ProcessScore(std::shared_ptr<SusDrawableNoteData> notes);
    bool CheckJudgement(std::shared_ptr<SusDrawableNoteData> note);
    bool CheckHellJudgement(std::shared_ptr<SusDrawableNoteData> note);
    bool CheckAirJudgement(std::shared_ptr<SusDrawableNoteData> note);
    bool CheckAirActionJudgement(std::shared_ptr<SusDrawableNoteData> note);
    bool CheckHoldJudgement(std::shared_ptr<SusDrawableNoteData> note);
    bool CheckSlideJudgement(std::shared_ptr<SusDrawableNoteData> note);
    void IncrementCombo(std::shared_ptr<SusDrawableNoteData> note, double reltime, CharacterNoteType type);
    void IncrementComboEx(std::shared_ptr<SusDrawableNoteData> note);
    void IncrementComboHell(std::shared_ptr<SusDrawableNoteData> note, int state);
    void IncrementComboAir(std::shared_ptr<SusDrawableNoteData> note, double reltime, CharacterNoteType type);
    void ResetCombo(std::shared_ptr<SusDrawableNoteData> note, CharacterNoteType type);

public:
    PlayableProcessor(ScenePlayer *player);
    PlayableProcessor(ScenePlayer *player, bool autoAir);

    void SetAutoAir(bool flag);
    void SetJudgeWidths(double jc, double j, double a);
    void SetJudgeAdjusts(double jas, double jms, double jaa, double jma);
    void Reset() override;
    bool ShouldJudge(std::shared_ptr<SusDrawableNoteData> note) override;
    void Update(std::vector<std::shared_ptr<SusDrawableNoteData>> &notes) override;
    void MovePosition(double relative) override;
    void Draw() override;
};

class AutoPlayerProcessor : public ScoreProcessor {
protected:
    ScenePlayer *Player;
    std::vector<std::shared_ptr<SusDrawableNoteData>> &data = DefaultDataValue;
    bool isInHold = false, isInSlide = false, isInAA = false, wasInHold = false, wasInSlide = false, wasInAA = false;

    void ProcessScore(std::shared_ptr<SusDrawableNoteData> notes);
    void IncrementCombo(CharacterNoteType type);

public:
    AutoPlayerProcessor(ScenePlayer *player);

    void Reset() override;
    bool ShouldJudge(std::shared_ptr<SusDrawableNoteData> note) override;
    void Update(std::vector<std::shared_ptr<SusDrawableNoteData>> &notes) override;
    void MovePosition(double relative) override;
    void Draw() override;
};