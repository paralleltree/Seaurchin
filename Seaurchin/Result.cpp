#include "Result.h"

void Result::SetAllNotes(uint32_t notes)
{
    Notes = notes ? notes : 1;
    GaugePerJusticeCritical = 60000.0 / Notes;
    ScorePerJusticeCritical = 1010000.0 / Notes;
    Reset();
}

void Result::Reset()
{
    JusticeCritical = 0;
    Justice = 0;
    Attack = 0;
    Miss = 0;
    GaugeValue = 0;
    CurrentScore = 0;
    CurrentCombo = MaxCombo = 0;
}

void Result::PerformJusticeCritical()
{
    JusticeCritical++;
    CurrentCombo++;
    MaxCombo = CurrentCombo > MaxCombo ? CurrentCombo : MaxCombo;
    GaugeValue += GaugePerJusticeCritical;
    CurrentScore += ScorePerJusticeCritical;
    GaugeValue = std::max(0.0, GaugeValue);
}

void Result::PerformJustice()
{
    Justice++;
    CurrentCombo++;
    MaxCombo = CurrentCombo > MaxCombo ? CurrentCombo : MaxCombo;
    GaugeValue += GaugePerJusticeCritical * 0.8;
    CurrentScore += ScorePerJusticeCritical / 1.01;
    GaugeValue = std::max(0.0, GaugeValue);
}

void Result::PerformAttack()
{
    Attack++;
    CurrentCombo++;
    MaxCombo = CurrentCombo > MaxCombo ? CurrentCombo : MaxCombo;
    GaugeValue += GaugePerJusticeCritical * 0.1;
    CurrentScore += ScorePerJusticeCritical / 1.01 * 0.5;
    GaugeValue = std::max(0.0, GaugeValue);
}

void Result::PerformMiss()
{
    Miss++;
    CurrentCombo = 0;
}

void Result::BoostGaugeByValue(int value)
{
    GaugeValue += value;
    GaugeValue = std::max(0.0, GaugeValue);
}

void Result::BoostGaugeJusticeCritical(double ratio)
{
    GaugeValue += GaugePerJusticeCritical * ratio;
    GaugeValue = std::max(0.0, GaugeValue);
}

void Result::BoostGaugeJustice(double ratio)
{
    GaugeValue += GaugePerJusticeCritical * ratio * 0.5;
    GaugeValue = std::max(0.0, GaugeValue);
}

void Result::BoostGaugeAttack(double ratio)
{
    GaugeValue += GaugePerJusticeCritical * ratio * 0.1;
    GaugeValue = std::max(0.0, GaugeValue);
}

void Result::BoostGaugeMiss(double ratio)
{
    GaugeValue = std::max(0.0, GaugeValue);
}

void Result::GetCurrentResult(DrawableResult *result)
{
    if (!result) return;
    result->JusticeCritical = JusticeCritical;
    result->Justice = Justice;
    result->Attack = Attack;
    result->Miss = Miss;
    result->Combo = CurrentCombo;
    result->MaxCombo = MaxCombo;
    result->Score = round(CurrentScore);

    result->FulfilledGauges = 0;
    result->CurrentGaugeRatio = 0;
    auto cg = round(GaugeValue);
    auto ng = 12000;
    auto delta = 2000;
    while (cg >= ng) {
        cg -= ng;
        result->FulfilledGauges++;
        ng += delta;
    }
    result->CurrentGaugeRatio = (double)cg / (double)ng;
}

void RegisterResultTypes(asIScriptEngine *engine)
{
    engine->RegisterObjectType(SU_IF_DRESULT, sizeof(DrawableResult), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<DrawableResult>());
    engine->RegisterObjectProperty(SU_IF_DRESULT, "uint32 JusticeCritical", asOFFSET(DrawableResult, JusticeCritical));
    engine->RegisterObjectProperty(SU_IF_DRESULT, "uint32 Justice", asOFFSET(DrawableResult, Justice));
    engine->RegisterObjectProperty(SU_IF_DRESULT, "uint32 Attack", asOFFSET(DrawableResult, Attack));
    engine->RegisterObjectProperty(SU_IF_DRESULT, "uint32 Miss", asOFFSET(DrawableResult, Miss));
    engine->RegisterObjectProperty(SU_IF_DRESULT, "uint32 Combo", asOFFSET(DrawableResult, Combo));
    engine->RegisterObjectProperty(SU_IF_DRESULT, "uint32 MaxCombo", asOFFSET(DrawableResult, MaxCombo));
    engine->RegisterObjectProperty(SU_IF_DRESULT, "uint32 Score", asOFFSET(DrawableResult, Score));
    engine->RegisterObjectProperty(SU_IF_DRESULT, "uint32 FulfilledGauges", asOFFSET(DrawableResult, FulfilledGauges));
    engine->RegisterObjectProperty(SU_IF_DRESULT, "double CurrentGaugeRatio", asOFFSET(DrawableResult, CurrentGaugeRatio));

    engine->RegisterObjectType(SU_IF_RESULT, 0, asOBJ_REF | asOBJ_NOCOUNT);
    engine->RegisterObjectMethod(SU_IF_RESULT, "void PerformJusticeCritical()", asMETHOD(Result, PerformJusticeCritical), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_RESULT, "void PerformJustice()", asMETHOD(Result, PerformJustice), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_RESULT, "void PerformAttack()", asMETHOD(Result, PerformAttack), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_RESULT, "void PerformMiss()", asMETHOD(Result, PerformMiss), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_RESULT, "void BoostGaugeByValue(int)", asMETHOD(Result, BoostGaugeByValue), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_RESULT, "void BoostGaugeJusticeCritical(double)", asMETHOD(Result, BoostGaugeJusticeCritical), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_RESULT, "void BoostGaugeJustice(double)", asMETHOD(Result, BoostGaugeJustice), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_RESULT, "void BoostGaugeAttack(double)", asMETHOD(Result, BoostGaugeAttack), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_RESULT, "void BoostGaugeMiss(double)", asMETHOD(Result, BoostGaugeMiss), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_RESULT, "void GetCurrentResult(" SU_IF_DRESULT " &out)", asMETHOD(Result, GetCurrentResult), asCALL_THISCALL);
}