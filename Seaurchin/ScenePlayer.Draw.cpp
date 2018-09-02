#include "ScenePlayer.h"
#include "ScriptSprite.h"
#include "ExecutionManager.h"
#include "Setting.h"
#include "Config.h"

using namespace std;

static const uint16_t rectVertexIndices[] = { 0, 1, 3, 3, 1, 2 };
static VERTEX3D groundVertices[] = {
    { VGet(SU_LANE_X_MIN, SU_LANE_Y_GROUND, SU_LANE_Z_MAX), VGet(0, 1, 0), GetColorU8(255, 255, 255, 255), GetColorU8(0, 0, 0, 0), 0.0f, 0.0f, 0.0f, 0.0f },
{ VGet(SU_LANE_X_MAX, SU_LANE_Y_GROUND, SU_LANE_Z_MAX), VGet(0, 1, 0), GetColorU8(255, 255, 255, 255), GetColorU8(0, 0, 0, 0), 1.0f, 0.0f, 0.0f, 0.0f },
{ VGet(SU_LANE_X_MAX, SU_LANE_Y_GROUND, SU_LANE_Z_MIN_EXT), VGet(0, 1, 0), GetColorU8(255, 255, 255, 255), GetColorU8(0, 0, 0, 0), 1.0f, 1.0f, 0.0f, 0.0f },
{ VGet(SU_LANE_X_MIN, SU_LANE_Y_GROUND, SU_LANE_Z_MIN_EXT), VGet(0, 1, 0), GetColorU8(255, 255, 255, 255), GetColorU8(0, 0, 0, 0), 0.0f, 1.0f, 0.0f, 0.0f }
};


void ScenePlayer::LoadResources()
{
    imageLaneGround = dynamic_cast<SImage*>(resources["LaneGround"]);
    imageLaneJudgeLine = dynamic_cast<SImage*>(resources["LaneJudgeLine"]);
    imageTap = dynamic_cast<SImage*>(resources["Tap"]);
    imageExTap = dynamic_cast<SImage*>(resources["ExTap"]);
    imageFlick = dynamic_cast<SImage*>(resources["Flick"]);
    imageHellTap = dynamic_cast<SImage*>(resources["HellTap"]);
    imageAir = dynamic_cast<SImage*>(resources["Air"]);
    imageAirUp = dynamic_cast<SImage*>(resources["AirUp"]);
    imageAirDown = dynamic_cast<SImage*>(resources["AirDown"]);
    imageHold = dynamic_cast<SImage*>(resources["Hold"]);
    imageHoldStrut = dynamic_cast<SImage*>(resources["HoldStrut"]);
    imageSlide = dynamic_cast<SImage*>(resources["Slide"]);
    imageSlideStrut = dynamic_cast<SImage*>(resources["SlideStrut"]);
    imageAirAction = dynamic_cast<SImage*>(resources["AirAction"]);
    animeTap = dynamic_cast<SAnimatedImage*>(resources["EffectTap"]);
    animeExTap = dynamic_cast<SAnimatedImage*>(resources["EffectExTap"]);
    animeSlideTap = dynamic_cast<SAnimatedImage*>(resources["EffectSlideTap"]);
    animeSlideLoop = dynamic_cast<SAnimatedImage*>(resources["EffectSlideLoop"]);
    animeAirAction = dynamic_cast<SAnimatedImage*>(resources["EffectAirAction"]);

    soundTap = dynamic_cast<SSound*>(resources["SoundTap"]);
    soundExTap = dynamic_cast<SSound*>(resources["SoundExTap"]);
    soundFlick = dynamic_cast<SSound*>(resources["SoundFlick"]);
    soundAir = dynamic_cast<SSound*>(resources["SoundAir"]);
    soundAirDown = dynamic_cast<SSound*>(resources["SoundAirDown"]);
    soundAirAction = dynamic_cast<SSound*>(resources["SoundAirAction"]);
    soundAirLoop = dynamic_cast<SSound*>(resources["SoundAirHoldLoop"]);
    soundSlideLoop = dynamic_cast<SSound*>(resources["SoundSlideLoop"]);
    soundHoldLoop = dynamic_cast<SSound*>(resources["SoundHoldLoop"]);
    soundSlideStep = dynamic_cast<SSound*>(resources["SoundSlideStep"]);
    soundHoldStep = dynamic_cast<SSound*>(resources["SoundHoldStep"]);
    soundMetronome = dynamic_cast<SSound*>(resources["Metronome"]);
    fontCombo = dynamic_cast<SFont*>(resources["FontCombo"]);

    auto setting = manager->GetSettingInstanceSafe();
    if (soundHoldLoop) soundHoldLoop->SetLoop(true);
    if (soundSlideLoop) soundSlideLoop->SetLoop(true);
    if (soundAirLoop) soundAirLoop->SetLoop(true);
    if (soundTap) soundTap->SetVolume(setting->ReadValue("Sound", "VolumeTap", 1.0));
    if (soundExTap) soundExTap->SetVolume(setting->ReadValue("Sound", "VolumeExTap", 1.0));
    if (soundFlick) soundFlick->SetVolume(setting->ReadValue("Sound", "VolumeFlick", 1.0));
    if (soundAir) soundAir->SetVolume(setting->ReadValue("Sound", "VolumeAir", 1.0));
    if (soundAirDown) soundAirDown->SetVolume(setting->ReadValue("Sound", "VolumeAir", 1.0));
    if (soundAirAction) soundAirAction->SetVolume(setting->ReadValue("Sound", "VolumeAirAction", 1.0));
    if (soundAirLoop) soundAirLoop->SetVolume(setting->ReadValue("Sound", "VolumeAirAction", 1.0));
    if (soundHoldLoop) soundHoldLoop->SetVolume(setting->ReadValue("Sound", "VolumeHold", 1.0));
    if (soundSlideLoop) soundSlideLoop->SetVolume(setting->ReadValue("Sound", "VolumeSlide", 1.0));
    if (soundHoldStep) soundHoldStep->SetVolume(setting->ReadValue("Sound", "VolumeHold", 1.0));
    if (soundSlideStep) soundSlideStep->SetVolume(setting->ReadValue("Sound", "VolumeSlide", 1.0));
    if (soundMetronome) {
        soundMetronome->SetVolume(setting->ReadValue("Sound", "VolumeTap", 1.0));
    } else {
        soundMetronome = soundTap;
    }

    vector<toml::Value> scv = { 0, 200, 255 };
    vector<toml::Value> aajcv = { 128, 255, 160 };
    scv = setting->ReadValue("Play", "ColorSlideLine", scv);
    aajcv = setting->ReadValue("Play", "ColorAirActionJudgeLine", aajcv);
    showSlideLine = setting->ReadValue("Play", "ShowSlideLine", true);
    showAirActionJudge = setting->ReadValue("Play", "ShowAirActionJudgeLine", true);
    slideLineColor = GetColor(scv[0].as<int>(), scv[1].as<int>(), scv[2].as<int>());
    airActionJudgeColor = GetColor(aajcv[0].as<int>(), aajcv[1].as<int>(), aajcv[2].as<int>());

    // 2^xêßå¿Ç™Ç†ÇÈÇÃÇ≈Ç±Ç±Ç≈åvéZ
    const int exty = laneBufferX * SU_LANE_ASPECT_EXT;
    auto bufferY = 2.0;
    while (exty > bufferY) bufferY *= 2;
    const float bufferV = exty / bufferY;
    for (auto i = 2; i < 4; i++) groundVertices[i].v = bufferV;
    hGroundBuffer = MakeScreen(laneBufferX, bufferY, TRUE);
    hBlank = MakeScreen(128, 128, FALSE);
    BEGIN_DRAW_TRANSACTION(hBlank);
    DrawBox(0, 0, 128, 128, GetColor(255, 255, 255), TRUE);
    FINISH_DRAW_TRANSACTION;
    // ÉXÉâÉCÉhÇÃ3Dä÷êîï`âÊÇ≈64x192Ç©ÇÁ64x256Ç…ÇµÇ»Ç¢Ç∆Ç¢ÇØÇ»Ç¢ÇÀ
    if (imageSlideStrut) {
        imageExtendedSlideStrut = MakeScreen(64, 256, TRUE);
        BEGIN_DRAW_TRANSACTION(imageExtendedSlideStrut);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
        DrawGraph(0, 0, imageSlideStrut->GetHandle(), TRUE);
        FINISH_DRAW_TRANSACTION;
    }

    fontCombo->AddRef();
    textCombo = STextSprite::Factory(fontCombo, "0000");
    textCombo->SetAlignment(STextAlign::Center, STextAlign::Center);
    const auto size = 320.0 / fontCombo->GetSize();
    ostringstream app;
    app << setprecision(5);
    app << "x:512, y:3200, " << "scaleX:" << size << ", scaleY:" << size;
    textCombo->Apply(app.str());
}

void ScenePlayer::AddSprite(SSprite *sprite)
{
    spritesPending.push_back(sprite);
}

void ScenePlayer::TickGraphics(const double delta)
{
    if (status.Combo > previousStatus.Combo) RefreshComboText();
    UpdateSlideEffect();
    laneBackgroundRoll += laneBackgroundSpeed * delta;
}

void ScenePlayer::Draw()
{
    ostringstream combo;
    combo << status.Combo;
    textCombo->SetText(combo.str());

    if (movieBackground) DrawExtendGraph(0, 0, SU_RES_WIDTH, SU_RES_HEIGHT, movieBackground, FALSE);

    BEGIN_DRAW_TRANSACTION(hGroundBuffer);
    // îwåiïî
    DrawLaneBackground();
    DrawLaneDivisionLines();
    for (auto& note : seenData) {
        auto &type = note->Type;
        if (type[size_t(SusNoteType::MeasureLine)]) DrawMeasureLine(note);
    }

    // â∫ë§ÇÃÉçÉìÉOÉmÅ[Écóﬁ
    for (auto& note : seenData) {
        auto &type = note->Type;
        if (type[size_t(SusNoteType::Hold)]) DrawHoldNotes(note);
        if (type[size_t(SusNoteType::Slide)]) DrawSlideNotes(note);
    }

    // è„ë§ÇÃÉVÉáÅ[ÉgÉmÅ[Écóﬁ
    for (auto& note : seenData) {
        auto &type = note->Type;
        if (type[size_t(SusNoteType::Tap)]) DrawShortNotes(note);
        if (type[size_t(SusNoteType::ExTap)]) DrawShortNotes(note);
        if (type[size_t(SusNoteType::AwesomeExTap)]) DrawShortNotes(note);
        if (type[size_t(SusNoteType::Flick)]) DrawShortNotes(note);
        if (type[size_t(SusNoteType::HellTap)]) DrawShortNotes(note);
        if (type[size_t(SusNoteType::Air)] && type[size_t(SusNoteType::Grounded)]) DrawShortNotes(note);
    }

    FINISH_DRAW_TRANSACTION;
    Prepare3DDrawCall();
    DrawPolygonIndexed3D(groundVertices, 4, rectVertexIndices, 2, hGroundBuffer, TRUE);
    for (auto& i : sprites) i->Draw();

    //3DånÉmÅ[Éc
    Prepare3DDrawCall();
    DrawAerialNotes(seenData);

    if (airActionShown && showAirActionJudge) {
        SetDrawBlendMode(DX_BLENDMODE_ADD, 192);
        SetUseZBuffer3D(FALSE);
        DrawTriangle3D(
            VGet(SU_LANE_X_MIN_EXT, SU_LANE_Y_AIR - 5, SU_LANE_Z_MIN - 5),
            VGet(SU_LANE_X_MIN_EXT, SU_LANE_Y_AIR + 5, SU_LANE_Z_MIN + 5),
            VGet(SU_LANE_X_MAX_EXT, SU_LANE_Y_AIR + 5, SU_LANE_Z_MIN + 5),
            airActionJudgeColor, TRUE);
        DrawTriangle3D(
            VGet(SU_LANE_X_MIN_EXT, SU_LANE_Y_AIR - 5, SU_LANE_Z_MIN - 5),
            VGet(SU_LANE_X_MAX_EXT, SU_LANE_Y_AIR + 5, SU_LANE_Z_MIN + 5),
            VGet(SU_LANE_X_MAX_EXT, SU_LANE_Y_AIR - 5, SU_LANE_Z_MIN - 5),
            airActionJudgeColor, TRUE);
    }
}

void ScenePlayer::DrawAerialNotes(const vector<shared_ptr<SusDrawableNoteData>>& notes)
{
    vector<AirDrawQuery> airdraws, covers;
    for (const auto &note : seenData) {
        if (note->Type.test(size_t(SusNoteType::AirAction))) {
            // DrawAirActionNotes(note);
            AirDrawQuery head;
            head.Type = AirDrawType::AirActionStart;
            head.Z = 1.0 - note->ModifiedPosition / seenDuration;
            head.Note = note;
            airdraws.push_back(head);
            auto prev = note;
            auto lastZ = head.Z;
            for (const auto &extra : note->ExtraData) {
                if (extra->Type.test(size_t(SusNoteType::Control))) continue;
                if (extra->Type.test(size_t(SusNoteType::Injection))) continue;
                const auto z = 1.0 - extra->ModifiedPosition / seenDuration;
                if ((z >= 0 || lastZ >= 0) && (z < cullingLimit || lastZ < cullingLimit)) {
                    AirDrawQuery tail;
                    tail.Type = AirDrawType::AirActionStep;
                    tail.Z = z;
                    tail.Note = extra;
                    tail.PreviousNote = prev;
                    airdraws.push_back(tail);
                    covers.push_back(tail);
                }
                prev = extra;
                lastZ = z;
            }
        }
        if (note->Type.test(size_t(SusNoteType::Air))) {
            const auto z = 1.0 - note->ModifiedPosition / seenDuration;
            if (z < 0 || z >= cullingLimit) continue;
            AirDrawQuery head;
            head.Type = AirDrawType::Air;
            head.Z = z;
            head.Note = note;
            airdraws.push_back(head);
        }
    }
    stable_sort(airdraws.begin(), airdraws.end(), [](const AirDrawQuery a, const AirDrawQuery b) {
        return a.Z < b.Z;
    });
    for (const auto &query : airdraws) {
        switch (query.Type) {
            case AirDrawType::Air:
                DrawAirNotes(query);
                break;
            case AirDrawType::AirActionStart:
                DrawAirActionStart(query);
                break;
            case AirDrawType::AirActionStep:
                DrawAirActionStep(query);
                break;
            default: break;
        }
    }
    for (const auto &query : covers) DrawAirActionCover(query);
    for (const auto &query : airdraws) {
        if (query.Type == AirDrawType::AirActionStep) DrawAirActionStepBox(query);
    }
}

void ScenePlayer::RefreshComboText() const
{
    const auto size = 320.0 / fontCombo->GetSize();
    ostringstream app;
    textCombo->AbortMove(true);

    app << setprecision(5);
    app << "scaleX:" << size * 1.05 << ", scaleY:" << size * 1.05;
    textCombo->Apply(app.str());
    app.str("");
    app << setprecision(5);
    app << "scale_to(" << "x:" << size << ", y:" << size << ", time: 0.2, ease:out_quad)";
    textCombo->AddMove(app.str());
}

// position ÇÕ 0 ~ 16
void ScenePlayer::SpawnJudgeEffect(const shared_ptr<SusDrawableNoteData>& target, const JudgeType type)
{
    Prepare3DDrawCall();
    const auto position = target->StartLane + target->Length / 2.0;
    const auto x = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, position / 16.0);
    switch (type) {
        case JudgeType::ShortNormal: {
            const auto spawnAt = ConvWorldPosToScreenPos(VGet(x, SU_LANE_Y_GROUND, SU_LANE_Z_MIN));
            animeTap->AddRef();
            auto sp = SAnimeSprite::Factory(animeTap);
            sp->Apply("origX:128, origY:224");
            sp->Transform.X = spawnAt.x;
            sp->Transform.Y = spawnAt.y;
            AddSprite(sp);
            break;
        }
        case JudgeType::ShortEx: {
            const auto spawnAt = ConvWorldPosToScreenPos(VGet(x, SU_LANE_Y_GROUND, SU_LANE_Z_MIN));
            animeExTap->AddRef();
            auto sp = SAnimeSprite::Factory(animeExTap);
            sp->Apply("origX:128, origY:256");
            sp->Transform.X = spawnAt.x;
            sp->Transform.Y = spawnAt.y;
            sp->Transform.ScaleX = target->Length / 6.0;
            AddSprite(sp);
            break;
        }
        case JudgeType::SlideTap: {
            const auto spawnAt = ConvWorldPosToScreenPos(VGet(x, SU_LANE_Y_GROUND, SU_LANE_Z_MIN));
            animeSlideTap->AddRef();
            auto sp = SAnimeSprite::Factory(animeSlideTap);
            sp->Apply("origX:128, origY:224");
            sp->Transform.X = spawnAt.x;
            sp->Transform.Y = spawnAt.y;
            AddSprite(sp);
            break;
        }
        case JudgeType::Action: {
            const auto spawnAt = ConvWorldPosToScreenPos(VGet(x, SU_LANE_Y_AIR, SU_LANE_Z_MIN));
            animeAirAction->AddRef();
            auto sp = SAnimeSprite::Factory(animeAirAction);
            sp->Apply("origX:128, origY:128");
            sp->Transform.X = spawnAt.x;
            sp->Transform.Y = spawnAt.y;
            AddSprite(sp);
            break;
        }
    }
}

void ScenePlayer::SpawnSlideLoopEffect(const shared_ptr<SusDrawableNoteData>& target)
{
    animeSlideLoop->AddRef();
    imageTap->AddRef();
    auto loopefx = SAnimeSprite::Factory(animeSlideLoop);
    loopefx->Apply("origX:128, origY:224");
    SpawnJudgeEffect(target, JudgeType::SlideTap);
    loopefx->SetLoopCount(-1);
    slideEffects[target] = loopefx;
    AddSprite(loopefx);
}

void ScenePlayer::RemoveSlideEffect()
{
    auto it = slideEffects.begin();
    while (it != slideEffects.end()) {
        auto note = (*it).first;
        auto effect = (*it).second;
        effect->Dismiss();
        it = slideEffects.erase(it);
    }
}

void ScenePlayer::UpdateSlideEffect()
{
    Prepare3DDrawCall();
    auto it = slideEffects.begin();
    while (it != slideEffects.end()) {
        auto note = (*it).first;
        auto effect = (*it).second;
        if (currentTime >= note->StartTime + note->Duration) {
            effect->Dismiss();
            it = slideEffects.erase(it);
            continue;
        }
        auto last = note;

        for (auto &slideElement : note->ExtraData) {
            if (slideElement->Type.test(size_t(SusNoteType::Control))) continue;
            if (slideElement->Type.test(size_t(SusNoteType::Injection))) continue;
            if (currentTime >= slideElement->StartTime) {
                last = slideElement;
                continue;
            }
            auto &segmentPositions = curveData[slideElement];

            auto lastSegmentPosition = segmentPositions[0];
            auto lastTimeInBlock = get<0>(lastSegmentPosition) / (slideElement->StartTime - last->StartTime);
            auto comp = false;
            for (auto &segmentPosition : segmentPositions) {
                if (lastSegmentPosition == segmentPosition) continue;
                const auto currentTimeInBlock = get<0>(segmentPosition) / (slideElement->StartTime - last->StartTime);
                const auto cst = glm::mix(last->StartTime, slideElement->StartTime, currentTimeInBlock);
                if (currentTime >= cst) {
                    lastSegmentPosition = segmentPosition;
                    lastTimeInBlock = currentTimeInBlock;
                    continue;
                }
                const auto lst = glm::mix(last->StartTime, slideElement->StartTime, lastTimeInBlock);
                const auto t = (currentTime - lst) / (cst - lst);
                const auto x = glm::mix(get<1>(lastSegmentPosition), get<1>(segmentPosition), t);
                const auto absx = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, x);
                const auto at = ConvWorldPosToScreenPos(VGet(absx, SU_LANE_Y_GROUND, SU_LANE_Z_MIN));
                effect->Transform.X = at.x;
                effect->Transform.Y = at.y;
                comp = true;
                break;
            }
            if (comp) break;
        }
        ++it;
    }
}


void ScenePlayer::DrawShortNotes(const shared_ptr<SusDrawableNoteData>& note) const
{
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
    const auto relpos = 1.0 - note->ModifiedPosition / seenDuration;
    const auto length = note->Length;
    const auto slane = note->StartLane;
    const auto zlane = note->CenterAtZero - length / 2.0f;
    const auto rlane = glm::mix(zlane, float(slane), relpos);
    SImage *handleToDraw = nullptr;

    if (note->Type.test(size_t(SusNoteType::Tap))) {
        handleToDraw = imageTap;
    } else if (note->Type.test(size_t(SusNoteType::ExTap))) {
        handleToDraw = imageExTap;
    } else if (note->Type.test(size_t(SusNoteType::AwesomeExTap))) {
        handleToDraw = imageExTap;
    } else if (note->Type.test(size_t(SusNoteType::Flick))) {
        handleToDraw = imageFlick;
    } else if (note->Type.test(size_t(SusNoteType::HellTap))) {
        handleToDraw = imageHellTap;
    } else if (note->Type.test(size_t(SusNoteType::Air))) {
        handleToDraw = imageAir;
    }

    //64*3 x 64 Çï`âÊÇ∑ÇÈÇ©ÇÁ1/2Ç≈Ç‚ÇÈïKóvÇ™Ç†ÇÈ

    if (handleToDraw) DrawTap(rlane, length, relpos, handleToDraw->GetHandle());
}

void ScenePlayer::DrawAirNotes(const AirDrawQuery &query) const
{
    auto note = query.Note;
    const auto length = note->Length;
    const auto slane = note->StartLane;
    const auto z = glm::mix(SU_LANE_Z_MAX, SU_LANE_Z_MIN, query.Z);
    const auto left = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, slane / 16.0);
    const auto right = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, (slane + length) / 16.0);
    double refroll;
    if (note->ExtraAttribute->RollTimeline) {
        auto state = note->ExtraAttribute->RollTimeline->GetRawDrawStateAt(currentTime);
        const auto mp = note->StartTimeEx - get<1>(state);
        refroll = NormalizedFmod(-mp * airRollSpeed, 0.5);
    } else {
        refroll = NormalizedFmod(-note->ModifiedPosition * airRollSpeed, 0.5);
    }
    const auto roll = note->Type.test(size_t(SusNoteType::Up)) ? refroll : 0.5 - refroll;
    const auto xadjust = note->Type.test(size_t(SusNoteType::Left)) ? -80.0 : (note->Type.test(size_t(SusNoteType::Right)) ? 80.0 : 0);
    const auto handle = note->Type.test(size_t(SusNoteType::Up)) ? imageAirUp->GetHandle() : imageAirDown->GetHandle();

    VERTEX3D vertices[] = {
        {
            VGet(left + xadjust, SU_LANE_Y_AIRINDICATE, z),
            VGet(0, 0, -1),
            GetColorU8(255, 255, 255, 255),
            GetColorU8(0, 0, 0, 0),
            0.0f, roll, 0.0f, 0.0f
        },
        {
            VGet(right + xadjust, SU_LANE_Y_AIRINDICATE, z),
            VGet(0, 0, -1),
            GetColorU8(255, 255, 255, 255),
            GetColorU8(0, 0, 0, 0),
            1.0f, roll, 0.0f, 0.0f
        },
        {
            VGet(right, SU_LANE_Y_GROUND, z),
            VGet(0, 0, -1),
            GetColorU8(255, 255, 255, 255),
            GetColorU8(0, 0, 0, 0),
            1.0f, roll + 0.5f, 0.0f, 0.0f
        },
        {
            VGet(left, SU_LANE_Y_GROUND, z),
            VGet(0, 0, -1),
            GetColorU8(255, 255, 255, 255),
            GetColorU8(0, 0, 0, 0),
            0.0f, roll + 0.5f, 0.0f, 0.0f
        }
    };
    Prepare3DDrawCall();
    SetUseZBuffer3D(FALSE);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
    DrawPolygonIndexed3D(vertices, 4, rectVertexIndices, 2, handle, TRUE);
}

void ScenePlayer::DrawHoldNotes(const shared_ptr<SusDrawableNoteData>& note) const
{
    const auto length = note->Length;
    const auto slane = note->StartLane;
    const auto endpoint = note->ExtraData.back();
    const auto relpos = 1.0 - note->ModifiedPosition / seenDuration;
    const auto reltailpos = 1.0 - endpoint->ModifiedPosition / seenDuration;
    // íÜêgÇæÇØêÊÇ…ï`âÊ
    // 1âÊñ ï™Ç≈8ï™äÑÇÆÇÁÇ¢Ç≈ÇÊÇ≥ÇªÇ§
    const int segments = fabs(relpos - reltailpos) * 8 + 1;
    SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
    for (auto i = 0; i < segments; i++) {
        const auto head = glm::mix(relpos, reltailpos, double(i) / segments);
        const auto tail = glm::mix(relpos, reltailpos, double(i + 1) / segments);
        if ((head < 0 && tail < 0) || (head >= cullingLimit && tail >= cullingLimit)) continue;
        DrawRectModiGraphF(
            slane * widthPerLane, laneBufferY * head,
            (slane + length) * widthPerLane, laneBufferY * head,
            (slane + length) * widthPerLane, laneBufferY * tail,
            slane * widthPerLane, laneBufferY * tail,
            0, (192.0 * i) / segments, noteImageBlockX, 192.0 / segments,
            imageHoldStrut->GetHandle(), TRUE
        );
    }

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
    DrawTap(slane, length, relpos, imageHold->GetHandle());

    for (auto &ex : note->ExtraData) {
        if (ex->Type.test(size_t(SusNoteType::Injection))) continue;
        const auto relendpos = 1.0 - ex->ModifiedPosition / seenDuration;
        DrawTap(slane, length, relendpos, imageHold->GetHandle());
    }
}

void ScenePlayer::DrawSlideNotes(const shared_ptr<SusDrawableNoteData>& note)
{
    auto lastStep = note;
    const auto strutBottom = 1.0;
    slideVertices.clear();
    slideIndices.clear();

    // éxíå
    auto drawcount = 0;
    uint16_t base = 0;
    for (auto &slideElement : note->ExtraData) {
        if (slideElement->Type.test(size_t(SusNoteType::Control))) continue;
        if (slideElement->Type.test(size_t(SusNoteType::Injection))) continue;
        auto &segmentPositions = curveData[slideElement];

        auto lastSegmentPosition = segmentPositions[0];
        auto lastSegmentLength = double(lastStep->Length);
        auto lastSegmentRelativeY = 1.0 - lastStep->ModifiedPosition / seenDuration;
        auto lastTimeInBlock = get<0>(lastSegmentPosition) / (slideElement->StartTime - lastStep->StartTime);

        for (const auto &segmentPosition : segmentPositions) {
            if (lastSegmentPosition == segmentPosition) continue;
            const auto currentTimeInBlock = get<0>(segmentPosition) / (slideElement->StartTime - lastStep->StartTime);
            const auto currentSegmentLength = glm::mix(double(lastStep->Length), double(slideElement->Length), currentTimeInBlock);
            const auto segmentExPosition = glm::mix(lastStep->ModifiedPosition, slideElement->ModifiedPosition, currentTimeInBlock);
            const auto currentSegmentRelativeY = 1.0 - segmentExPosition / seenDuration;
            if ((currentSegmentRelativeY >= 0 || lastSegmentRelativeY >= 0)
                && (currentSegmentRelativeY < cullingLimit || lastSegmentRelativeY < cullingLimit)) {
                slideVertices.push_back(
                    {
                        VGet(get<1>(lastSegmentPosition) * laneBufferX - lastSegmentLength / 2 * widthPerLane, laneBufferY * lastSegmentRelativeY, 0),
                        1.0f,
                        GetColorU8(255, 255, 255, 255),
                        0.0f, float(lastTimeInBlock * strutBottom)
                    }
                );
                slideVertices.push_back(
                    {
                        VGet(get<1>(lastSegmentPosition) * laneBufferX + lastSegmentLength / 2 * widthPerLane, laneBufferY * lastSegmentRelativeY, 0),
                        1.0f,
                        GetColorU8(255, 255, 255, 255),
                        1.0f, float(lastTimeInBlock * strutBottom)
                    }
                );
                slideVertices.push_back(
                    {
                        VGet(get<1>(segmentPosition) * laneBufferX - currentSegmentLength / 2 * widthPerLane, laneBufferY * currentSegmentRelativeY, 0),
                        1.0f,
                        GetColorU8(255, 255, 255, 255),
                        0.0f, float(currentTimeInBlock * strutBottom)
                    }
                );
                slideVertices.push_back(
                    {
                        VGet(get<1>(segmentPosition) * laneBufferX + currentSegmentLength / 2 * widthPerLane, laneBufferY * currentSegmentRelativeY, 0),
                        1.0f,
                        GetColorU8(255, 255, 255, 255),
                        1.0f, float(currentTimeInBlock * strutBottom)
                    }
                );
                vector<uint16_t> here = { base, uint16_t(base + 2), uint16_t(base + 1), uint16_t(base + 2), uint16_t(base + 1), uint16_t(base + 3) };
                slideIndices.insert(slideIndices.end(), here.begin(), here.end());
                base += 4;
                drawcount += 2;
            }
            lastSegmentPosition = segmentPosition;
            lastSegmentLength = currentSegmentLength;
            lastSegmentRelativeY = currentSegmentRelativeY;
            lastTimeInBlock = currentTimeInBlock;
        }
        lastStep = slideElement;
    }
    SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
    SetUseBackCulling(FALSE);
    DrawPolygonIndexed2D(slideVertices.data(), slideVertices.size(), slideIndices.data(), drawcount, imageSlideStrut->GetHandle(), TRUE);

    // íÜêSê¸
    if (showSlideLine) {
        lastStep = note;
        for (auto &slideElement : note->ExtraData) {
            if (slideElement->Type.test(size_t(SusNoteType::Control))) continue;
            if (slideElement->Type.test(size_t(SusNoteType::Injection))) continue;
            auto &segmentPositions = curveData[slideElement];
            auto lastSegmentPosition = segmentPositions[0];
            auto lastSegmentRelativeY = 1.0 - lastStep->ModifiedPosition / seenDuration;

            for (auto &segmentPosition : segmentPositions) {
                if (lastSegmentPosition == segmentPosition) continue;
                const auto currentTimeInBlock = get<0>(segmentPosition) / (slideElement->StartTime - lastStep->StartTime);
                const auto segmentExPosition = glm::mix(lastStep->ModifiedPosition, slideElement->ModifiedPosition, currentTimeInBlock);
                const auto currentSegmentRelativeY = 1.0 - segmentExPosition / seenDuration;
                if ((currentSegmentRelativeY >= 0 || lastSegmentRelativeY >= 0)
                    && (currentSegmentRelativeY < cullingLimit || lastSegmentRelativeY < cullingLimit)) {
                    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
                    DrawLineAA(
                        get<1>(lastSegmentPosition) * laneBufferX, laneBufferY * lastSegmentRelativeY,
                        get<1>(segmentPosition) * laneBufferX, laneBufferY * currentSegmentRelativeY,
                        slideLineColor, 16
                    );
                }
                lastSegmentPosition = segmentPosition;
                lastSegmentRelativeY = currentSegmentRelativeY;
            }
            lastStep = slideElement;
        }
    }

    // Tap
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
    DrawTap(note->StartLane, note->Length, 1.0 - note->ModifiedPosition / seenDuration, imageSlide->GetHandle());
    for (auto &slideElement : note->ExtraData) {
        if (slideElement->Type.test(size_t(SusNoteType::Control))) continue;
        if (slideElement->Type.test(size_t(SusNoteType::Injection))) continue;
        if (slideElement->Type.test(size_t(SusNoteType::Invisible))) continue;
        const auto currentStepRelativeY = 1.0 - slideElement->ModifiedPosition / seenDuration;
        if (currentStepRelativeY >= 0 && currentStepRelativeY < cullingLimit)
            DrawTap(slideElement->StartLane, slideElement->Length, currentStepRelativeY, imageSlide->GetHandle());
    }
}

void ScenePlayer::DrawAirActionStart(const AirDrawQuery &query) const
{
    const auto lastStep = query.Note;
    const auto lastStepRelativeY = query.Z;

    const auto aasz = glm::mix(SU_LANE_Z_MAX, SU_LANE_Z_MIN, lastStepRelativeY);
    const auto cry = (double(lastStep->StartLane) + lastStep->Length / 2.0) / 16.0;
    const auto center = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, cry);
    VERTEX3D vertices[] = {
        {
            VGet(center - 10, SU_LANE_Y_GROUND, aasz),
            VGet(0, 0, -1),
            GetColorU8(255, 255, 255, 255),
            GetColorU8(0, 0, 0, 0),
            0.9375f, 1.0f, 1.0f, 0.0f
        },
        {
            VGet(center - 10, SU_LANE_Y_AIR * lastStep->ExtraAttribute->HeightScale, aasz),
            VGet(0, 0, -1),
            GetColorU8(255, 255, 255, 255),
            GetColorU8(0, 0, 0, 0),
            0.9375f, 0.0f, 0.0f, 0.0f
        },
        {
            VGet(center + 10, SU_LANE_Y_AIR * lastStep->ExtraAttribute->HeightScale, aasz),
            VGet(0, 0, -1),
            GetColorU8(255, 255, 255, 255),
            GetColorU8(0, 0, 0, 0),
            1.0000f, 0.0f, 0.0f, 0.0f
        },
        {
            VGet(center + 10, SU_LANE_Y_GROUND, aasz),
            VGet(0, 0, -1),
            GetColorU8(255, 255, 255, 255),
            GetColorU8(0, 0, 0, 0),
            1.0000f, 1.0f, 0.0f, 0.0f
        },
    };
    DrawPolygonIndexed3D(vertices, 4, rectVertexIndices, 2, imageAirAction->GetHandle(), TRUE);
}

void ScenePlayer::DrawAirActionStepBox(const AirDrawQuery &query) const
{
    const auto slideElement = query.Note;
    const auto currentStepRelativeY = query.Z;

    SetUseZBuffer3D(TRUE);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
    if (!slideElement->Type.test(size_t(SusNoteType::Invisible))) {
        const auto atLeft = (slideElement->StartLane) / 16.0;
        const auto atRight = (slideElement->StartLane + slideElement->Length) / 16.0;
        const auto left = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, atLeft) + 5;
        const auto right = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, atRight) - 5;
        const auto z = glm::mix(SU_LANE_Z_MAX, SU_LANE_Z_MIN, currentStepRelativeY);
        const auto color = GetColorU8(255, 255, 255, 255);
        const auto yBase = SU_LANE_Y_AIR * slideElement->ExtraAttribute->HeightScale;
        VERTEX3D vertices[] = {
            { VGet(left, SU_LANE_Y_GROUND, z), VGet(0, 0, -1), GetColorU8(255, 255, 255, 255), GetColorU8(0, 0, 0, 0), 0.625f, 1.0f, 0.0f, 0.0f },
        { VGet(left, yBase, z), VGet(0, 0, -1), GetColorU8(255, 255, 255, 255), GetColorU8(0, 0, 0, 0), 0.625f, 0.0f, 0.0f, 0.0f },
        { VGet(left, yBase, z - 20), VGet(0, 0, -1), color, GetColorU8(0, 0, 0, 0), 0.0f, 0.25f, 0.0f, 0.0f },
        { VGet(left, yBase + 20, z - 20), VGet(0, 0, -1), color, GetColorU8(0, 0, 0, 0), 0.125f, 0.25f, 0.0f, 0.0f },
        { VGet(left, yBase + 20, z - 20), VGet(0, 0, -1), color, GetColorU8(0, 0, 0, 0), 0.208f, 1.0f, 0.0f, 0.0f },
        { VGet(left, yBase + 40, z - 20), VGet(0, 0, -1), color, GetColorU8(0, 0, 0, 0), 0.208f, 0.5f, 0.0f, 0.0f },
        { VGet(left, yBase, z + 20), VGet(0, 0, -1), color, GetColorU8(0, 0, 0, 0), 0.0f, 0.0f, 0.0f, 0.0f },
        { VGet(left, yBase + 20, z + 20), VGet(0, 0, -1), color, GetColorU8(0, 0, 0, 0), 0.125f, 0.0f, 0.0f, 0.0f },
        { VGet(left, yBase + 20, z + 20), VGet(0, 0, -1), color, GetColorU8(0, 0, 0, 0), 0.0f, 1.0f, 0.0f, 0.0f },
        { VGet(left, yBase + 40, z + 20), VGet(0, 0, -1), color, GetColorU8(0, 0, 0, 0), 0.0f, 0.5f, 0.0f, 0.0f },

        { VGet(right, SU_LANE_Y_GROUND, z), VGet(0, 0, -1), GetColorU8(255, 255, 255, 255), GetColorU8(0, 0, 0, 0), 0.875f, 1.0f, 0.0f, 0.0f },
        { VGet(right, yBase, z), VGet(0, 0, -1), GetColorU8(255, 255, 255, 255), GetColorU8(0, 0, 0, 0), 0.875f, 0.0f, 0.0f, 0.0f },
        { VGet(right, yBase, z - 20), VGet(0, 0, -1), color, GetColorU8(0, 0, 0, 0), 0.625f, 0.25f, 0.0f, 0.0f },
        { VGet(right, yBase + 20, z - 20), VGet(0, 0, -1), color, GetColorU8(0, 0, 0, 0), 0.5f, 0.25f, 0.0f, 0.0f },
        { VGet(right, yBase + 20, z - 20), VGet(0, 0, -1), color, GetColorU8(0, 0, 0, 0), 0.625f, 1.0f, 0.0f, 0.0f },
        { VGet(right, yBase + 40, z - 20), VGet(0, 0, -1), color, GetColorU8(0, 0, 0, 0), 0.625f, 0.5f, 0.0f, 0.0f },
        { VGet(right, yBase, z + 20), VGet(0, 0, -1), color, GetColorU8(0, 0, 0, 0), 0.625f, 0.0f, 0.0f, 0.0f },
        { VGet(right, yBase + 20, z + 20), VGet(0, 0, -1), color, GetColorU8(0, 0, 0, 0), 0.5f, 0.0f, 0.0f, 0.0f },
        { VGet(right, yBase + 20, z + 20), VGet(0, 0, -1), color, GetColorU8(0, 0, 0, 0), 0.416f, 1.0f, 0.0f, 0.0f },
        { VGet(right, yBase + 40, z + 20), VGet(0, 0, -1), color, GetColorU8(0, 0, 0, 0), 0.416f, 0.5f, 0.0f, 0.0f },

        { VGet(left, yBase, z - 20), VGet(0, 0, -1), color, GetColorU8(0, 0, 0, 0), 0.125f, 0.5f, 0.0f, 0.0f },
        { VGet(right, yBase, z - 20), VGet(0, 0, -1), color, GetColorU8(0, 0, 0, 0), 0.5f, 0.5f, 0.0f, 0.0f },
        };
        uint16_t indices[] = {
            // ñ{ìñÇÕè„2Ç¬Ç¢ÇÁÇ»Ç¢ÇØÇ«indexåvéZÇ™ñ ì|Ç»ÇÃÇ≈ï˙íu
            //â∫ÇÃÇ‚Ç¬
            0, 1, 11,
            0, 11, 10,
            //ñ{ëÃ
            //è„
            3, 7, 17,
            3, 17, 13,
            //ç∂
            6, 7, 3,
            6, 3, 2,
            //âE
            12, 13, 17,
            12, 17, 16,
            //éËëO
            20, 3, 13,
            20, 13, 21,

            //Ç÷ÇŒÇËÇ¬Ç¢ÇƒÇÈÇÃ
            //éËëO
            4, 5, 15,
            4, 15, 14,
            //å„ÇÎ
            8, 9, 19,
            8, 19, 18,
            //ç∂
            8, 9, 5,
            8, 5, 4,
            //âE
            14, 15, 19,
            14, 19, 18,
        };
        SetUseZBuffer3D(TRUE);
        DrawPolygonIndexed3D(vertices, 22, indices + 6, 16, imageAirAction->GetHandle(), TRUE);
    }
}

void ScenePlayer::DrawAirActionStep(const AirDrawQuery &query) const
{
    const auto slideElement = query.Note;
    const auto currentStepRelativeY = query.Z;

    SetUseZBuffer3D(TRUE);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
    if (!slideElement->Type.test(size_t(SusNoteType::Invisible))) {
        const auto atLeft = (slideElement->StartLane) / 16.0;
        const auto atRight = (slideElement->StartLane + slideElement->Length) / 16.0;
        const auto left = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, atLeft) + 5;
        const auto right = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, atRight) - 5;
        const auto z = glm::mix(SU_LANE_Z_MAX, SU_LANE_Z_MIN, currentStepRelativeY);
        const auto yBase = SU_LANE_Y_AIR * slideElement->ExtraAttribute->HeightScale;
        VERTEX3D vertices[] = {
            VERTEX3D { VGet(left, SU_LANE_Y_GROUND, z), VGet(0, 0, -1), GetColorU8(255, 255, 255, 255), GetColorU8(0, 0, 0, 0), 0.625f, 1.0f, 0.0f, 0.0f },
            VERTEX3D { VGet(left, yBase, z), VGet(0, 0, -1), GetColorU8(255, 255, 255, 255), GetColorU8(0, 0, 0, 0), 0.625f, 0.0f, 0.0f, 0.0f },
            VERTEX3D { VGet(right, SU_LANE_Y_GROUND, z), VGet(0, 0, -1), GetColorU8(255, 255, 255, 255), GetColorU8(0, 0, 0, 0), 0.875f, 1.0f, 0.0f, 0.0f },
            VERTEX3D { VGet(right, yBase, z), VGet(0, 0, -1), GetColorU8(255, 255, 255, 255), GetColorU8(0, 0, 0, 0), 0.875f, 0.0f, 0.0f, 0.0f },
        };
        uint16_t indices[] = {
            0, 1, 3,
            0, 3, 2,
        };
        SetUseZBuffer3D(FALSE);
        DrawPolygonIndexed3D(vertices, 4, indices, 2, imageAirAction->GetHandle(), TRUE);
    }
}

void ScenePlayer::DrawAirActionCover(const AirDrawQuery &query)
{
    const auto slideElement = query.Note;
    const auto lastStep = query.PreviousNote;
    const auto &segmentPositions = curveData[slideElement];

    auto lastSegmentPosition = segmentPositions[0];
    const auto blockDuration = slideElement->StartTime - lastStep->StartTime;
    auto lastSegmentLength = double(lastStep->Length);
    auto lastTimeInBlock = get<0>(lastSegmentPosition) / blockDuration;
    auto lastSegmentRelativeY = 1.0 - lastStep->ModifiedPosition / seenDuration;
    for (const auto &segmentPosition : segmentPositions) {
        if (lastSegmentPosition == segmentPosition) continue;
        const auto currentTimeInBlock = get<0>(segmentPosition) / (slideElement->StartTime - lastStep->StartTime);
        const auto currentSegmentLength = glm::mix(double(lastStep->Length), double(slideElement->Length), currentTimeInBlock);
        const auto segmentExPosition = glm::mix(lastStep->ModifiedPosition, slideElement->ModifiedPosition, currentTimeInBlock);
        const auto currentSegmentRelativeY = 1.0 - segmentExPosition / seenDuration;

        if ((currentSegmentRelativeY >= 0 || lastSegmentRelativeY >= 0)
            && (currentSegmentRelativeY < cullingLimit || lastSegmentRelativeY < cullingLimit)) {
            SetUseZBuffer3D(FALSE);
            SetUseBackCulling(FALSE);
            const auto back = glm::mix(SU_LANE_Z_MAX, SU_LANE_Z_MIN, currentSegmentRelativeY);
            const auto front = glm::mix(SU_LANE_Z_MAX, SU_LANE_Z_MIN, lastSegmentRelativeY);
            const auto backLeft = get<1>(segmentPosition) - currentSegmentLength / 32.0;
            const auto backRight = get<1>(segmentPosition) + currentSegmentLength / 32.0;
            const auto frontLeft = get<1>(lastSegmentPosition) - lastSegmentLength / 32.0;
            const auto frontRight = get<1>(lastSegmentPosition) + lastSegmentLength / 32.0;
            const auto pbl = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, backLeft);
            const auto pbr = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, backRight);
            const auto pfl = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, frontLeft);
            const auto pfr = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, frontRight);
            const auto pbz = glm::mix(lastStep->ExtraAttribute->HeightScale, slideElement->ExtraAttribute->HeightScale, currentTimeInBlock);
            const auto pfz = glm::mix(lastStep->ExtraAttribute->HeightScale, slideElement->ExtraAttribute->HeightScale, lastTimeInBlock);
            VERTEX3D vertices[] = {
                VERTEX3D { VGet(pfl, SU_LANE_Y_AIR * pfz, front), VGet(0, 0, -1), GetColorU8(255, 255, 255, 255), GetColorU8(0, 0, 0, 0), 0.875f, 1.0f, 1.0f, 0.0f },
                VERTEX3D { VGet(pbl, SU_LANE_Y_AIR * pbz, back), VGet(0, 0, -1), GetColorU8(255, 255, 255, 255), GetColorU8(0, 0, 0, 0), 0.875f, 0.0f, 0.0f, 0.0f },
                VERTEX3D { VGet(pbr, SU_LANE_Y_AIR * pbz, back), VGet(0, 0, -1), GetColorU8(255, 255, 255, 255), GetColorU8(0, 0, 0, 0), 0.9375f, 0.0f, 0.0f, 0.0f },
                VERTEX3D { VGet(pfr, SU_LANE_Y_AIR * pfz, front), VGet(0, 0, -1), GetColorU8(255, 255, 255, 255), GetColorU8(0, 0, 0, 0), 0.9375f, 1.0f, 0.0f, 0.0f },
            };
            DrawPolygonIndexed3D(vertices, 4, rectVertexIndices, 2, imageAirAction->GetHandle(), TRUE);

            vertices[0].pos.x = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, get<1>(lastSegmentPosition)) - 10;
            vertices[1].pos.x = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, get<1>(segmentPosition)) - 10;
            vertices[2].pos.x = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, get<1>(segmentPosition)) + 10;
            vertices[3].pos.x = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, get<1>(lastSegmentPosition)) + 10;
            vertices[0].u = 0.9375f; vertices[0].v = 1.0f;
            vertices[1].u = 0.9375f; vertices[1].v = 0.0f;
            vertices[2].u = 1.0000f; vertices[2].v = 0.0f;
            vertices[3].u = 1.0000f; vertices[3].v = 1.0f;
            DrawPolygonIndexed3D(vertices, 4, rectVertexIndices, 2, imageAirAction->GetHandle(), TRUE);
        }

        lastSegmentPosition = segmentPosition;
        lastSegmentLength = currentSegmentLength;
        lastSegmentRelativeY = currentSegmentRelativeY;
        lastTimeInBlock = currentTimeInBlock;
    }
}

void ScenePlayer::DrawTap(const float lane, const int length, const double relpos, const int handle) const
{
    for (auto i = 0; i < length * 2; i++) {
        const auto type = i ? (i == length * 2 - 1 ? 2 : 1) : 0;
        DrawRectRotaGraph3F(
            (lane * 2 + i) * widthPerLane / 2, laneBufferY * relpos,
            noteImageBlockX * type, (0),
            noteImageBlockX, noteImageBlockY,
            0, noteImageBlockY / 2,
            actualNoteScaleX, actualNoteScaleY, 0,
            handle, TRUE, FALSE);
    }
}

void ScenePlayer::DrawMeasureLine(const shared_ptr<SusDrawableNoteData>& note) const
{
    const auto relpos = 1.0 - note->ModifiedPosition / seenDuration;
    DrawLineAA(0, relpos * laneBufferY, laneBufferX, relpos * laneBufferY, GetColor(255, 255, 255), 6);
}

void ScenePlayer::DrawLaneDivisionLines() const
{
    const auto division = 8;
    for (auto i = 1; i < division; i++) {
        DrawLineAA(
            laneBufferX / division * i, 0,
            laneBufferX / division * i, laneBufferY * cullingLimit,
            GetColor(255, 255, 255), 3
        );
    }
}

void ScenePlayer::DrawLaneBackground() const
{
    ClearDrawScreen();
    // bg
    const int exty = laneBufferX * SU_LANE_ASPECT_EXT;
    const auto bgiw = imageLaneGround->GetWidth();
    const auto scale = laneBufferX / bgiw;
    auto cy = laneBackgroundRoll;
    while (cy > 0) cy -= imageLaneGround->GetHeight() * scale;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
    while (cy <= exty) {
        DrawRotaGraph2F(0, cy, 0, 0, scale, 0, imageLaneGround->GetHandle(), TRUE, FALSE);
        cy += imageLaneGround->GetHeight() * scale;
    }

    SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
    DrawRectRotaGraph3F(
        0, laneBufferY,
        0, 0,
        imageLaneJudgeLine->GetWidth(), imageLaneJudgeLine->GetHeight(),
        0, imageLaneJudgeLine->GetHeight() / 2,
        1, 1, 0,
        imageLaneJudgeLine->GetHandle(), TRUE, FALSE);
    processor->Draw();
    textCombo->Draw();
}

void ScenePlayer::Prepare3DDrawCall() const
{
    SetUseLighting(FALSE);
    SetCameraPositionAndTarget_UpVecY(VGet(0, cameraY, cameraZ), VGet(0, SU_LANE_Y_GROUND, cameraTargetZ));
}
