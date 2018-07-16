#include "ScenePlayer.h"
#include "ScriptSprite.h"
#include "ExecutionManager.h"
#include "Setting.h"

using namespace std;

static const uint16_t RectVertexIndices[] = { 0, 1, 3, 3, 1, 2 };
static VERTEX3D GroundVertices[] = {
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
    soundSlideLoop = dynamic_cast<SSound*>(resources["SoundSlideLoop"]);
    soundHoldLoop = dynamic_cast<SSound*>(resources["SoundHoldLoop"]);
    soundSlideStep = dynamic_cast<SSound*>(resources["SoundSlideStep"]);
    soundHoldStep = dynamic_cast<SSound*>(resources["SoundHoldStep"]);
    soundMetronome = dynamic_cast<SSound*>(resources["Metronome"]);
    fontCombo = dynamic_cast<SFont*>(resources["FontCombo"]);

    auto setting = manager->GetSettingInstanceSafe();
    if (soundHoldLoop) soundHoldLoop->SetLoop(true);
    if (soundSlideLoop) soundSlideLoop->SetLoop(true);
    if (soundTap) soundTap->SetVolume(setting->ReadValue("Sound", "VolumeTap", 1.0));
    if (soundExTap) soundExTap->SetVolume(setting->ReadValue("Sound", "VolumeExTap", 1.0));
    if (soundFlick) soundFlick->SetVolume(setting->ReadValue("Sound", "VolumeFlick", 1.0));
    if (soundAir) soundAir->SetVolume(setting->ReadValue("Sound", "VolumeAir", 1.0));
    if (soundAirDown) soundAirDown->SetVolume(setting->ReadValue("Sound", "VolumeAir", 1.0));
    if (soundAirAction) soundAirAction->SetVolume(setting->ReadValue("Sound", "VolumeAirAction", 1.0));
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
    int exty = laneBufferX * SU_LANE_ASPECT_EXT;
    double bufferY = 2;
    while (exty > bufferY) bufferY *= 2;
    float bufferV = exty / bufferY;
    for (int i = 2; i < 4; i++) GroundVertices[i].v = bufferV;
    hGroundBuffer = MakeScreen(laneBufferX, bufferY, TRUE);
    hBlank = MakeScreen(128, 128, FALSE);
    BEGIN_DRAW_TRANSACTION(hBlank);
    DrawBox(0, 0, 128, 128, GetColor(255, 255, 255), TRUE);
    FINISH_DRAW_TRANSACTION;

    fontCombo->AddRef();
    textCombo = STextSprite::Factory(fontCombo, "0000");
    textCombo->SetAlignment(STextAlign::Center, STextAlign::Center);
    auto size = 320.0 / fontCombo->get_Size();
    ostringstream app;
    app << setprecision(5);
    app << "x:512, y:3200, " << "scaleX:" << size << ", scaleY:" << size;
    textCombo->Apply(app.str());
}

void ScenePlayer::AddSprite(SSprite *sprite)
{
    spritesPending.push_back(sprite);
}

void ScenePlayer::TickGraphics(double delta)
{
    if (Status.Combo > PreviousStatus.Combo) RefreshComboText();
    UpdateSlideEffect();
    laneBackgroundRoll += laneBackgroundSpeed * delta;
}

void ScenePlayer::Draw()
{
    ostringstream combo;
    int division = 8;
    combo << Status.Combo;
    textCombo->set_Text(combo.str());

    if (movieBackground) DrawExtendGraph(0, 0, SU_RES_WIDTH, SU_RES_HEIGHT, movieBackground, FALSE);

    BEGIN_DRAW_TRANSACTION(hGroundBuffer);
    // îwåiïî
    DrawLaneBackground();
    DrawLaneDivisionLines();
    for (auto& note : seenData) {
        auto &type = note->Type;
        if (type[(size_t)SusNoteType::MeasureLine]) DrawMeasureLine(note);
    }

    // â∫ë§ÇÃÉçÉìÉOÉmÅ[Écóﬁ
    for (auto& note : seenData) {
        auto &type = note->Type;
        if (type[(size_t)SusNoteType::Hold]) DrawHoldNotes(note);
        if (type[(size_t)SusNoteType::Slide]) DrawSlideNotes(note);
    }

    // è„ë§ÇÃÉVÉáÅ[ÉgÉmÅ[Écóﬁ
    for (auto& note : seenData) {
        auto &type = note->Type;
        if (type[(size_t)SusNoteType::Tap]) DrawShortNotes(note);
        if (type[(size_t)SusNoteType::ExTap]) DrawShortNotes(note);
        if (type[(size_t)SusNoteType::AwesomeExTap]) DrawShortNotes(note);
        if (type[(size_t)SusNoteType::Flick]) DrawShortNotes(note);
        if (type[(size_t)SusNoteType::HellTap]) DrawShortNotes(note);
        if (type[(size_t)SusNoteType::Air] && type[(size_t)SusNoteType::Grounded]) DrawShortNotes(note);
    }

    FINISH_DRAW_TRANSACTION;
    Prepare3DDrawCall();
    DrawPolygonIndexed3D(GroundVertices, 4, RectVertexIndices, 2, hGroundBuffer, TRUE);
    for (auto& i : sprites) i->Draw();

    //3DånÉmÅ[Éc
    Prepare3DDrawCall();
    DrawAerialNotes(seenData);

    if (AirActionShown && showAirActionJudge) {
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

void ScenePlayer::DrawAerialNotes(vector<shared_ptr<SusDrawableNoteData>> notes)
{
    vector<AirDrawQuery> airdraws, covers;
    for (const auto &note : seenData) {
        if (note->Type.test((size_t)SusNoteType::AirAction)) {
            // DrawAirActionNotes(note);
            AirDrawQuery head;
            head.Type = AirDrawType::AirActionStart;
            head.Z = 1.0 - note->ModifiedPosition / SeenDuration;
            head.Note = note;
            airdraws.push_back(head);
            shared_ptr<SusDrawableNoteData> prev = note;
            for (const auto extra : note->ExtraData) {
                if (extra->Type.test((size_t)SusNoteType::Control)) continue;
                if (extra->Type.test((size_t)SusNoteType::Injection)) continue;
                AirDrawQuery tail;
                tail.Type = AirDrawType::AirActionStep;
                tail.Z = 1.0 - extra->ModifiedPosition / SeenDuration;
                tail.Note = extra;
                tail.PreviousNote = prev;
                airdraws.push_back(tail);
                covers.push_back(tail);
                prev = extra;
            }
        }
        if (note->Type.test((size_t)SusNoteType::Air)) {
            // DrawAirNotes(note);
            AirDrawQuery head;
            head.Type = AirDrawType::Air;
            head.Z = 1.0 - note->ModifiedPosition / SeenDuration;
            head.Note = note;
            airdraws.push_back(head);
        }
    }
    stable_sort(airdraws.begin(), airdraws.end(), [](AirDrawQuery a, AirDrawQuery b) {
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
        }
    }
    for (const auto &query : covers) DrawAirActionCover(query);
    for (const auto &query : airdraws) {
        if (query.Type == AirDrawType::AirActionStep) DrawAirActionStepBox(query);
    }
}

void ScenePlayer::RefreshComboText()
{
    auto size = 320.0 / fontCombo->get_Size();
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
void ScenePlayer::SpawnJudgeEffect(shared_ptr<SusDrawableNoteData> target, JudgeType type)
{
    Prepare3DDrawCall();
    auto position = target->StartLane + target->Length / 2.0;
    auto x = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, position / 16.0);
    switch (type) {
        case JudgeType::ShortNormal: {
            auto spawnAt = ConvWorldPosToScreenPos(VGet(x, SU_LANE_Y_GROUND, SU_LANE_Z_MIN));
            animeTap->AddRef();
            auto sp = SAnimeSprite::Factory(animeTap);
            sp->Apply("origX:128, origY:224");
            sp->Transform.X = spawnAt.x;
            sp->Transform.Y = spawnAt.y;
            AddSprite(sp);
            break;
        }
        case JudgeType::ShortEx: {
            auto spawnAt = ConvWorldPosToScreenPos(VGet(x, SU_LANE_Y_GROUND, SU_LANE_Z_MIN));
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
            auto spawnAt = ConvWorldPosToScreenPos(VGet(x, SU_LANE_Y_GROUND, SU_LANE_Z_MIN));
            animeSlideTap->AddRef();
            auto sp = SAnimeSprite::Factory(animeSlideTap);
            sp->Apply("origX:128, origY:224");
            sp->Transform.X = spawnAt.x;
            sp->Transform.Y = spawnAt.y;
            AddSprite(sp);
            break;
        }
        case JudgeType::Action: {
            auto spawnAt = ConvWorldPosToScreenPos(VGet(x, SU_LANE_Y_AIR, SU_LANE_Z_MIN));
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

void ScenePlayer::SpawnSlideLoopEffect(shared_ptr<SusDrawableNoteData> target)
{
    animeSlideLoop->AddRef();
    imageTap->AddRef();
    auto loopefx = SAnimeSprite::Factory(animeSlideLoop);
    loopefx->Apply("origX:128, origY:224");
    SpawnJudgeEffect(target, JudgeType::SlideTap);
    loopefx->SetLoopCount(-1);
    SlideEffects[target] = loopefx;
    AddSprite(loopefx);
}

void ScenePlayer::RemoveSlideEffect()
{
    auto it = SlideEffects.begin();
    while (it != SlideEffects.end()) {
        auto note = (*it).first;
        auto effect = (*it).second;
        effect->Dismiss();
        it = SlideEffects.erase(it);
    }
}

void ScenePlayer::UpdateSlideEffect()
{
    Prepare3DDrawCall();
    auto it = SlideEffects.begin();
    while (it != SlideEffects.end()) {
        auto note = (*it).first;
        auto effect = (*it).second;
        if (CurrentTime >= note->StartTime + note->Duration) {
            effect->Dismiss();
            it = SlideEffects.erase(it);
            continue;
        }
        auto last = note;

        for (auto &slideElement : note->ExtraData) {
            if (slideElement->Type.test((size_t)SusNoteType::Control)) continue;
            if (slideElement->Type.test((size_t)SusNoteType::Injection)) continue;
            if (CurrentTime >= slideElement->StartTime) {
                last = slideElement;
                continue;
            }
            auto &segmentPositions = curveData[slideElement];

            auto lastSegmentPosition = segmentPositions[0];
            double lastTimeInBlock = get<0>(lastSegmentPosition) / (slideElement->StartTime - last->StartTime);
            bool comp = false;
            for (auto &segmentPosition : segmentPositions) {
                if (lastSegmentPosition == segmentPosition) continue;
                double currentTimeInBlock = get<0>(segmentPosition) / (slideElement->StartTime - last->StartTime);
                double cst = glm::mix(last->StartTime, slideElement->StartTime, currentTimeInBlock);
                if (CurrentTime >= cst) {
                    lastSegmentPosition = segmentPosition;
                    lastTimeInBlock = currentTimeInBlock;
                    continue;
                }
                double lst = glm::mix(last->StartTime, slideElement->StartTime, lastTimeInBlock);
                double t = (CurrentTime - lst) / (cst - lst);
                double x = glm::mix(get<1>(lastSegmentPosition), get<1>(segmentPosition), t);
                double absx = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, x);
                auto at = ConvWorldPosToScreenPos(VGet(absx, SU_LANE_Y_GROUND, SU_LANE_Z_MIN));
                effect->Transform.X = at.x;
                effect->Transform.Y = at.y;
                comp = true;
                break;
            }
            if (comp) break;
        }
        it++;
    }
}


void ScenePlayer::DrawShortNotes(shared_ptr<SusDrawableNoteData> note)
{
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
    double relpos = 1.0 - note->ModifiedPosition / SeenDuration;
    auto length = note->Length;
    auto slane = note->StartLane;
    SImage *handleToDraw = nullptr;

    if (note->Type.test((size_t)SusNoteType::Tap)) {
        handleToDraw = imageTap;
    } else if (note->Type.test((size_t)SusNoteType::ExTap)) {
        handleToDraw = imageExTap;
    } else if (note->Type.test((size_t)SusNoteType::AwesomeExTap)) {
        handleToDraw = imageExTap;
    } else if (note->Type.test((size_t)SusNoteType::Flick)) {
        handleToDraw = imageFlick;
    } else if (note->Type.test((size_t)SusNoteType::HellTap)) {
        handleToDraw = imageHellTap;
    } else if (note->Type.test((size_t)SusNoteType::Air)) {
        handleToDraw = imageAir;
    }

    //64*3 x 64 Çï`âÊÇ∑ÇÈÇ©ÇÁ1/2Ç≈Ç‚ÇÈïKóvÇ™Ç†ÇÈ
    if (handleToDraw) DrawTap(slane, length, relpos, handleToDraw->GetHandle());
}

void ScenePlayer::DrawAirNotes(const AirDrawQuery &query)
{
    auto note = query.Note;
    auto length = note->Length;
    auto slane = note->StartLane;
    double z = glm::mix(SU_LANE_Z_MAX, SU_LANE_Z_MIN, query.Z);
    auto left = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, slane / 16.0);
    auto right = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, (slane + length) / 16.0);
    auto refrole = NormalizedFmod(-note->ModifiedPosition, 0.5);
    auto role = note->Type.test((size_t)SusNoteType::Up) ? refrole : 0.5 - refrole;
    auto xadjust = note->Type.test((size_t)SusNoteType::Left) ? -80.0 : (note->Type.test((size_t)SusNoteType::Right) ? 80.0 : 0);
    auto handle = note->Type.test((size_t)SusNoteType::Up) ? imageAirUp->GetHandle() : imageAirDown->GetHandle();

    VERTEX3D vertices[] = {
        {
            VGet(left + xadjust, SU_LANE_Y_AIRINDICATE, z),
            VGet(0, 0, -1),
            GetColorU8(255, 255, 255, 255),
            GetColorU8(0, 0, 0, 0),
            0.0f, role, 0.0f, 0.0f
        },
        {
            VGet(right + xadjust, SU_LANE_Y_AIRINDICATE, z),
            VGet(0, 0, -1),
            GetColorU8(255, 255, 255, 255),
            GetColorU8(0, 0, 0, 0),
            1.0f, role, 0.0f, 0.0f
        },
        {
            VGet(right, SU_LANE_Y_GROUND, z),
            VGet(0, 0, -1),
            GetColorU8(255, 255, 255, 255),
            GetColorU8(0, 0, 0, 0),
            1.0f, role + 0.5f, 0.0f, 0.0f
        },
        {
            VGet(left, SU_LANE_Y_GROUND, z),
            VGet(0, 0, -1),
            GetColorU8(255, 255, 255, 255),
            GetColorU8(0, 0, 0, 0),
            0.0f, role + 0.5f, 0.0f, 0.0f
        }
    };
    Prepare3DDrawCall();
    SetUseZBuffer3D(FALSE);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
    DrawPolygonIndexed3D(vertices, 4, RectVertexIndices, 2, handle, TRUE);
}

void ScenePlayer::DrawHoldNotes(shared_ptr<SusDrawableNoteData> note)
{
    auto length = note->Length;
    auto slane = note->StartLane;
    auto endpoint = note->ExtraData.back();
    double relpos = 1.0 - note->ModifiedPosition / SeenDuration;
    double reltailpos = 1.0 - endpoint->ModifiedPosition / SeenDuration;
    // íÜêgÇæÇØêÊÇ…ï`âÊ
    // 1âÊñ ï™Ç≈8ï™äÑÇÆÇÁÇ¢Ç≈ÇÊÇ≥ÇªÇ§
    int segments = fabs(relpos - reltailpos) * 8 + 1;
    SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
    for (int i = 0; i < segments; i++) {
        double head = glm::mix(relpos, reltailpos, (double)i / segments);
        double tail = glm::mix(relpos, reltailpos, (double)(i + 1) / segments);
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
        if (ex->Type.test((size_t)SusNoteType::Injection)) continue;
        double relendpos = 1.0 - ex->ModifiedPosition / SeenDuration;
        DrawTap(slane, length, relendpos, imageHold->GetHandle());
    }
}

void ScenePlayer::DrawSlideNotes(shared_ptr<SusDrawableNoteData> note)
{
    auto lastStep = note;
    auto lastStepRelativeY = 1.0 - lastStep->ModifiedPosition / SeenDuration;
    double segmentLength = 128.0;   // Bufferè„Ç≈ÇÃç≈è¨ÇÃí∑Ç≥

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
    DrawTap(lastStep->StartLane, lastStep->Length, lastStepRelativeY, imageSlide->GetHandle());

    for (auto &slideElement : note->ExtraData) {
        if (slideElement->Type.test((size_t)SusNoteType::Control)) continue;
        if (slideElement->Type.test((size_t)SusNoteType::Injection)) continue;
        double currentStepRelativeY = 1.0 - slideElement->ModifiedPosition / SeenDuration;
        auto &segmentPositions = curveData[slideElement];

        auto lastSegmentPosition = segmentPositions[0];
        double lastSegmentLength = lastStep->Length;
        double lastTimeInBlock = get<0>(lastSegmentPosition) / (slideElement->StartTime - lastStep->StartTime);
        auto lastSegmentRelativeY = 1.0 - lastStep->ModifiedPosition / SeenDuration;
        double currentExPosition = get<1>(lastStep->Timeline->GetRawDrawStateAt(CurrentTime));
        for (auto &segmentPosition : segmentPositions) {
            if (lastSegmentPosition == segmentPosition) continue;
            double currentTimeInBlock = get<0>(segmentPosition) / (slideElement->StartTime - lastStep->StartTime);
            double currentSegmentLength = glm::mix((double)lastStep->Length, (double)slideElement->Length, currentTimeInBlock);
            double segmentExPosition = glm::mix(lastStep->ModifiedPosition, slideElement->ModifiedPosition, currentTimeInBlock);
            double currentSegmentRelativeY = 1.0 - segmentExPosition / SeenDuration;

            if (currentSegmentRelativeY < cullingLimit || lastSegmentRelativeY < cullingLimit) {
                SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
                DrawRectModiGraphF(
                    get<1>(lastSegmentPosition) * laneBufferX - lastSegmentLength / 2 * widthPerLane, laneBufferY * lastSegmentRelativeY,
                    get<1>(lastSegmentPosition) * laneBufferX + lastSegmentLength / 2 * widthPerLane, laneBufferY * lastSegmentRelativeY,
                    get<1>(segmentPosition) * laneBufferX + currentSegmentLength / 2 * widthPerLane, laneBufferY * currentSegmentRelativeY,
                    get<1>(segmentPosition) * laneBufferX - currentSegmentLength / 2 * widthPerLane, laneBufferY * currentSegmentRelativeY,
                    0, 192.0 * lastTimeInBlock, noteImageBlockX, 192.0 * (currentTimeInBlock - lastTimeInBlock),
                    imageSlideStrut->GetHandle(), TRUE
                );
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
                if (showSlideLine) DrawLineAA(
                    get<1>(lastSegmentPosition) * laneBufferX, laneBufferY * lastSegmentRelativeY,
                    get<1>(segmentPosition) * laneBufferX, laneBufferY * currentSegmentRelativeY,
                    slideLineColor, 16);
            }
            lastSegmentPosition = segmentPosition;
            lastSegmentLength = currentSegmentLength;
            lastSegmentRelativeY = currentSegmentRelativeY;
            lastTimeInBlock = currentTimeInBlock;
        }

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
        if (!slideElement->Type.test((size_t)SusNoteType::Invisible))
            DrawTap(slideElement->StartLane, slideElement->Length, currentStepRelativeY, imageSlide->GetHandle());

        lastStep = slideElement;
        lastStepRelativeY = currentStepRelativeY;
    }
}

void ScenePlayer::DrawAirActionStart(const AirDrawQuery &query)
{
    auto lastStep = query.Note;
    auto lastStepRelativeY = query.Z;
    double segmentLength = 128.0;   // Bufferè„Ç≈ÇÃç≈è¨ÇÃí∑Ç≥

    double aasz = glm::mix(SU_LANE_Z_MAX, SU_LANE_Z_MIN, lastStepRelativeY);
    double cry = ((double)lastStep->StartLane + lastStep->Length / 2.0) / 16.0;
    double center = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, cry);
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
    DrawPolygonIndexed3D(vertices, 4, RectVertexIndices, 2, imageAirAction->GetHandle(), TRUE);
}

void ScenePlayer::DrawAirActionStepBox(const AirDrawQuery &query)
{
    auto slideElement = query.Note;
    double currentStepRelativeY = query.Z;

    SetUseZBuffer3D(TRUE);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
    if (!slideElement->Type.test((size_t)SusNoteType::Invisible)) {
        double atLeft = (slideElement->StartLane) / 16.0;
        double atRight = (slideElement->StartLane + slideElement->Length) / 16.0;
        double left = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, atLeft) + 5;
        double right = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, atRight) - 5;
        double z = glm::mix(SU_LANE_Z_MAX, SU_LANE_Z_MIN, currentStepRelativeY);
        auto color = GetColorU8(255, 255, 255, 255);
        auto yBase = SU_LANE_Y_AIR * slideElement->ExtraAttribute->HeightScale;
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

void ScenePlayer::DrawAirActionStep(const AirDrawQuery &query)
{
    auto slideElement = query.Note;
    double currentStepRelativeY = query.Z;

    SetUseZBuffer3D(TRUE);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
    if (!slideElement->Type.test((size_t)SusNoteType::Invisible)) {
        double atLeft = (slideElement->StartLane) / 16.0;
        double atRight = (slideElement->StartLane + slideElement->Length) / 16.0;
        double left = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, atLeft) + 5;
        double right = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, atRight) - 5;
        double z = glm::mix(SU_LANE_Z_MAX, SU_LANE_Z_MIN, currentStepRelativeY);
        auto color = GetColorU8(255, 255, 255, 255);
        auto yBase = SU_LANE_Y_AIR * slideElement->ExtraAttribute->HeightScale;
        VERTEX3D vertices[] = {
            { VGet(left, SU_LANE_Y_GROUND, z), VGet(0, 0, -1), GetColorU8(255, 255, 255, 255), GetColorU8(0, 0, 0, 0), 0.625f, 1.0f, 0.0f, 0.0f },
        { VGet(left, yBase, z), VGet(0, 0, -1), GetColorU8(255, 255, 255, 255), GetColorU8(0, 0, 0, 0), 0.625f, 0.0f, 0.0f, 0.0f },

        { VGet(right, SU_LANE_Y_GROUND, z), VGet(0, 0, -1), GetColorU8(255, 255, 255, 255), GetColorU8(0, 0, 0, 0), 0.875f, 1.0f, 0.0f, 0.0f },
        { VGet(right, yBase, z), VGet(0, 0, -1), GetColorU8(255, 255, 255, 255), GetColorU8(0, 0, 0, 0), 0.875f, 0.0f, 0.0f, 0.0f },
        };
        uint16_t indices[] = {
            0, 1, 3,
            0, 3, 2,
        };
        SetUseZBuffer3D(FALSE);
        DrawPolygonIndexed3D(vertices, 22, indices, 2, imageAirAction->GetHandle(), TRUE);
    }
}

void ScenePlayer::DrawAirActionCover(const AirDrawQuery &query)
{
    auto slideElement = query.Note;
    auto lastStep = query.PreviousNote;
    auto &segmentPositions = curveData[slideElement];

    auto lastSegmentPosition = segmentPositions[0];
    double blockDuration = slideElement->StartTime - lastStep->StartTime;
    double lastSegmentLength = lastStep->Length;
    double lastTimeInBlock = get<0>(lastSegmentPosition) / blockDuration;
    auto lastSegmentRelativeY = 1.0 - lastStep->ModifiedPosition / SeenDuration;
    for (auto &segmentPosition : segmentPositions) {
        if (lastSegmentPosition == segmentPosition) continue;
        double currentTimeInBlock = get<0>(segmentPosition) / (slideElement->StartTime - lastStep->StartTime);
        double currentSegmentLength = glm::mix((double)lastStep->Length, (double)slideElement->Length, currentTimeInBlock);
        double segmentExPosition = glm::mix(lastStep->ModifiedPosition, slideElement->ModifiedPosition, currentTimeInBlock);
        double currentSegmentRelativeY = 1.0 - segmentExPosition / SeenDuration;

        if (currentSegmentRelativeY < cullingLimit || lastSegmentRelativeY < cullingLimit) {
            SetUseZBuffer3D(FALSE);
            double back = glm::mix(SU_LANE_Z_MAX, SU_LANE_Z_MIN, currentSegmentRelativeY);
            double front = glm::mix(SU_LANE_Z_MAX, SU_LANE_Z_MIN, lastSegmentRelativeY);
            double backLeft = get<1>(segmentPosition) - currentSegmentLength / 32.0;
            double backRight = get<1>(segmentPosition) + currentSegmentLength / 32.0;
            double frontLeft = get<1>(lastSegmentPosition) - lastSegmentLength / 32.0;
            double frontRight = get<1>(lastSegmentPosition) + lastSegmentLength / 32.0;
            double pbl = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, backLeft);
            double pbr = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, backRight);
            double pfl = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, frontLeft);
            double pfr = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, frontRight);
            double pbz = glm::mix(lastStep->ExtraAttribute->HeightScale, slideElement->ExtraAttribute->HeightScale, currentTimeInBlock);
            double pfz = glm::mix(lastStep->ExtraAttribute->HeightScale, slideElement->ExtraAttribute->HeightScale, lastTimeInBlock);
            // TODO: ç≈ìKâªÇµÇÎ
            VERTEX3D vertices[] = {
                { VGet(pfl, SU_LANE_Y_AIR * pfz, front), VGet(0, 0, -1), GetColorU8(255, 255, 255, 255), GetColorU8(0, 0, 0, 0), 0.875f, 1.0f, 1.0f, 0.0f },
            { VGet(pbl, SU_LANE_Y_AIR * pbz, back), VGet(0, 0, -1), GetColorU8(255, 255, 255, 255), GetColorU8(0, 0, 0, 0), 0.875f, 0.0f, 0.0f, 0.0f },
            { VGet(pbr, SU_LANE_Y_AIR * pbz, back), VGet(0, 0, -1), GetColorU8(255, 255, 255, 255), GetColorU8(0, 0, 0, 0), 0.9375f, 0.0f, 0.0f, 0.0f },
            { VGet(pfr, SU_LANE_Y_AIR * pfz, front), VGet(0, 0, -1), GetColorU8(255, 255, 255, 255), GetColorU8(0, 0, 0, 0), 0.9375f, 1.0f, 0.0f, 0.0f },
            };
            DrawPolygonIndexed3D(vertices, 4, RectVertexIndices, 2, imageAirAction->GetHandle(), TRUE);

            vertices[0].pos.x = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, get<1>(lastSegmentPosition)) - 10;
            vertices[1].pos.x = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, get<1>(segmentPosition)) - 10;
            vertices[2].pos.x = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, get<1>(segmentPosition)) + 10;
            vertices[3].pos.x = glm::mix(SU_LANE_X_MIN, SU_LANE_X_MAX, get<1>(lastSegmentPosition)) + 10;
            vertices[0].u = 0.9375f; vertices[0].v = 1.0f;
            vertices[1].u = 0.9375f; vertices[1].v = 0.0f;
            vertices[2].u = 1.0000f; vertices[2].v = 0.0f;
            vertices[3].u = 1.0000f; vertices[3].v = 1.0f;
            DrawPolygonIndexed3D(vertices, 4, RectVertexIndices, 2, imageAirAction->GetHandle(), TRUE);
        }

        lastSegmentPosition = segmentPosition;
        lastSegmentLength = currentSegmentLength;
        lastSegmentRelativeY = currentSegmentRelativeY;
        lastTimeInBlock = currentTimeInBlock;
    }
}

void ScenePlayer::DrawTap(int lane, int length, double relpos, int handle)
{
    for (int i = 0; i < length * 2; i++) {
        int type = i ? (i == length * 2 - 1 ? 2 : 1) : 0;
        DrawRectRotaGraph3F(
            (lane * 2 + i) * widthPerLane / 2, laneBufferY * relpos,
            noteImageBlockX * type, (0),
            noteImageBlockX, noteImageBlockY,
            0, noteImageBlockY / 2,
            actualNoteScaleX, actualNoteScaleY, 0,
            handle, TRUE, FALSE);
    }
}

void ScenePlayer::DrawMeasureLine(shared_ptr<SusDrawableNoteData> note)
{
    auto relpos = 1.0 - note->ModifiedPosition / SeenDuration;
    DrawLineAA(0, relpos * laneBufferY, laneBufferX, relpos * laneBufferY, GetColor(255, 255, 255), 6);
}

void ScenePlayer::DrawLaneDivisionLines()
{
    int division = 8;
    for (int i = 1; i < division; i++) {
        DrawLineAA(
            laneBufferX / division * i, 0,
            laneBufferX / division * i, laneBufferY * cullingLimit,
            GetColor(255, 255, 255), 3
        );
    }
}

void ScenePlayer::DrawLaneBackground()
{
    ClearDrawScreen();
    // bg
    int exty = laneBufferX * SU_LANE_ASPECT_EXT;
    auto bgiw = imageLaneGround->get_Width();
    double scale = laneBufferX / bgiw;
    double cy = laneBackgroundRoll;
    while (cy > 0) cy -= imageLaneGround->get_Height() * scale;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
    while (cy <= exty) {
        DrawRotaGraph2F(0, cy, 0, 0, scale, 0, imageLaneGround->GetHandle(), TRUE, FALSE);
        cy += imageLaneGround->get_Height() * scale;
    }

    SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
    DrawRectRotaGraph3F(
        0, laneBufferY,
        0, 0,
        imageLaneJudgeLine->get_Width(), imageLaneJudgeLine->get_Height(),
        0, imageLaneJudgeLine->get_Height() / 2,
        1, 1, 0,
        imageLaneJudgeLine->GetHandle(), TRUE, FALSE);
    processor->Draw();
    textCombo->Draw();
}

void ScenePlayer::Prepare3DDrawCall()
{
    SetUseLighting(FALSE);
    SetCameraPositionAndTarget_UpVecY(VGet(0, cameraY, cameraZ), VGet(0, SU_LANE_Y_GROUND, cameraTargetZ));
}