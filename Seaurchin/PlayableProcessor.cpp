#include "ScoreProcessor.h"
#include "ExecutionManager.h"
#include "ScenePlayer.h"

using namespace std;

PlayableProcessor::PlayableProcessor(ScenePlayer *splayer)
{
    player = splayer;
    currentState = player->manager->GetControlStateSafe();
    SetJudgeWidths(0.033, 0.066, 0.084);
    SetJudgeAdjusts(0, 1, 0, 1);
}

PlayableProcessor::PlayableProcessor(ScenePlayer *splayer, const bool autoAir) : PlayableProcessor(splayer)
{
    isAutoAir = autoAir;
}

void PlayableProcessor::SetAutoAir(const bool flag)
{
    isAutoAir = flag;
}

void PlayableProcessor::SetJudgeWidths(const double jc, const double j, const double a)
{
    judgeWidthAttack = a;
    judgeWidthJustice = j;
    judgeWidthJusticeCritical = jc;
}

void PlayableProcessor::SetJudgeAdjusts(const double jas, const double jms, const double jaa, const double jma)
{
    judgeAdjustSlider = jas;
    judgeMultiplierSlider = jms;
    judgeAdjustAirString = jaa;
    judgeMultiplierAir = jma;
}

void PlayableProcessor::Reset()
{
    player->currentResult->Reset();
    data = player->data;
    auto an = 0;
    for (auto &note : data) {
        const auto type = note->Type.to_ulong();
        if (type & SU_NOTE_LONG_MASK) {
            if (!note->Type.test(size_t(SusNoteType::AirAction))) an++;
            for (auto &ex : note->ExtraData)
                if (
                    ex->Type.test(size_t(SusNoteType::End))
                    || ex->Type.test(size_t(SusNoteType::Step))
                    || ex->Type.test(size_t(SusNoteType::Injection)))
                    an++;
        } else if (type & SU_NOTE_SHORT_MASK) {
            an++;
        }
    }
    player->currentResult->SetAllNotes(an);

    imageHoldLight = dynamic_cast<SImage*>(player->resources["LaneHoldLight"]);
}

bool PlayableProcessor::ShouldJudge(std::shared_ptr<SusDrawableNoteData> note)
{
    auto current = player->currentTime - note->StartTime;
    const auto extra = 0.033;
    const auto leastWidth = judgeWidthAttack * judgeMultiplierAir + extra;
    if (note->Type.to_ulong() & SU_NOTE_LONG_MASK) {
        if (note->Type[size_t(SusNoteType::AirAction)]) {
            current -= judgeAdjustAirString;
        } else {
            current -= judgeAdjustSlider;
        }
        return current >= -leastWidth && current <= note->Duration + leastWidth;
    }
    if (note->Type[size_t(SusNoteType::Air)]) {
        current -= judgeAdjustAirString;
        current /= judgeMultiplierAir;
        return current >= -leastWidth && current <= leastWidth;
    }
    current -= judgeAdjustSlider;
    current /= judgeMultiplierSlider;
    return current >= -leastWidth && current <= leastWidth;
}

void PlayableProcessor::Update(vector<shared_ptr<SusDrawableNoteData>>& notes)
{
    auto slideCheck = false;
    auto holdCheck = false;
    auto aaCheck = false;
    isInHold = false;
    for (auto& note : notes) {
        ProcessScore(note);
        slideCheck = isInSlide || slideCheck;
        holdCheck = isInHold || holdCheck;
        aaCheck = isInAA || aaCheck;
    }

    if (!wasInSlide && slideCheck) player->EnqueueJudgeSound(JudgeSoundType::Sliding);
    if (wasInSlide && !slideCheck) player->EnqueueJudgeSound(JudgeSoundType::SlidingStop);
    if (!wasInHold && holdCheck) player->EnqueueJudgeSound(JudgeSoundType::Holding);
    if (wasInHold && !holdCheck) player->EnqueueJudgeSound(JudgeSoundType::HoldingStop);
    if (!wasInAA && aaCheck) player->EnqueueJudgeSound(JudgeSoundType::AirHolding);
    if (wasInAA && !aaCheck) player->EnqueueJudgeSound(JudgeSoundType::AirHoldingStop);
    player->airActionShown = aaCheck;

    wasInHold = holdCheck;
    wasInSlide = slideCheck;
    wasInAA = aaCheck;
}

void PlayableProcessor::MovePosition(const double relative)
{
    const auto newTime = player->currentTime + relative;
    player->currentResult->Reset();

    wasInHold = isInHold = false;
    wasInSlide = isInSlide = false;
    wasInAA = isInAA = false;
    player->EnqueueJudgeSound(JudgeSoundType::HoldingStop);
    player->EnqueueJudgeSound(JudgeSoundType::SlidingStop);
    player->EnqueueJudgeSound(JudgeSoundType::AirHoldingStop);
    player->RemoveSlideEffect();

    // 送り: 飛ばした部分をFinishedに
    // 戻し: 入ってくる部分をUn-Finishedに
    for (auto &note : data) {
        if (note->Type.test(size_t(SusNoteType::Hold))
            || note->Type.test(size_t(SusNoteType::Slide))
            || note->Type.test(size_t(SusNoteType::AirAction))) {
            if (note->StartTime <= newTime) {
                note->OnTheFlyData.set(size_t(NoteAttribute::Finished));
            } else {
                note->OnTheFlyData.reset(size_t(NoteAttribute::Finished));
            }
            for (auto &extra : note->ExtraData) {
                if (!extra->Type.test(size_t(SusNoteType::End))
                    && !extra->Type.test(size_t(SusNoteType::Step))
                    && !extra->Type.test(size_t(SusNoteType::Injection))) continue;
                if (extra->StartTime <= newTime) {
                    extra->OnTheFlyData.set(size_t(NoteAttribute::Finished));
                } else {
                    extra->OnTheFlyData.reset(size_t(NoteAttribute::Finished));
                }
            }
        } else {
            if (note->StartTime <= newTime) {
                note->OnTheFlyData.set(size_t(NoteAttribute::Finished));
            } else {
                note->OnTheFlyData.reset(size_t(NoteAttribute::Finished));
            }
        }
    }
}

void PlayableProcessor::Draw()
{
    if (!imageHoldLight) return;
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
    for (auto i = 0; i < 16; i++)
        if (currentState->GetCurrentState(ControllerSource::IntegratedSliders, i))
            DrawRectRotaGraph3F(
                player->widthPerLane * i, player->laneBufferY,
                0, 0,
                imageHoldLight->GetWidth(), imageHoldLight->GetHeight(),
                0, imageHoldLight->GetHeight(),
                1, 2, 0,
                imageHoldLight->GetHandle(), TRUE, FALSE);
}

void PlayableProcessor::ProcessScore(const shared_ptr<SusDrawableNoteData>& note)
{
    if (note->OnTheFlyData.test(size_t(NoteAttribute::Finished)) && note->ExtraData.empty()) return;

    if (note->Type.test(size_t(SusNoteType::Hold))) {
        isInHold = CheckHoldJudgement(note);
    } else if (note->Type.test(size_t(SusNoteType::Slide))) {
        isInSlide = CheckSlideJudgement(note);
    } else if (note->Type.test(size_t(SusNoteType::AirAction))) {
        isInAA = CheckAirActionJudgement(note);
    } else if (note->Type.test(size_t(SusNoteType::Air))) {
        if (!CheckAirJudgement(note)) return;
        if (note->Type[size_t(SusNoteType::Up)]) {
            player->EnqueueJudgeSound(JudgeSoundType::Air);
        } else {
            player->EnqueueJudgeSound(JudgeSoundType::AirDown);
        }
        player->SpawnJudgeEffect(note, JudgeType::ShortNormal);
        player->SpawnJudgeEffect(note, JudgeType::ShortEx);
    } else if (note->Type.test(size_t(SusNoteType::Tap))) {
        if (!CheckJudgement(note)) return;
        player->EnqueueJudgeSound(JudgeSoundType::Tap);
        player->SpawnJudgeEffect(note, JudgeType::ShortNormal);
    } else if (note->Type.test(size_t(SusNoteType::ExTap))) {
        if (!CheckJudgement(note)) return;
        player->EnqueueJudgeSound(JudgeSoundType::ExTap);
        player->SpawnJudgeEffect(note, JudgeType::ShortNormal);
        player->SpawnJudgeEffect(note, JudgeType::ShortEx);
    } else if (note->Type.test(size_t(SusNoteType::AwesomeExTap))) {
        if (!CheckJudgement(note)) return;
        player->EnqueueJudgeSound(JudgeSoundType::ExTap);
        player->SpawnJudgeEffect(note, JudgeType::ShortNormal);
        player->SpawnJudgeEffect(note, JudgeType::ShortEx);
    } else if (note->Type.test(size_t(SusNoteType::Flick))) {
        if (!CheckJudgement(note)) return;
        player->EnqueueJudgeSound(JudgeSoundType::Flick);
        player->SpawnJudgeEffect(note, JudgeType::ShortNormal);
    } else if (note->Type.test(size_t(SusNoteType::HellTap))) {
        if (!CheckHellJudgement(note)) return;
        player->EnqueueJudgeSound(JudgeSoundType::Tap);
        player->SpawnJudgeEffect(note, JudgeType::ShortNormal);
    }
}


bool PlayableProcessor::CheckJudgement(const shared_ptr<SusDrawableNoteData>& note) const
{
    auto reltime = player->currentTime - note->StartTime - judgeAdjustSlider;
    reltime /= judgeMultiplierSlider;
    if (note->OnTheFlyData.test(size_t(NoteAttribute::Finished))) return false;
    if (reltime < -judgeWidthAttack) return false;
    if (reltime > judgeWidthAttack) {
        if (note->Type[size_t(SusNoteType::ExTap)]) {
            ResetCombo(note, AbilityNoteType::ExTap);
        } else if (note->Type[size_t(SusNoteType::Flick)]) {
            ResetCombo(note, AbilityNoteType::Flick);
        } else {
            ResetCombo(note, AbilityNoteType::Tap);
        }
        return false;
    }
    for (int i = note->StartLane; i < note->StartLane + note->Length; i++) {
        if (!currentState->GetTriggerState(ControllerSource::IntegratedSliders, i)) continue;
        if (note->Type[size_t(SusNoteType::ExTap)]) {
            IncrementComboEx(note, "");
        } else if (note->Type[size_t(SusNoteType::AwesomeExTap)]) {
            IncrementComboEx(
                note,
                note->Type[size_t(SusNoteType::Down)]
                ? "AwesomeExTapDown"
                : "AwesomeExTapUp"
            );
        } else if (note->Type[size_t(SusNoteType::Flick)]) {
            IncrementCombo(note, reltime, AbilityNoteType::Flick, "");
        } else {
            IncrementCombo(note, reltime, AbilityNoteType::Tap, "");
        }
        return true;
    }
    return false;
}

bool PlayableProcessor::CheckHellJudgement(const shared_ptr<SusDrawableNoteData>& note) const
{
    auto reltime = player->currentTime - note->StartTime - judgeAdjustSlider;
    reltime *= judgeMultiplierSlider;  // Hellだからね
    if (note->OnTheFlyData.test(size_t(NoteAttribute::Finished))) return false;
    if (reltime < -judgeWidthAttack) return false;
    if (reltime > judgeWidthAttack) {
        IncrementComboHell(note, -1, "");
        return false;
    }
    if (reltime >= 0 && !note->OnTheFlyData.test(size_t(NoteAttribute::HellChecking))) {
        IncrementComboHell(note, 0, "");
        return true;
    }

    for (int i = note->StartLane; i < note->StartLane + note->Length; i++) {
        if (!currentState->GetTriggerState(ControllerSource::IntegratedSliders, i)) continue;
        if (reltime >= 0) {
            IncrementComboHell(note, 1, "");
        } else {
            IncrementComboHell(note, 2, "");
        }
        return false;
    }
    return false;
}

bool PlayableProcessor::CheckAirJudgement(const shared_ptr<SusDrawableNoteData>& note) const
{
    auto reltime = player->currentTime - note->StartTime - judgeAdjustAirString;
    reltime /= judgeMultiplierAir;
    if (note->OnTheFlyData.test(size_t(NoteAttribute::Finished))) return false;
    if (reltime < -judgeWidthAttack) return false;
    if (reltime > judgeWidthAttack) {
        ResetCombo(note, AbilityNoteType::Air);
        return false;
    }

    if (isAutoAir) {
        if (reltime >= 0) {
            IncrementComboAir(note, reltime, AbilityNoteType::Air, "");
            return true;
        }
    } else {
        const auto judged = currentState->GetTriggerState(
            ControllerSource::IntegratedAir,
            note->Type[size_t(SusNoteType::Up)] ? int(AirControlSource::AirUp) : int(AirControlSource::AirDown));
        if (judged || (isAutoAir && reltime >= 0)) {
            IncrementComboAir(note, reltime, AbilityNoteType::Air, "");
            return true;
        }
    }
    return false;
}

bool PlayableProcessor::CheckHoldJudgement(const shared_ptr<SusDrawableNoteData>& note) const
{
    const auto reltime = player->currentTime - note->StartTime - judgeAdjustSlider;
    if (reltime < -judgeWidthAttack) return false;
    if (note->OnTheFlyData[size_t(NoteAttribute::Completed)]) return false;

    //現在の判定位置を調べる
    const int left = note->StartLane;
    const auto right = left + note->Length;
    // left <= i < right で判定
    auto held = false, trigger = false, release = false;
    for (auto i = left; i < right; i++) {
        held |= currentState->GetCurrentState(ControllerSource::IntegratedSliders, i);
        trigger |= currentState->GetTriggerState(ControllerSource::IntegratedSliders, i);
        release |= currentState->GetLastState(ControllerSource::IntegratedSliders, i) && !currentState->GetCurrentState(ControllerSource::IntegratedSliders, i);
    }
    auto judgeTime = player->currentTime - note->StartTime - judgeAdjustSlider;
    judgeTime /= judgeMultiplierSlider;

    // Start判定
    if (!note->OnTheFlyData[size_t(NoteAttribute::Finished)]) {
        if (judgeTime >= judgeWidthAttack) {
            ResetCombo(note, AbilityNoteType::Hold);
        } else if (trigger) {
            IncrementCombo(note, judgeTime, AbilityNoteType::Hold, "");
            player->EnqueueJudgeSound(JudgeSoundType::Tap);
            player->SpawnJudgeEffect(note, JudgeType::ShortNormal);
        }
        return held;
    }

    // Step~End判定
    // 手前:  無視
    // 0~Att: 判定が入ったタイミングで加算
    // Att~:  切る
    for (const auto &extra : note->ExtraData) {
        judgeTime = player->currentTime - extra->StartTime - judgeAdjustSlider;
        judgeTime /= judgeMultiplierSlider;
        if (extra->OnTheFlyData[size_t(NoteAttribute::Finished)]) continue;
        if (extra->Type[size_t(SusNoteType::Control)]) continue;
        if (extra->Type[size_t(SusNoteType::Invisible)]) continue;

        if (judgeTime < 0) {
            return held;
        }
        if (judgeTime >= judgeWidthAttack) {
            ResetCombo(extra, AbilityNoteType::Hold);
            if (extra->Type[size_t(SusNoteType::End)]) {
                note->OnTheFlyData.set(size_t(NoteAttribute::Completed));
            }
            return held;
        }
        if (held) {
            if (extra->Type[size_t(SusNoteType::Injection)]) {
                IncrementCombo(extra, judgeTime, AbilityNoteType::Hold, "");
            } else {
                IncrementCombo(extra, judgeTime, AbilityNoteType::Hold, "");
                player->EnqueueJudgeSound(JudgeSoundType::HoldStep);
                player->SpawnJudgeEffect(extra, JudgeType::SlideTap);
            }
            if (extra->Type[size_t(SusNoteType::End)]) {
                note->OnTheFlyData.set(size_t(NoteAttribute::Completed));
            }
            return true;
        }
    }
    return held;
}

bool PlayableProcessor::CheckSlideJudgement(const shared_ptr<SusDrawableNoteData>& note) const
{
    const auto reltime = player->currentTime - note->StartTime - judgeAdjustSlider;
    if (reltime < -judgeWidthAttack) return false;
    if (note->OnTheFlyData[size_t(NoteAttribute::Completed)]) return false;

    //現在の判定位置を調べる
    auto lastStep = note;
    auto refNote = note;
    int left, right;
    for (const auto &extra : note->ExtraData) {
        if (extra->Type[size_t(SusNoteType::Control)]) continue;
        if (extra->Type[size_t(SusNoteType::Injection)]) continue;
        if (player->currentTime <= extra->StartTime) {
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
        auto &refcurve = player->curveData[refNote];
        const auto timeInBlock = player->currentTime - lastStep->StartTime;
        auto start = refcurve[0];
        auto next = refcurve[0];
        for (const auto &segment : refcurve) {
            if (get<0>(segment) >= timeInBlock) {
                next = segment;
                break;
            }
            start = next = segment;
        }
        const auto center = glm::mix(get<1>(start), get<1>(next), (timeInBlock - get<0>(start)) / (get<0>(next) - get<0>(start))) * 16;
        const auto width = glm::mix(lastStep->Length, refNote->Length, timeInBlock / (refNote->StartTime - lastStep->StartTime));
        left = floor(center - width / 2.0);
        right = ceil(center + width / 2.0);
        ostringstream ss;
        // ss << "Curve: " << left << ", " << right << endl;
        // WriteDebugConsole(ss.str().c_str());
    }
    // left <= i < right で判定
    auto held = false, trigger = false, release = false;
    for (auto i = left; i < right; i++) {
        held |= currentState->GetCurrentState(ControllerSource::IntegratedSliders, i);
        trigger |= currentState->GetTriggerState(ControllerSource::IntegratedSliders, i);
        release |= currentState->GetLastState(ControllerSource::IntegratedSliders, i) && !currentState->GetCurrentState(ControllerSource::IntegratedSliders, i);
    }
    auto judgeTime = player->currentTime - note->StartTime - judgeAdjustSlider;
    judgeTime /= judgeMultiplierSlider;

    // Start判定
    if (!note->OnTheFlyData[size_t(NoteAttribute::Finished)]) {
        if (judgeTime >= judgeWidthAttack) {
            ResetCombo(note, AbilityNoteType::Slide);
        } else if (trigger) {
            IncrementCombo(note, judgeTime, AbilityNoteType::Slide, "");
            player->EnqueueJudgeSound(JudgeSoundType::SlideStep);
            player->SpawnJudgeEffect(note, JudgeType::ShortNormal);
        }
        return held;
    }

    // Step~End判定
    for (const auto &extra : note->ExtraData) {
        judgeTime = player->currentTime - extra->StartTime - judgeAdjustSlider;
        judgeTime /= judgeMultiplierSlider;
        if (extra->OnTheFlyData[size_t(NoteAttribute::Finished)]) continue;
        if (extra->Type[size_t(SusNoteType::Control)]) continue;
        if (extra->Type[size_t(SusNoteType::Invisible)]) continue;

        if (judgeTime < 0) {
            return held;
        }
        if (judgeTime >= judgeWidthAttack) {
            ResetCombo(extra, AbilityNoteType::Slide);
            if (extra->Type[size_t(SusNoteType::End)]) {
                note->OnTheFlyData.set(size_t(NoteAttribute::Completed));
            }
            return held;
        }
        if (held) {
            if (extra->Type[size_t(SusNoteType::Injection)]) {
                IncrementCombo(extra, judgeTime, AbilityNoteType::Slide, "");
            } else {
                IncrementCombo(extra, judgeTime, AbilityNoteType::Slide, "");
                player->EnqueueJudgeSound(JudgeSoundType::Tap);
                player->SpawnJudgeEffect(extra, JudgeType::SlideTap);
            }
            if (extra->Type[size_t(SusNoteType::End)]) {
                note->OnTheFlyData.set(size_t(NoteAttribute::Completed));
            }
            return true;
        }
        return held;
    }
    return held;
}

bool PlayableProcessor::CheckAirActionJudgement(const shared_ptr<SusDrawableNoteData>& note) const
{
    const auto reltime = player->currentTime - note->StartTime - judgeAdjustAirString;
    if (reltime < -judgeWidthAttack * judgeMultiplierAir) return false;
    if (note->OnTheFlyData[size_t(NoteAttribute::Completed)]) return false;

    const auto held = currentState->GetCurrentState(ControllerSource::IntegratedAir, int(AirControlSource::AirHold)) || isAutoAir;

    // Start判定
    // なし

    // Step~End判定
    for (const auto &extra : note->ExtraData) {
        auto judgeTime = player->currentTime - extra->StartTime - judgeAdjustAirString;
        judgeTime /= judgeMultiplierAir;

        if (extra->OnTheFlyData[size_t(NoteAttribute::Finished)]) continue;
        if (extra->Type[size_t(SusNoteType::Control)]) continue;
        if (extra->Type[size_t(SusNoteType::Invisible)]) continue;

        if (judgeTime < 0) {
            return held;
        }
        if (judgeTime >= judgeWidthAttack) {
            ResetCombo(extra, AbilityNoteType::AirAction);
            if (extra->Type[size_t(SusNoteType::End)]) {
                note->OnTheFlyData.set(size_t(NoteAttribute::Completed));
            }
            return held;
        }
        if (held) {
            if (extra->Type[size_t(SusNoteType::Injection)]) {
                IncrementCombo(extra, judgeTime, AbilityNoteType::AirAction, "");
            } else {
                IncrementCombo(extra, judgeTime, AbilityNoteType::AirAction, "");
                player->EnqueueJudgeSound(JudgeSoundType::AirAction);
                player->SpawnJudgeEffect(extra, JudgeType::Action);
            }
            if (extra->Type[size_t(SusNoteType::End)]) {
                note->OnTheFlyData.set(size_t(NoteAttribute::Completed));
            }
            return true;
        }
        return held;
    }
    return held;
}

void PlayableProcessor::IncrementCombo(const shared_ptr<SusDrawableNoteData>& note, double reltime, const AbilityNoteType type, const string& extra) const
{
    reltime = fabs(reltime);
    if (reltime <= judgeWidthJusticeCritical) {
        note->OnTheFlyData.set(size_t(NoteAttribute::Finished));
        player->currentResult->PerformJusticeCritical();
        player->currentCharacterInstance->OnJusticeCritical(type, extra);
    } else if (reltime <= judgeWidthJustice) {
        note->OnTheFlyData.set(size_t(NoteAttribute::Finished));
        player->currentResult->PerformJustice();
        player->currentCharacterInstance->OnJustice(type, extra);
    } else {
        note->OnTheFlyData.set(size_t(NoteAttribute::Finished));
        player->currentResult->PerformAttack();
        player->currentCharacterInstance->OnAttack(type, extra);
    }
}

void PlayableProcessor::IncrementComboEx(const shared_ptr<SusDrawableNoteData>& note, const string& extra) const
{
    note->OnTheFlyData.set(size_t(NoteAttribute::Finished));
    player->currentResult->PerformJusticeCritical();
    player->currentCharacterInstance->OnJusticeCritical(
        note->Type[size_t(SusNoteType::AwesomeExTap)]
        ? AbilityNoteType::AwesomeExTap
        : AbilityNoteType::ExTap,
        extra
    );
}

void PlayableProcessor::IncrementComboHell(const std::shared_ptr<SusDrawableNoteData>& note, const int state, const string& extra) const
{
    switch (state) {
        case -1:
            // 無事判定終了、もう心配ない
            note->OnTheFlyData.reset(size_t(NoteAttribute::HellChecking));
            note->OnTheFlyData.set(size_t(NoteAttribute::Finished));
            break;
        case 0:
            // とりあえず通過したのでJC
            note->OnTheFlyData.set(size_t(NoteAttribute::HellChecking));
            player->currentResult->PerformJusticeCritical();
            player->currentCharacterInstance->OnJusticeCritical(AbilityNoteType::HellTap, extra);
            break;
        case 1:
            // 普通に判定失敗
            player->currentResult->PerformMiss();
            player->currentCharacterInstance->OnMiss(AbilityNoteType::HellTap, extra);
            note->OnTheFlyData.set(size_t(NoteAttribute::Finished));
            break;
        case 2:
            // 後から入って判定失敗
            // TODO: JCの数減らせ
            player->currentResult->PerformMiss();
            player->currentCharacterInstance->OnMiss(AbilityNoteType::HellTap, extra);
            note->OnTheFlyData.set(size_t(NoteAttribute::Finished));
        default: break;
    }
}

void PlayableProcessor::IncrementComboAir(const std::shared_ptr<SusDrawableNoteData>& note, double reltime, const AbilityNoteType type, const string& extra) const
{
    reltime = fabs(reltime);
    if (reltime <= judgeWidthJusticeCritical) {
        note->OnTheFlyData.set(size_t(NoteAttribute::Finished));
        player->currentResult->PerformJusticeCritical();
        player->currentCharacterInstance->OnJusticeCritical(type, extra);
    } else if (reltime <= judgeWidthJustice) {
        note->OnTheFlyData.set(size_t(NoteAttribute::Finished));
        player->currentResult->PerformJustice();
        player->currentCharacterInstance->OnJustice(type, extra);
    } else {
        note->OnTheFlyData.set(size_t(NoteAttribute::Finished));
        player->currentResult->PerformAttack();
        player->currentCharacterInstance->OnAttack(type, extra);
    }
}

void PlayableProcessor::ResetCombo(const shared_ptr<SusDrawableNoteData>& note, const AbilityNoteType type) const
{
    note->OnTheFlyData.set(size_t(NoteAttribute::Finished));
    player->currentResult->PerformMiss();
    player->currentCharacterInstance->OnMiss(type, "");
}
