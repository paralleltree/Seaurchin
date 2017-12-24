#pragma once

#define SU_IF_DRESULT "DrawableResult"
#define SU_IF_RESULT "Result"

class Result final {
private:
    uint32_t Notes;
    uint32_t JusticeCritical;
    uint32_t Justice;
    uint32_t Attack;
    uint32_t Miss;
    uint32_t CurrentCombo;
    uint32_t MaxCombo;

    double GaugeValue;
    double GaugePerJusticeCritical;

    double CurrentScore;
    double ScorePerJusticeCritical;

public:
    Result(uint32_t notes);
    void Reset();
    void PerformJusticeCritical();
    void PerformJustice();
    void PerformAttack();
    void PerformMiss();
    void GetCurrentResult(DrawableResult *result);
};

struct DrawableResult {
    uint32_t JusticeCritical;
    uint32_t Justice;
    uint32_t Attack;
    uint32_t Miss;
    
    uint32_t Combo;
    uint32_t MaxCombo;

    uint32_t FulfilledGauges;
    double CurrentGaugeRatio;

    uint32_t Score;
};