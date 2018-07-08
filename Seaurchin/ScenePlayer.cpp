#include "ScenePlayer.h"
#include "ScriptSprite.h"
#include "ExecutionManager.h"
#include "Result.h"
#include "Character.h"
#include "Setting.h"
#include "Misc.h"

using namespace std;
static const double qNaN = numeric_limits<double>::quiet_NaN();

void RegisterPlayerScene(ExecutionManager * manager)
{
    auto engine = manager->GetScriptInterfaceUnsafe()->GetEngine();

    engine->RegisterObjectType(SU_IF_SCENE_PLAYER, 0, asOBJ_REF);
    engine->RegisterObjectBehaviour(SU_IF_SCENE_PLAYER, asBEHAVE_ADDREF, "void f()", asMETHOD(ScenePlayer, AddRef), asCALL_THISCALL);
    engine->RegisterObjectBehaviour(SU_IF_SCENE_PLAYER, asBEHAVE_RELEASE, "void f()", asMETHOD(ScenePlayer, Release), asCALL_THISCALL);
    engine->RegisterObjectProperty(SU_IF_SCENE_PLAYER, "int Z", asOFFSET(ScenePlayer, ZIndex));
    engine->RegisterObjectMethod(SU_IF_SPRITE, SU_IF_SCENE_PLAYER "@ opCast()", asFUNCTION((CastReferenceType<SSprite, ScenePlayer>)), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod(SU_IF_SCENE_PLAYER, SU_IF_SPRITE "@ opImplCast()", asFUNCTION((CastReferenceType<ScenePlayer, SSprite>)), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod(SU_IF_SCENE_PLAYER, "void Initialize()", asMETHOD(ScenePlayer, Initialize), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SCENE_PLAYER, "void AdjustCamera(double, double, double)", asMETHOD(ScenePlayer, AdjustCamera), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SCENE_PLAYER, "void SetResource(const string &in, " SU_IF_IMAGE "@)", asMETHOD(ScenePlayer, SetPlayerResource), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SCENE_PLAYER, "void SetResource(const string &in, " SU_IF_FONT "@)", asMETHOD(ScenePlayer, SetPlayerResource), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SCENE_PLAYER, "void SetResource(const string &in, " SU_IF_SOUND "@)", asMETHOD(ScenePlayer, SetPlayerResource), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SCENE_PLAYER, "void SetResource(const string &in, " SU_IF_ANIMEIMAGE "@)", asMETHOD(ScenePlayer, SetPlayerResource), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SCENE_PLAYER, "void Load()", asMETHOD(ScenePlayer, Load), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SCENE_PLAYER, "bool IsLoadCompleted()", asMETHOD(ScenePlayer, IsLoadCompleted), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SCENE_PLAYER, "void GetReady()", asMETHOD(ScenePlayer, GetReady), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SCENE_PLAYER, "void Play()", asMETHOD(ScenePlayer, Play), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SCENE_PLAYER, "void Pause()", asMETHOD(ScenePlayer, Pause), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SCENE_PLAYER, "void Resume()", asMETHOD(ScenePlayer, Resume), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SCENE_PLAYER, "void Reload()", asMETHOD(ScenePlayer, Reload), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SCENE_PLAYER, "double GetCurrentTime()", asMETHOD(ScenePlayer, GetPlayingTime), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SCENE_PLAYER, SU_IF_CHARACTER_INSTANCE "@ GetCharacterInstance()", asMETHOD(ScenePlayer, GetCharacterInstance), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SCENE_PLAYER, "void GetCurrentResult(" SU_IF_DRESULT " &out)", asMETHOD(ScenePlayer, GetCurrentResult), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SCENE_PLAYER, "void MovePositionBySecond(double)", asMETHOD(ScenePlayer, MovePositionBySecond), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SCENE_PLAYER, "void MovePositionByMeasure(int)", asMETHOD(ScenePlayer, MovePositionByMeasure), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SCENE_PLAYER, "void SetJudgeCallback(" SU_IF_JUDGE_CALLBACK "@)", asMETHOD(ScenePlayer, SetJudgeCallback), asCALL_THISCALL);
}


ScenePlayer::ScenePlayer(ExecutionManager *exm) : manager(exm), judgeSoundQueue(32)
{
    soundManager = manager->GetSoundManagerUnsafe();
    judgeSoundThread = thread([this]() {
        ProcessSoundQueue();
    });
}

ScenePlayer::~ScenePlayer()
{
    Finalize();
}

void ScenePlayer::Initialize()
{
    analyzer = make_unique<SusAnalyzer>(192);
    isLoadCompleted = false;
    CurrentResult = make_shared<Result>();
    switch (manager->GetData<int>("AutoPlay", 1)) {
        case 0: {
            auto pp = new PlayableProcessor(this);
            SetProcessorOptions(pp);
            processor = pp;
            break;
        }
        case 1:
            processor = new AutoPlayerProcessor(this);
            break;
        case 2: {
            auto pp = new PlayableProcessor(this, true);
            SetProcessorOptions(pp);
            processor = pp;
            break;
        }
    }

    auto setting = manager->GetSettingInstanceSafe();
    HispeedMultiplier = setting->ReadValue<double>("Play", "Hispeed", 6);
    SoundBufferingLatency = setting->ReadValue<int>("Sound", "BufferLatency", 30) / 1000.0;
    PreloadingTime = 0.5;

    LoadResources();

    auto cp = manager->GetCharacterManagerSafe()->GetCharacterParameterSafe(0);
    auto sp = manager->GetSkillManagerSafe()->GetSkillParameterSafe(0);
    CurrentCharacterInstance = CharacterInstance::CreateInstance(cp, sp, manager->GetScriptInterfaceSafe(), CurrentResult);
}

void ScenePlayer::SetProcessorOptions(PlayableProcessor *processor)
{
    auto setting = manager->GetSettingInstanceSafe();
    double jas = setting->ReadValue<int>("Play", "JudgeAdjustSlider", 0) / 1000.0;
    double jms = setting->ReadValue<double>("Play", "JudgeMultiplierSlider", 1);
    double jaa = setting->ReadValue<int>("Play", "JudgeAdjustAirString", 200) / 1000.0;
    double jma = setting->ReadValue<double>("Play", "JudgeMultiplierAirString", 4);
    processor->SetJudgeAdjusts(jas, jms, jaa, jma);
}

void ScenePlayer::EnqueueJudgeSound(JudgeSoundType type)
{
    judgeSoundQueue.push(type);
}


void ScenePlayer::Finalize()
{
    isTerminating = true;
    soundManager->StopGlobal(soundHoldLoop->GetSample());
    soundManager->StopGlobal(soundSlideLoop->GetSample());
    for (auto& res : resources) if (res.second) res.second->Release();
    soundManager->StopGlobal(bgmStream);
    delete processor;
    delete bgmStream;

    fontCombo->Release();
    DeleteGraph(hGroundBuffer);
    DeleteGraph(hBlank);
    if (movieBackground) DeleteGraph(movieBackground);
    judgeSoundThread.join();
}

void ScenePlayer::LoadWorker()
{
    {
        lock_guard<mutex> lock(asyncMutex);
        isLoadCompleted = false;
    }

    auto mm = manager->GetMusicsManager();
    auto scorefile = mm->GetSelectedScorePath();

    analyzer->Reset();
    analyzer->LoadFromFile(scorefile.wstring());
    MetronomeAvailable = !analyzer->SharedMetaData.ExtraFlags[(size_t)SusMetaDataFlags::DisableMetronome];
    analyzer->RenderScoreData(data, curveData);
    usePrioritySort = analyzer->SharedMetaData.ExtraFlags[(size_t)SusMetaDataFlags::EnableDrawPriority];
    processor->Reset();
    State = PlayingState::BgmNotLoaded;
    ScoreDuration = analyzer->SharedMetaData.ScoreDuration;

    auto file = boost::filesystem::path(scorefile).parent_path() / ConvertUTF8ToUnicode(analyzer->SharedMetaData.UWaveFileName);
    bgmStream = SoundStream::CreateFromFile(file.wstring().c_str());
    State = PlayingState::ReadyToStart;

    if (analyzer->SharedMetaData.UMovieFileName != "") {
        movieFileName = (boost::filesystem::path(scorefile).parent_path() / ConvertUTF8ToUnicode(analyzer->SharedMetaData.UMovieFileName)).wstring();
    }

    // 前カウントの計算
    // WaveOffsetが1小節分より長いとめんどくさそうなので差し引いてく
    BackingTime = -60.0 / analyzer->GetBpmAt(0, 0) * analyzer->GetBeatsAt(0);
    NextMetronomeTime = BackingTime;
    while (BackingTime > analyzer->SharedMetaData.WaveOffset) BackingTime -= 60.0 / analyzer->GetBpmAt(0, 0) * analyzer->GetBeatsAt(0);
    CurrentTime = BackingTime;

    {
        lock_guard<mutex> lock(asyncMutex);
        manager->SetData("Player:Title", analyzer->SharedMetaData.UTitle);
        manager->SetData("Player:Artist", analyzer->SharedMetaData.UArtist);
        manager->SetData<int>("Player:Level", analyzer->SharedMetaData.Level);
        isLoadCompleted = true;
    }
}

void ScenePlayer::CalculateNotes(double time, double duration, double preced)
{
    judgeData.clear();
    copy_if(data.begin(), data.end(), back_inserter(judgeData), [&](shared_ptr<SusDrawableNoteData> n) {
        return this->processor->ShouldJudge(n);
    });

    seenData.clear();
    copy_if(data.begin(), data.end(), back_inserter(seenData), [&](shared_ptr<SusDrawableNoteData> n) {
        double ptime = time - preced;
        if (n->Type.to_ulong() & SU_NOTE_LONG_MASK) {
            // ロング
            if (time > n->StartTime + n->Duration) return false;
            auto st = n->GetStateAt(time);
            // 先頭が見えてるならもちろん見える
            if (n->ModifiedPosition >= -preced && n->ModifiedPosition <= duration) return get<0>(st);
            // 先頭含めて全部-precedより手前なら見えない
            if (all_of(n->ExtraData.begin(), n->ExtraData.end(), [preced, duration](shared_ptr<SusDrawableNoteData> en) {
                if (isnan(en->ModifiedPosition)) return true;
                if (en->ModifiedPosition < -preced) return true;
                return false;
            }) && n->ModifiedPosition < -preced) return false;
            //先頭含めて全部durationより後なら見えない
            if (all_of(n->ExtraData.begin(), n->ExtraData.end(), [preced, duration](shared_ptr<SusDrawableNoteData> en) {
                if (isnan(en->ModifiedPosition)) return true;
                if (en->ModifiedPosition > duration) return true;
                return false;
            }) && n->ModifiedPosition > duration) return false;
            return true;
        } else {
            // ショート
            if (time > n->StartTime) return false;
            auto st = n->GetStateAt(time);
            if (n->ModifiedPosition < -preced || n->ModifiedPosition > duration) return false;
            return get<0>(st);
        }
    });
    if (usePrioritySort) sort(seenData.begin(), seenData.end(), [](shared_ptr<SusDrawableNoteData> a, shared_ptr<SusDrawableNoteData> b) {
        return a->ExtraAttribute->Priority < b->ExtraAttribute->Priority;
    });
}

void ScenePlayer::Tick(double delta)
{
    textCombo->Tick(delta);

    for (auto& sprite : spritesPending) sprites.emplace(sprite);
    spritesPending.clear();
    auto i = sprites.begin();
    while (i != sprites.end()) {
        (*i)->Tick(delta);
        if ((*i)->IsDead) {
            (*i)->Release();
            i = sprites.erase(i);
        } else {
            i++;
        }
    }

    if (State != PlayingState::Paused) {
        if (State >= PlayingState::ReadyCounting) CurrentTime += delta;
        CurrentSoundTime = CurrentTime + SoundBufferingLatency;
    }

    if (State >= PlayingState::Paused) {
        // ----------------------
        // (HSx1000)px / 4beats
        double referenceBpm = 120.0;
        // ----------------------

        double cbpm = get<1>(analyzer->SharedBpmChanges[0]);
        for (const auto &bc : analyzer->SharedBpmChanges) {
            if (get<0>(bc) < CurrentTime) break;
            cbpm = get<1>(bc);
        }
        double bpmMultiplier = (cbpm / analyzer->SharedMetaData.BaseBpm);
        double sizeFor4Beats = bpmMultiplier * HispeedMultiplier * 1000.0;
        double seenRatio = (SU_LANE_Z_MAX - SU_LANE_Z_MIN) / sizeFor4Beats;
        SeenDuration = 60.0 * 4.0 * seenRatio / referenceBpm;

        CalculateNotes(CurrentTime, SeenDuration, PreloadingTime);
    }

    PreviousStatus = Status;
    if (State != PlayingState::Paused) processor->Update(judgeData);
    CurrentResult->GetCurrentResult(&Status);

    TickGraphics(delta);
    ProcessSound();
}

void ScenePlayer::ProcessSound()
{
    double ActualOffset = analyzer->SharedMetaData.WaveOffset - SoundBufferingLatency;
    if (State < PlayingState::ReadyCounting) return;

    switch (State) {
        case PlayingState::ReadyCounting:
            if (ActualOffset < 0 && CurrentTime >= ActualOffset) {
                soundManager->PlayGlobal(bgmStream);
                State = PlayingState::BgmPreceding;
            } else if (CurrentTime >= 0) {
                State = PlayingState::OnlyScoreOngoing;
            } else if (NextMetronomeTime < 0 && CurrentTime >= NextMetronomeTime) {
                //TODO: NextMetronomeにもLatency適用？
                if (MetronomeAvailable) soundManager->PlayGlobal(soundTap->GetSample());
                NextMetronomeTime += 60 / analyzer->GetBpmAt(0, 0);
            }
            break;
        case PlayingState::BgmPreceding:
            if (CurrentTime >= 0) State = PlayingState::BothOngoing;
            break;
        case PlayingState::OnlyScoreOngoing:
            if (CurrentTime >= ActualOffset) {
                soundManager->PlayGlobal(bgmStream);
                State = PlayingState::BothOngoing;
            }
            break;
        case PlayingState::BothOngoing:
            if (bgmStream->GetStatus() == BASS_ACTIVE_STOPPED) {
                if (CurrentTime >= ScoreDuration) {
                    hasEnded = true;
                    manager->Fire("Player:Completed");
                    State = PlayingState::Completed;
                } else {
                    State = PlayingState::ScoreLasting;
                }
            } else {
                if (CurrentTime >= ScoreDuration) {
                    hasEnded = true;
                    State = PlayingState::BgmLasting;
                }
            }
            break;
        case PlayingState::BgmLasting:
            if (bgmStream->GetStatus() == BASS_ACTIVE_STOPPED) {
                manager->Fire("Player:Completed");
                State = PlayingState::Completed;
            }
            break;
        case PlayingState::ScoreLasting:
            if (CurrentTime >= ScoreDuration) {
                hasEnded = true;
                manager->Fire("Player:Completed");
                State = PlayingState::Completed;
            }
            break;
    }

    if (moviePlaying) {
        movieCurrentPosition = TellMovieToGraph(movieBackground) / 1000.0;
        return;
    }
    if (CurrentTime >= -movieCurrentPosition) {
        PlayMovieToGraph(movieBackground);
        moviePlaying = true;
    }
}

void ScenePlayer::ProcessSoundQueue()
{
    JudgeSoundType type;
    while (!isTerminating) {
        if (!judgeSoundQueue.pop(type)) {
            Sleep(0);
            continue;
        }
        switch (type) {
            case JudgeSoundType::Tap:
                if (soundTap) soundManager->PlayGlobal(soundTap->GetSample());
                break;
            case JudgeSoundType::ExTap:
                if (soundExTap) soundManager->PlayGlobal(soundExTap->GetSample());
                break;
            case JudgeSoundType::Flick:
                if (soundFlick) soundManager->PlayGlobal(soundFlick->GetSample());
                break;
            case JudgeSoundType::Air:
                if (soundAir) soundManager->PlayGlobal(soundAir->GetSample());
                break;
            case JudgeSoundType::AirDown:
                if (soundAirDown) soundManager->PlayGlobal(soundAirDown->GetSample());
                break;
            case JudgeSoundType::AirAction:
                if (soundAirAction) soundManager->PlayGlobal(soundAirAction->GetSample());
                break;
            case JudgeSoundType::Holding:
                if (soundHoldLoop) soundManager->PlayGlobal(soundHoldLoop->GetSample());
                break;
            case JudgeSoundType::HoldingStop:
                if (soundHoldLoop) soundManager->StopGlobal(soundHoldLoop->GetSample());
                break;
            case JudgeSoundType::Sliding:
                if (soundSlideLoop) soundManager->PlayGlobal(soundSlideLoop->GetSample());
                break;
            case JudgeSoundType::SlidingStop:
                if (soundSlideLoop) soundManager->StopGlobal(soundSlideLoop->GetSample());
                break;
        }
    }
}

// スクリプト側から呼べるやつら

void ScenePlayer::Load()
{
    thread loadThread([&] { LoadWorker(); });
    loadThread.detach();
    //LoadWorker();
}

bool ScenePlayer::IsLoadCompleted()
{
    lock_guard<mutex> lock(asyncMutex);
    return isLoadCompleted;
}

void ScenePlayer::GetReady()
{
    if (!isLoadCompleted || isReady) return;
    
    // これはUIスレッドでやる必要あり マジかよ
    if (movieFileName != L"") {
        movieBackground = LoadGraph(reinterpret_cast<const char*>(movieFileName.c_str()));
        double offset = analyzer->SharedMetaData.MovieOffset;
        if (offset < 0) {
            // 先にシークして0.0から再生開始
            SeekMovieToGraph(movieBackground, (int)(-offset * 1000));
            movieCurrentPosition = 0;
        } else {
            // offset待って再生開始
            movieCurrentPosition = -offset;
        }
    }

    isReady = true;
    CurrentCharacterInstance->OnStart();
}

void ScenePlayer::SetPlayerResource(const string & name, SResource * resource)
{
    if (resources.find(name) != resources.end()) resources[name]->Release();
    resources[name] = resource;
}

void ScenePlayer::Play()
{
    if (!isLoadCompleted || !isReady) return;
    if (State < PlayingState::ReadyToStart) return;
    State = PlayingState::ReadyCounting;
}

double ScenePlayer::GetPlayingTime()
{
    return CurrentTime;
}

void ScenePlayer::GetCurrentResult(DrawableResult *result)
{
    CurrentResult->GetCurrentResult(result);
}

CharacterInstance* ScenePlayer::GetCharacterInstance()
{
    CurrentCharacterInstance->AddRef();
    return CurrentCharacterInstance.get();
}

void ScenePlayer::MovePositionBySecond(double sec)
{
    //実際に動いた時間で計算せよ
    if (State < PlayingState::BothOngoing && State != PlayingState::Paused) return;
    if (hasEnded) return;
    double gap = analyzer->SharedMetaData.WaveOffset - SoundBufferingLatency;
    double oldBgmPos = bgmStream->GetPlayingPosition();
    double oldTime = CurrentTime;
    int oldMeas = get<0>(analyzer->GetRelativeTime(CurrentTime));
    double newTime = oldTime + sec;
    double newBgmPos = oldBgmPos + (newTime - oldTime);
    newBgmPos = max(0.0, newBgmPos);
    bgmStream->SetPlayingPosition(newBgmPos);
    CurrentTime = newBgmPos + gap;
    processor->MovePosition(CurrentTime - oldTime);
    SeekMovieToGraph(movieBackground, (int)((CurrentTime - oldTime + movieCurrentPosition) * 1000.0));
}

void ScenePlayer::MovePositionByMeasure(int meas)
{
    if (State < PlayingState::BothOngoing && State != PlayingState::Paused) return;
    if (hasEnded) return;
    double gap = analyzer->SharedMetaData.WaveOffset - SoundBufferingLatency;
    double oldBgmPos = bgmStream->GetPlayingPosition();
    double oldTime = CurrentTime;
    int oldMeas = get<0>(analyzer->GetRelativeTime(CurrentTime));
    double newTime = analyzer->GetAbsoluteTime(max(0, oldMeas + meas), 0);
    if (fabs(newTime - oldTime) <= 0.005) newTime = analyzer->GetAbsoluteTime(max(0, oldMeas + meas + (meas > 0 ? 1 : -1)), 0);
    double newBgmPos = oldBgmPos + (newTime - oldTime);
    newBgmPos = max(0.0, newBgmPos);
    bgmStream->SetPlayingPosition(newBgmPos);
    CurrentTime = newBgmPos + gap;
    processor->MovePosition(CurrentTime - oldTime);
    SeekMovieToGraph(movieBackground, (int)((CurrentTime - oldTime + movieCurrentPosition) * 1000.0));
}

void ScenePlayer::Pause()
{
    if (State <= PlayingState::Paused || hasEnded) return;
    LastState = State;
    State = PlayingState::Paused;
    bgmStream->Pause();
    PauseMovieToGraph(movieBackground);
}

void ScenePlayer::Resume()
{
    if (State != PlayingState::Paused) return;
    State = LastState;
    bgmStream->Resume();
    PlayMovieToGraph(movieBackground);
}

void ScenePlayer::Reload()
{
    // TODO: 非同期ローディングに対応する方法を考える
    // TODO: 再生中リロードに対応
    if (State != PlayingState::Paused) return;
    // LoadWorker()で破壊される情報をとっておく
    auto prevCurrentTime = CurrentTime;
    auto prevOffset = analyzer->SharedMetaData.WaveOffset;
    auto prevBgmPos = bgmStream->GetPlayingPosition();
    soundManager->StopGlobal(bgmStream);
    delete bgmStream;

    SetMainWindowText(reinterpret_cast<const char*>(L"リロード中…"));
    LoadWorker();
    SetMainWindowText(reinterpret_cast<const char*>(ConvertUTF8ToUnicode(SU_APP_NAME " " SU_APP_VERSION).c_str()));

    auto bgmMeantToBePlayedAt = prevBgmPos - (analyzer->SharedMetaData.WaveOffset - prevOffset);
    bgmStream->SetPlayingPosition(bgmMeantToBePlayedAt);
    CurrentTime = prevCurrentTime;
    State = PlayingState::Paused;
}

void ScenePlayer::SetJudgeCallback(asIScriptFunction *func)
{
    if (!CurrentCharacterInstance) return;
    CurrentCharacterInstance->SetCallback(func);
}

void ScenePlayer::AdjustCamera(double cy, double cz, double ctz)
{
    cameraY += cy;
    cameraZ += cz;
    cameraTargetZ += ctz;
}

void ScenePlayer::StoreResult()
{
    CurrentResult->GetCurrentResult(&manager->LastResult);
}