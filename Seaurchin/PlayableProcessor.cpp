#include "ScoreProcessor.h"
#include "ExecutionManager.h"
#include "ScenePlayer.h"
#include "Misc.h"
#include "Debug.h"

using namespace std;

PlayableProcessor::PlayableProcessor(ScenePlayer * player)
{
    Player = player;
    CurrentState = Player->manager->GetControlStateSafe();
    SetJudgeWidths(0.033, 0.066, 0.084);
    SetJudgeAdjusts(0, 1, 0, 1);
}

PlayableProcessor::PlayableProcessor(ScenePlayer * player, bool autoAir) : PlayableProcessor(player)
{
    Player = player;
    CurrentState = Player->manager->GetControlStateSafe();
    SetJudgeWidths(0.033, 0.066, 0.084);
    SetJudgeAdjusts(0, 1, 0, 1);
    isAutoAir = autoAir;
}

void PlayableProcessor::SetAutoAir(bool flag)
{
    isAutoAir = flag;
}

void PlayableProcessor::SetJudgeWidths(double jc, double j, double a)
{
    judgeWidthAttack = a;
    judgeWidthJustice = j;
    judgeWidthJusticeCritical = jc;
}

void PlayableProcessor::SetJudgeAdjusts(double jas, double jms, double jaa, double jma)
{
    judgeAdjustSlider = jas;
    judgeMultiplierSlider = jms;
    judgeAdjustAirString = jaa;
    judgeMultiplierAir = jma;
}

void PlayableProcessor::Reset()
{
    Player->CurrentResult->Reset();
    data = Player->data;
    int an = 0;
    for (auto &note : data) {
        auto type = note->Type.to_ulong();
        if (type & SU_NOTE_LONG_MASK) {
            if (!note->Type.test((size_t)SusNoteType::AirAction)) an++;
            for (auto &ex : note->ExtraData)
                if (
                    ex->Type.test((size_t)SusNoteType::End)
                    || ex->Type.test((size_t)SusNoteType::Step)
                    || ex->Type.test((size_t)SusNoteType::Injection))
                    an++;
        } else if (type & SU_NOTE_SHORT_MASK) {
            an++;
        }
    }
    Player->CurrentResult->SetAllNotes(an);

    imageHoldLight = dynamic_cast<SImage*>(Player->resources["LaneHoldLight"]);
}

bool PlayableProcessor::ShouldJudge(std::shared_ptr<SusDrawableNoteData> note)
{
    double current = Player->CurrentTime - note->StartTime;;
    double extra = 0.033;
    double leastWidth = judgeWidthAttack + extra;
    if (note->Type.to_ulong() & SU_NOTE_LONG_MASK) {
        if (note->Type[(size_t)SusNoteType::AirAction]) {
            current -= judgeAdjustAirString;
            current /= judgeMultiplierAir;
        } else {
            current -= judgeAdjustSlider;
            current /= judgeMultiplierSlider;
        }
        return current >= -leastWidth && current - note->Duration <= leastWidth;
    } else if (note->Type[(size_t)SusNoteType::Air]) {
        current -= judgeAdjustAirString;
        current /= judgeMultiplierAir;
        return current >= -leastWidth && current <= leastWidth;
    } else {
        current -= judgeAdjustSlider;
        current /= judgeMultiplierSlider;
        return current >= -leastWidth && current <= leastWidth;
    }
    return false;
}

void PlayableProcessor::Update(vector<shared_ptr<SusDrawableNoteData>>& notes)
{
    bool SlideCheck = false;
    bool HoldCheck = false;
    isInHold = false;
    for (auto& note : notes) {
        ProcessScore(note);
        SlideCheck = isInSlide || SlideCheck;
        HoldCheck = isInHold || HoldCheck;
    }

    if (!wasInSlide && SlideCheck) Player->EnqueueJudgeSound(JudgeSoundType::Sliding);
    if (wasInSlide && !SlideCheck) Player->EnqueueJudgeSound(JudgeSoundType::SlidingStop);
    if (!wasInHold && HoldCheck) Player->EnqueueJudgeSound(JudgeSoundType::Holding);
    if (wasInHold && !HoldCheck) Player->EnqueueJudgeSound(JudgeSoundType::HoldingStop);

    wasInHold = HoldCheck;
    wasInSlide = SlideCheck;
}

void PlayableProcessor::MovePosition(double relative)
{
    double newTime = Player->CurrentTime + relative;
    Player->CurrentResult->Reset();

    wasInHold = isInHold = false;
    wasInSlide = isInSlide = false;
    Player->EnqueueJudgeSound(JudgeSoundType::HoldingStop);
    Player->EnqueueJudgeSound(JudgeSoundType::SlidingStop);
    Player->RemoveSlideEffect();

    // 送り: 飛ばした部分をFinishedに
    // 戻し: 入ってくる部分をUn-Finishedに
    for (auto &note : data) {
        if (note->Type.test((size_t)SusNoteType::Hold)
            || note->Type.test((size_t)SusNoteType::Slide)
            || note->Type.test((size_t)SusNoteType::AirAction)) {
            if (note->StartTime <= newTime) {
                note->OnTheFlyData.set((size_t)NoteAttribute::Finished);
            } else {
                note->OnTheFlyData.reset((size_t)NoteAttribute::Finished);
            }
            for (auto &extra : note->ExtraData) {
                if (!extra->Type.test((size_t)SusNoteType::End)
                    && !extra->Type.test((size_t)SusNoteType::Step)
                    && !extra->Type.test((size_t)SusNoteType::Injection)) continue;
                if (extra->StartTime <= newTime) {
                    extra->OnTheFlyData.set((size_t)NoteAttribute::Finished);
                } else {
                    extra->OnTheFlyData.reset((size_t)NoteAttribute::Finished);
                }
            }
        } else {
            if (note->StartTime <= newTime) {
                note->OnTheFlyData.set((size_t)NoteAttribute::Finished);
            } else {
                note->OnTheFlyData.reset((size_t)NoteAttribute::Finished);
            }
        }
    }
}

void PlayableProcessor::Draw()
{
    if (!imageHoldLight) return;
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
    for (int i = 0; i < 16; i++)
        if (CurrentState->GetCurrentState(ControllerSource::IntegratedSliders, i))
            DrawRectRotaGraph3F(
                Player->widthPerLane * i, Player->laneBufferY,
                0, 0,
                imageHoldLight->get_Width(), imageHoldLight->get_Height(),
                0, imageHoldLight->get_Height(),
                1, 2, 0,
                imageHoldLight->GetHandle(), TRUE, FALSE);
}

void PlayableProcessor::ProcessScore(shared_ptr<SusDrawableNoteData> note)
{
    if (note->OnTheFlyData.test((size_t)NoteAttribute::Finished) && note->ExtraData.size() == 0) return;
    auto state = note->Type.to_ulong();

    if (note->Type.test((size_t)SusNoteType::Hold)) {
        isInHold = CheckHoldJudgement(note);
    } else if (note->Type.test((size_t)SusNoteType::Slide)) {
        isInSlide = CheckSlideJudgement(note);
    } else if (note->Type.test((size_t)SusNoteType::AirAction)) {
        CheckAirActionJudgement(note);
    } else if (note->Type.test((size_t)SusNoteType::Air)) {
        if (!CheckAirJudgement(note)) return;
        Player->EnqueueJudgeSound(JudgeSoundType::Air);
        Player->SpawnJudgeEffect(note, JudgeType::ShortNormal);
        Player->SpawnJudgeEffect(note, JudgeType::ShortEx);
    } else if (note->Type.test((size_t)SusNoteType::Tap)) {
        if (!CheckJudgement(note)) return;
        Player->EnqueueJudgeSound(JudgeSoundType::Tap);
        Player->SpawnJudgeEffect(note, JudgeType::ShortNormal);
    } else if (note->Type.test((size_t)SusNoteType::ExTap)) {
        if (!CheckJudgement(note)) return;
        Player->EnqueueJudgeSound(JudgeSoundType::ExTap);
        Player->SpawnJudgeEffect(note, JudgeType::ShortNormal);
        Player->SpawnJudgeEffect(note, JudgeType::ShortEx);
    } else if (note->Type.test((size_t)SusNoteType::Flick)) {
        if (!CheckJudgement(note)) return;
        Player->EnqueueJudgeSound(JudgeSoundType::Flick);
        Player->SpawnJudgeEffect(note, JudgeType::ShortNormal);
    } else if (note->Type.test((size_t)SusNoteType::HellTap)) {
        if (!CheckHellJudgement(note)) return;
        Player->EnqueueJudgeSound(JudgeSoundType::Tap);
        Player->SpawnJudgeEffect(note, JudgeType::ShortNormal);
    }
}


bool PlayableProcessor::CheckJudgement(shared_ptr<SusDrawableNoteData> note)
{
    double reltime = Player->CurrentTime - note->StartTime - judgeAdjustSlider;
    reltime /= judgeMultiplierSlider;
    if (note->OnTheFlyData.test((size_t)NoteAttribute::Finished)) return false;
    if (reltime < -judgeWidthAttack) return false;
    if (reltime > judgeWidthAttack) {
        if (note->Type[(size_t)SusNoteType::ExTap]) {
            ResetCombo(note, AbilityNoteType::ExTap);
        } else if (note->Type[(size_t)SusNoteType::Flick]) {
            ResetCombo(note, AbilityNoteType::Flick);
        } else {
            ResetCombo(note, AbilityNoteType::Tap);
        }
        return false;
    }
    for (int i = note->StartLane; i < note->StartLane + note->Length; i++) {
        if (!CurrentState->GetTriggerState(ControllerSource::IntegratedSliders, i)) continue;
        if (note->Type[(size_t)SusNoteType::ExTap]) {
            IncrementComboEx(note);
        } else if (note->Type[(size_t)SusNoteType::Flick]) {
            IncrementCombo(note, reltime, AbilityNoteType::Flick);
        } else {
            IncrementCombo(note, reltime, AbilityNoteType::Tap);
        }
        return true;
    }
    return false;
}

bool PlayableProcessor::CheckHellJudgement(shared_ptr<SusDrawableNoteData> note)
{
    double reltime = Player->CurrentTime - note->StartTime - judgeAdjustSlider;
    reltime *= judgeMultiplierSlider;  // Hellだからね
    if (note->OnTheFlyData.test((size_t)NoteAttribute::Finished)) return false;
    if (reltime < -judgeWidthAttack) return false;
    if (reltime > judgeWidthAttack) {
        IncrementComboHell(note, -1);
        return false;
    }
    if (reltime >= 0 && !note->OnTheFlyData.test((size_t)NoteAttribute::HellChecking)) {
        IncrementComboHell(note, 0);
        return true;
    }

    for (int i = note->StartLane; i < note->StartLane + note->Length; i++) {
        if (!CurrentState->GetTriggerState(ControllerSource::IntegratedSliders, i)) continue;
        if (reltime >= 0) {
            IncrementComboHell(note, 1);
        } else {
            IncrementComboHell(note, 2);
        }
        return false;
    }
    return false;
}

bool PlayableProcessor::CheckAirJudgement(shared_ptr<SusDrawableNoteData> note)
{
    double reltime = Player->CurrentTime - note->StartTime - judgeAdjustAirString;
    reltime /= judgeMultiplierAir;
    if (note->OnTheFlyData.test((size_t)NoteAttribute::Finished)) return false;
    if (reltime < -judgeWidthAttack) return false;
    if (reltime > judgeWidthAttack) {
        ResetCombo(note, AbilityNoteType::Air);
        return false;
    }

    if (isAutoAir) {
        if (reltime >= 0) {
            IncrementComboAir(note, reltime, AbilityNoteType::Air);
            return true;
        }
    } else {
        bool judged = CurrentState->GetTriggerState(
            ControllerSource::IntegratedAir,
            note->Type[(size_t)SusNoteType::Up] ? (int)AirControlSource::AirUp : (int)AirControlSource::AirDown);
        if (judged || (isAutoAir && reltime >= 0)) {
            IncrementComboAir(note, reltime, AbilityNoteType::Air);
            return true;
        }
    }
    return false;
}

bool PlayableProcessor::CheckHoldJudgement(shared_ptr<SusDrawableNoteData> note)
{
    double reltime = Player->CurrentTime - note->StartTime - judgeAdjustSlider;
    if (reltime < -judgeWidthAttack) return false;
    if (reltime >= note->Duration + judgeWidthAttack) return false;
    if (note->OnTheFlyData[(size_t)NoteAttribute::Completed]) return false;

    //現在の判定位置を調べる
    int left = note->StartLane;
    int right = left + note->Length;
    // left <= i < right で判定
    bool held = false, trigger = false, release = false;
    for (int i = left; i < right; i++) {
        held |= CurrentState->GetCurrentState(ControllerSource::IntegratedSliders, i);
        trigger |= CurrentState->GetTriggerState(ControllerSource::IntegratedSliders, i);
        release |= CurrentState->GetLastState(ControllerSource::IntegratedSliders, i) && !CurrentState->GetCurrentState(ControllerSource::IntegratedSliders, i);
    }
    double judgeTime = Player->CurrentTime - note->StartTime - judgeAdjustSlider;
    judgeTime /= judgeMultiplierSlider;

    // Start判定
    if (!note->OnTheFlyData[(size_t)NoteAttribute::Finished]) {
        if (judgeTime >= judgeWidthAttack) {
            ResetCombo(note, AbilityNoteType::Hold);
        } else if (trigger) {
            IncrementCombo(note, judgeTime, AbilityNoteType::Hold);
            Player->EnqueueJudgeSound(JudgeSoundType::Tap);
            Player->SpawnJudgeEffect(note, JudgeType::ShortNormal);
        }
        return held;
    }

    // Step~End判定
    for (const auto &extra : note->ExtraData) {
        judgeTime = Player->CurrentTime - extra->StartTime - judgeAdjustSlider;
        judgeTime /= judgeMultiplierSlider;
        if (extra->OnTheFlyData[(size_t)NoteAttribute::Finished]) continue;
        if (extra->Type[(size_t)SusNoteType::Control]) continue;
        if (extra->Type[(size_t)SusNoteType::Invisible]) continue;

        if (judgeTime < -judgeWidthAttack) {
            return held;
        } else if (judgeTime >= judgeWidthAttack) {
            ResetCombo(extra, AbilityNoteType::Hold);
            return held;
        } else if (judgeTime >= 0 && held) {
            if (extra->Type[(size_t)SusNoteType::Injection]) {
                IncrementCombo(extra, judgeTime, AbilityNoteType::Hold);
            } else if (extra->Type[(size_t)SusNoteType::Step]) {
                IncrementCombo(extra, judgeTime, AbilityNoteType::Hold);
                Player->EnqueueJudgeSound(JudgeSoundType::Tap);
                Player->SpawnJudgeEffect(extra, JudgeType::SlideTap);
            } else {
                IncrementCombo(extra, judgeTime, AbilityNoteType::Hold);
                Player->EnqueueJudgeSound(JudgeSoundType::Tap);
                Player->SpawnJudgeEffect(extra, JudgeType::SlideTap);
                note->OnTheFlyData.set((size_t)NoteAttribute::Completed);
            }
        } else {
            if (extra->Type[(size_t)SusNoteType::End] && release) {
                IncrementCombo(extra, judgeTime, AbilityNoteType::Hold);
                Player->EnqueueJudgeSound(JudgeSoundType::Tap);
                Player->SpawnJudgeEffect(extra, JudgeType::SlideTap);
                note->OnTheFlyData.set((size_t)NoteAttribute::Completed);
            }
        }
        return held;
    }
    return held;
}

bool PlayableProcessor::CheckSlideJudgement(shared_ptr<SusDrawableNoteData> note)
{
    double reltime = Player->CurrentTime - note->StartTime - judgeAdjustSlider;
    if (reltime < -judgeWidthAttack) return false;
    if (reltime >= note->Duration + judgeWidthAttack) return false;
    if (note->OnTheFlyData[(size_t)NoteAttribute::Completed]) return false;

    //現在の判定位置を調べる
    auto lastStep = note;
    auto refNote = note;
    int left = 0, right = 0;
    for (const auto &extra : note->ExtraData) {
        if (extra->Type[(size_t)SusNoteType::Control]) continue;
        if (extra->Type[(size_t)SusNoteType::Injection]) continue;
        if (Player->CurrentTime <= extra->StartTime) {
            refNote = extra;
            break;
        }
        lastStep = refNote = extra;
    }
    if (lastStep == refNote) {
        // 終点より後
        left = lastStep->StartLane;
        right = left + lastStep->Length;
    } else if (reltime < 0) {
        // 始点より前
        left = note->StartLane;
        right = left + note->Length;
    } else {
        // カーブデータ存在範囲内
        auto &refcurve = Player->curveData[refNote];
        double timeInBlock = Player->CurrentTime - lastStep->StartTime;
        auto start = refcurve[0];
        auto next = refcurve[0];
        for (const auto &segment : refcurve) {
            if (get<0>(segment) >= timeInBlock) {
                next = segment;
                break;
            }
            start = next = segment;
        }
        auto center = glm::mix(get<1>(start), get<1>(next), (timeInBlock - get<0>(start)) / (get<0>(next) - get<0>(start))) * 16;
        auto width = glm::mix(lastStep->Length, refNote->Length, timeInBlock / (refNote->StartTime - lastStep->StartTime));
        left = floor(center - width / 2.0);
        right = ceil(center + width / 2.0);
        ostringstream ss;
        // ss << "Curve: " << left << ", " << right << endl;
        // WriteDebugConsole(ss.str().c_str());
    }
    // left <= i < right で判定
    bool held = false, trigger = false, release = false;
    for (int i = left; i < right; i++) {
        held |= CurrentState->GetCurrentState(ControllerSource::IntegratedSliders, i);
        trigger |= CurrentState->GetTriggerState(ControllerSource::IntegratedSliders, i);
        release |= CurrentState->GetLastState(ControllerSource::IntegratedSliders, i) && !CurrentState->GetCurrentState(ControllerSource::IntegratedSliders, i);
    }
    double judgeTime = Player->CurrentTime - note->StartTime - judgeAdjustSlider;
    judgeTime /= judgeMultiplierSlider;

    // Start判定
    if (!note->OnTheFlyData[(size_t)NoteAttribute::Finished]) {
        if (judgeTime >= judgeWidthAttack) {
            ResetCombo(note, AbilityNoteType::Slide);
        } else if (trigger) {
            IncrementCombo(note, judgeTime, AbilityNoteType::Slide);
            Player->EnqueueJudgeSound(JudgeSoundType::Tap);
            Player->SpawnJudgeEffect(note, JudgeType::ShortNormal);
        }
        return held;
    }

    // Step~End判定
    for (const auto &extra : note->ExtraData) {
        judgeTime = Player->CurrentTime - extra->StartTime - judgeAdjustSlider;
        judgeTime /= judgeMultiplierSlider;
        if (extra->OnTheFlyData[(size_t)NoteAttribute::Finished]) continue;
        if (extra->Type[(size_t)SusNoteType::Control]) continue;
        if (extra->Type[(size_t)SusNoteType::Invisible]) continue;

        if (judgeTime < -judgeWidthAttack) {
            return held;
        } else if (judgeTime >= judgeWidthAttack) {
            ResetCombo(extra, AbilityNoteType::Slide);
            return held;
        } else if (judgeTime >= 0 && held) {
            if (extra->Type[(size_t)SusNoteType::Injection]) {
                IncrementCombo(extra, judgeTime, AbilityNoteType::Slide);
            } else if (extra->Type[(size_t)SusNoteType::Step]) {
                IncrementCombo(extra, judgeTime, AbilityNoteType::Slide);
                Player->EnqueueJudgeSound(JudgeSoundType::Tap);
                Player->SpawnJudgeEffect(extra, JudgeType::SlideTap);
            } else {
                IncrementCombo(extra, judgeTime, AbilityNoteType::Slide);
                Player->EnqueueJudgeSound(JudgeSoundType::Tap);
                Player->SpawnJudgeEffect(extra, JudgeType::SlideTap);
                note->OnTheFlyData.set((size_t)NoteAttribute::Completed);
            }
        } else {
            if (extra->Type[(size_t)SusNoteType::End] && release) {
                IncrementCombo(extra, judgeTime, AbilityNoteType::Slide);
                Player->EnqueueJudgeSound(JudgeSoundType::Tap);
                Player->SpawnJudgeEffect(extra, JudgeType::SlideTap);
                note->OnTheFlyData.set((size_t)NoteAttribute::Completed);
            }
        }
        return held;
    }
    return held;
}

bool PlayableProcessor::CheckAirActionJudgement(shared_ptr<SusDrawableNoteData> note)
{
    double reltime = Player->CurrentTime - note->StartTime - judgeAdjustAirString;
    if (reltime < -judgeWidthAttack * judgeMultiplierAir) return false;
    if (reltime >= note->Duration + judgeWidthAttack * judgeMultiplierAir) return false;
    if (note->OnTheFlyData[(size_t)NoteAttribute::Completed]) return false;

    bool held = false, trigger = false;
    held = CurrentState->GetCurrentState(ControllerSource::IntegratedAir, (int)AirControlSource::AirHold) || isAutoAir;
    trigger = CurrentState->GetTriggerState(ControllerSource::IntegratedAir, (int)AirControlSource::AirAction);

    double judgeTime = Player->CurrentTime - note->StartTime - judgeAdjustAirString;
    judgeTime /= judgeMultiplierAir;

    // Start判定
    // なし

    // Step~End判定
    for (const auto &extra : note->ExtraData) {
        judgeTime = Player->CurrentTime - extra->StartTime - judgeAdjustAirString;
        judgeTime /= judgeMultiplierAir;

        if (extra->OnTheFlyData[(size_t)NoteAttribute::Finished]) continue;
        if (extra->Type[(size_t)SusNoteType::Control]) continue;
        if (extra->Type[(size_t)SusNoteType::Invisible]) continue;

        if (judgeTime < -judgeWidthAttack) {
            return held;
        } else if (judgeTime >= judgeWidthAttack) {
            ResetCombo(extra, AbilityNoteType::AirAction);
            return held;
        } else if (extra->Type[(size_t)SusNoteType::Injection]) {
            if (judgeTime >= 0 && held) {
                IncrementCombo(extra, judgeTime, AbilityNoteType::AirAction);
            }
        } else {
            if (trigger || (isAutoAir && judgeTime >= 0)) {
                IncrementCombo(extra, judgeTime, AbilityNoteType::AirAction);
                Player->EnqueueJudgeSound(JudgeSoundType::AirAction);
                Player->SpawnJudgeEffect(extra, JudgeType::Action);
                if (extra->Type[(size_t)SusNoteType::End]) note->OnTheFlyData.set((size_t)NoteAttribute::Completed);
            }
        }
        return held;
    }
    return held;
}

void PlayableProcessor::IncrementCombo(shared_ptr<SusDrawableNoteData> note, double reltime, AbilityNoteType type)
{
    reltime = fabs(reltime);
    if (reltime <= judgeWidthJusticeCritical) {
        note->OnTheFlyData.set((size_t)NoteAttribute::Finished);
        Player->CurrentResult->PerformJusticeCritical();
        Player->CurrentCharacterInstance->OnJusticeCritical(type);
    } else if (reltime <= judgeWidthJustice) {
        note->OnTheFlyData.set((size_t)NoteAttribute::Finished);
        Player->CurrentResult->PerformJustice();
        Player->CurrentCharacterInstance->OnJustice(type);
    } else {
        note->OnTheFlyData.set((size_t)NoteAttribute::Finished);
        Player->CurrentResult->PerformAttack();
        Player->CurrentCharacterInstance->OnAttack(type);
    }
}

void PlayableProcessor::IncrementComboEx(std::shared_ptr<SusDrawableNoteData> note)
{
    note->OnTheFlyData.set((size_t)NoteAttribute::Finished);
    Player->CurrentResult->PerformJusticeCritical();
    Player->CurrentCharacterInstance->OnJusticeCritical(AbilityNoteType::ExTap);
}

void PlayableProcessor::IncrementComboHell(std::shared_ptr<SusDrawableNoteData> note, int state)
{
    switch (state) {
        case -1:
            // 無事判定終了、もう心配ない
            note->OnTheFlyData.reset((size_t)NoteAttribute::HellChecking);
            note->OnTheFlyData.set((size_t)NoteAttribute::Finished);
            break;
        case 0:
            // とりあえず通過したのでJC
            note->OnTheFlyData.set((size_t)NoteAttribute::HellChecking);
            Player->CurrentResult->PerformJusticeCritical();
            Player->CurrentCharacterInstance->OnJusticeCritical(AbilityNoteType::HellTap);
            break;
        case 1:
            // 普通に判定失敗
            Player->CurrentResult->PerformMiss();
            Player->CurrentCharacterInstance->OnMiss(AbilityNoteType::HellTap);
            note->OnTheFlyData.set((size_t)NoteAttribute::Finished);
            break;
        case 2:
            // 後から入って判定失敗
            // TODO: JCの数減らせ
            Player->CurrentResult->PerformMiss();
            Player->CurrentCharacterInstance->OnMiss(AbilityNoteType::HellTap);
            note->OnTheFlyData.set((size_t)NoteAttribute::Finished);
    }
}

void PlayableProcessor::IncrementComboAir(std::shared_ptr<SusDrawableNoteData> note, double reltime, AbilityNoteType type)
{
    reltime = fabs(reltime);
    if (reltime <= judgeWidthJusticeCritical) {
        note->OnTheFlyData.set((size_t)NoteAttribute::Finished);
        Player->CurrentResult->PerformJusticeCritical();
        Player->CurrentCharacterInstance->OnJusticeCritical(type);
    } else if (reltime <= judgeWidthJustice) {
        note->OnTheFlyData.set((size_t)NoteAttribute::Finished);
        Player->CurrentResult->PerformJustice();
        Player->CurrentCharacterInstance->OnJustice(type);
    } else {
        note->OnTheFlyData.set((size_t)NoteAttribute::Finished);
        Player->CurrentResult->PerformAttack();
        Player->CurrentCharacterInstance->OnAttack(type);
    }
}

void PlayableProcessor::ResetCombo(shared_ptr<SusDrawableNoteData> note, AbilityNoteType type)
{
    note->OnTheFlyData.set((size_t)NoteAttribute::Finished);
    Player->CurrentResult->PerformMiss();
    Player->CurrentCharacterInstance->OnMiss(type);
}