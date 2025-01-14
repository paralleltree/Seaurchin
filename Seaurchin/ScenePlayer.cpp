#include "ScenePlayer.h"
#include "ScriptSprite.h"
#include "ExecutionManager.h"
#include "Result.h"
#include "Character.h"
#include "Setting.h"
#include "Misc.h"
#include "Config.h"

using namespace std;

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
    currentResult = make_shared<Result>();
    switch (manager->GetData<int>("AutoPlay", 1)) {
        case 0: {
            const auto pp = new PlayableProcessor(this);
            SetProcessorOptions(pp);
            processor = pp;
            break;
        }
        case 1:
            processor = new AutoPlayerProcessor(this);
            break;
        case 2: {
            const auto pp = new PlayableProcessor(this, true);
            SetProcessorOptions(pp);
            processor = pp;
            break;
        }
        default: break;
    }

    auto setting = manager->GetSettingInstanceSafe();
    hispeedMultiplier = setting->ReadValue<double>("Play", "Hispeed", 6);
    airRollSpeed = setting->ReadValue<double>("Play", "AirRollMultiplier", 1.5);
    soundBufferingLatency = setting->ReadValue<int>("Sound", "BufferLatency", 30) / 1000.0;
    preloadingTime = 0.5;

    LoadResources();

    const auto cp = manager->GetCharacterManagerSafe()->GetCharacterParameterSafe(0);
    const auto sp = manager->GetSkillManagerSafe()->GetSkillParameterSafe(0);
    currentCharacterInstance = CharacterInstance::CreateInstance(cp, sp, manager->GetScriptInterfaceSafe(), currentResult);
}

void ScenePlayer::SetProcessorOptions(PlayableProcessor *processor) const
{
    auto setting = manager->GetSettingInstanceSafe();
    const auto jas = setting->ReadValue<int>("Play", "JudgeAdjustSlider", 0) / 1000.0;
    const auto jms = setting->ReadValue<double>("Play", "JudgeMultiplierSlider", 1);
    const auto jaa = setting->ReadValue<int>("Play", "JudgeAdjustAirString", 200) / 1000.0;
    const auto jma = setting->ReadValue<double>("Play", "JudgeMultiplierAirString", 4);
    processor->SetJudgeAdjusts(jas, jms, jaa, jma);
}

void ScenePlayer::EnqueueJudgeSound(const JudgeSoundType type)
{
    judgeSoundQueue.push(type);
}


void ScenePlayer::Finalize()
{
    isTerminating = true;
    SoundManager::StopGlobal(soundHoldLoop->GetSample());
    SoundManager::StopGlobal(soundSlideLoop->GetSample());
    SoundManager::StopGlobal(soundAirLoop->GetSample());
    for (auto& res : resources) if (res.second) res.second->Release();
    SoundManager::StopGlobal(bgmStream);
    delete processor;
    delete bgmStream;

    fontCombo->Release();
    DeleteGraph(hGroundBuffer);
    DeleteGraph(hBlank);
    DeleteGraph(imageExtendedSlideStrut);
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

    // 譜面の読み込み
    analyzer->Reset();
    analyzer->LoadFromFile(scorefile.wstring());
    metronomeAvailable = !analyzer->SharedMetaData.ExtraFlags[size_t(SusMetaDataFlags::DisableMetronome)];
    analyzer->RenderScoreData(data, curveData);
    // 各種情報の設定
    segmentsPerSecond = analyzer->SharedMetaData.SegmentsPerSecond;
    usePrioritySort = analyzer->SharedMetaData.ExtraFlags[size_t(SusMetaDataFlags::EnableDrawPriority)];
    state = PlayingState::BgmNotLoaded;
    scoreDuration = analyzer->SharedMetaData.ScoreDuration;
    // Processor
    processor->Reset();
    // スライド描画バッファ
    uint32_t maxElements = 0;
    for (const auto& note : data) {
        if (!note->Type[size_t(SusNoteType::Slide)]) continue;
        const auto reserved = accumulate(note->ExtraData.begin(), note->ExtraData.end(), 2, [this](const int current, const shared_ptr<SusDrawableNoteData> part) {
            if (part->Type.test(size_t(SusNoteType::Control))) return current;
            if (part->Type.test(size_t(SusNoteType::Injection))) return current;
            return current + int(curveData[part].size()) + 1;
        });
        maxElements = max(maxElements, uint32_t(reserved));
    }
    slideVertices.reserve(maxElements * 4);
    slideIndices.reserve(maxElements * 6);


    // 動画・音声の読み込み
    auto file = boost::filesystem::path(scorefile).parent_path() / ConvertUTF8ToUnicode(analyzer->SharedMetaData.UWaveFileName);
    bgmStream = SoundStream::CreateFromFile(file.wstring());
    state = PlayingState::ReadyToStart;

    if (!analyzer->SharedMetaData.UMovieFileName.empty()) {
        movieFileName = (boost::filesystem::path(scorefile).parent_path() / ConvertUTF8ToUnicode(analyzer->SharedMetaData.UMovieFileName)).wstring();
    }

    // 前カウントの計算
    // WaveOffsetが1小節分より長いとめんどくさそうなので差し引いてく
    backingTime = -60.0 / analyzer->GetBpmAt(0, 0) * analyzer->GetBeatsAt(0);
    nextMetronomeTime = backingTime;
    while (backingTime > analyzer->SharedMetaData.WaveOffset) backingTime -= 60.0 / analyzer->GetBpmAt(0, 0) * analyzer->GetBeatsAt(0);
    currentTime = backingTime;

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
    copy_if(data.begin(), data.end(), back_inserter(judgeData), [&](const shared_ptr<SusDrawableNoteData> n) {
        return this->processor->ShouldJudge(n);
    });

    seenData.clear();
    copy_if(data.begin(), data.end(), back_inserter(seenData), [&](shared_ptr<SusDrawableNoteData> n) {
        const auto types = n->Type.to_ulong();
        if (types & SU_NOTE_LONG_MASK) {
            // ロング
            if (time > n->StartTime + n->Duration) return false;
            auto st = n->GetStateAt(time);
            // 先頭が見えてるならもちろん見える
            if (n->ModifiedPosition >= -preced && n->ModifiedPosition <= duration) return get<0>(st);
            // 先頭含めて全部-precedより手前なら見えない
            if (all_of(n->ExtraData.begin(), n->ExtraData.end(), [preced](const shared_ptr<SusDrawableNoteData> en) {
                if (isnan(en->ModifiedPosition)) return true;
                if (en->ModifiedPosition < -preced) return true;
                return false;
            }) && n->ModifiedPosition < -preced) return false;
            //先頭含めて全部durationより後なら見えない
            if (all_of(n->ExtraData.begin(), n->ExtraData.end(), [duration](const shared_ptr<SusDrawableNoteData> en) {
                if (isnan(en->ModifiedPosition)) return true;
                if (en->ModifiedPosition > duration) return true;
                return false;
            }) && n->ModifiedPosition > duration) return false;
            return true;
        }
        if (types & SU_NOTE_SHORT_MASK) {
            // ショート
            if (time > n->StartTime) return false;
            auto st = n->GetStateAt(time);
            if (n->ModifiedPosition < -preced || n->ModifiedPosition > duration) return false;
            return get<0>(st);
        }
        if (n->Type[size_t(SusNoteType::MeasureLine)]) {
            auto st = n->GetStateAt(time);
            if (n->ModifiedPosition < -preced || n->ModifiedPosition > duration) return false;
            return get<0>(st);
        }
        return false;
    });
    if (usePrioritySort) sort(seenData.begin(), seenData.end(), [](const shared_ptr<SusDrawableNoteData> a, const shared_ptr<SusDrawableNoteData> b) {
        return a->ExtraAttribute->Priority < b->ExtraAttribute->Priority;
    });
}

void ScenePlayer::Tick(const double delta)
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
            ++i;
        }
    }

    if (state != PlayingState::Paused) {
        if (state >= PlayingState::ReadyCounting) currentTime += delta;
        currentSoundTime = currentTime + soundBufferingLatency;
    }

    if (state >= PlayingState::Paused) {
        // ----------------------
        // (HSx1000)px / 4beats
        const auto referenceBpm = 120.0;
        // ----------------------

        auto cbpm = get<1>(analyzer->SharedBpmChanges[0]);
        if (currentTime >= 0) {
            for (const auto &bc : analyzer->SharedBpmChanges) {
                if (get<0>(bc) < currentTime) break;
                cbpm = get<1>(bc);
            }
        }
        const auto bpmMultiplier = (cbpm / analyzer->SharedMetaData.BaseBpm);
        const auto sizeFor4Beats = bpmMultiplier * hispeedMultiplier * 1000.0;
        const auto seenRatio = (SU_LANE_Z_MAX - SU_LANE_Z_MIN) / sizeFor4Beats;
        seenDuration = 60.0 * 4.0 * seenRatio / referenceBpm;

        CalculateNotes(currentTime, seenDuration, preloadingTime);
    }

    previousStatus = status;
    if (state != PlayingState::Paused) processor->Update(judgeData);
    currentResult->GetCurrentResult(&status);

    TickGraphics(delta);
    ProcessSound();
}

void ScenePlayer::ProcessSound()
{
    const auto actualOffset = analyzer->SharedMetaData.WaveOffset - soundBufferingLatency;
    if (state < PlayingState::ReadyCounting) return;

    switch (state) {
        case PlayingState::ReadyCounting:
            if (actualOffset < 0 && currentTime >= actualOffset) {
                SoundManager::PlayGlobal(bgmStream);
                state = PlayingState::BgmPreceding;
            } else if (currentTime >= 0) {
                state = PlayingState::OnlyScoreOngoing;
            } else if (nextMetronomeTime < 0 && currentTime >= nextMetronomeTime) {
                // TODO: NextMetronomeにもLatency適用？
                if (metronomeAvailable) SoundManager::PlayGlobal(soundMetronome->GetSample());
                nextMetronomeTime += 60 / analyzer->GetBpmAt(0, 0);
            }
            break;
        case PlayingState::BgmPreceding:
            if (currentTime >= 0) state = PlayingState::BothOngoing;
            break;
        case PlayingState::OnlyScoreOngoing:
            if (currentTime >= actualOffset) {
                SoundManager::PlayGlobal(bgmStream);
                state = PlayingState::BothOngoing;
            }
            break;
        case PlayingState::BothOngoing:
            if (bgmStream->GetStatus() == BASS_ACTIVE_STOPPED) {
                if (currentTime >= scoreDuration) {
                    hasEnded = true;
                    manager->Fire("Player:Completed");
                    state = PlayingState::Completed;
                } else {
                    state = PlayingState::ScoreLasting;
                }
            } else {
                if (currentTime >= scoreDuration) {
                    hasEnded = true;
                    state = PlayingState::BgmLasting;
                }
            }
            break;
        case PlayingState::BgmLasting:
            if (bgmStream->GetStatus() == BASS_ACTIVE_STOPPED) {
                manager->Fire("Player:Completed");
                state = PlayingState::Completed;
            }
            break;
        case PlayingState::ScoreLasting:
            if (currentTime >= scoreDuration) {
                hasEnded = true;
                manager->Fire("Player:Completed");
                state = PlayingState::Completed;
            }
            break;
        default: break;
    }

    if (moviePlaying) {
        movieCurrentPosition = TellMovieToGraph(movieBackground) / 1000.0;
        return;
    }
    if (currentTime >= -movieCurrentPosition) {
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
                if (soundTap) SoundManager::PlayGlobal(soundTap->GetSample());
                break;
            case JudgeSoundType::ExTap:
                if (soundExTap) SoundManager::PlayGlobal(soundExTap->GetSample());
                break;
            case JudgeSoundType::Flick:
                if (soundFlick) SoundManager::PlayGlobal(soundFlick->GetSample());
                break;
            case JudgeSoundType::Air:
                if (soundAir) SoundManager::PlayGlobal(soundAir->GetSample());
                break;
            case JudgeSoundType::AirDown:
                if (soundAirDown) SoundManager::PlayGlobal(soundAirDown->GetSample());
                break;
            case JudgeSoundType::AirAction:
                if (soundAirAction) SoundManager::PlayGlobal(soundAirAction->GetSample());
                break;
            case JudgeSoundType::Holding:
                if (soundHoldLoop) SoundManager::PlayGlobal(soundHoldLoop->GetSample());
                break;
            case JudgeSoundType::HoldStep:
                if (soundHoldStep) SoundManager::PlayGlobal(soundHoldStep->GetSample());
                break;
            case JudgeSoundType::HoldingStop:
                if (soundHoldLoop) SoundManager::StopGlobal(soundHoldLoop->GetSample());
                break;
            case JudgeSoundType::Sliding:
                if (soundSlideLoop) SoundManager::PlayGlobal(soundSlideLoop->GetSample());
                break;
            case JudgeSoundType::SlideStep:
                if (soundSlideStep) SoundManager::PlayGlobal(soundSlideStep->GetSample());
                break;
            case JudgeSoundType::SlidingStop:
                if (soundSlideLoop) SoundManager::StopGlobal(soundSlideLoop->GetSample());
                break;
            case JudgeSoundType::AirHolding:
                if (soundAirLoop) SoundManager::PlayGlobal(soundAirLoop->GetSample());
                break;
            case JudgeSoundType::AirHoldingStop:
                if (soundAirLoop) SoundManager::StopGlobal(soundAirLoop->GetSample());
                break;
            default: break;
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
    if (!movieFileName.empty()) {
        movieBackground = LoadGraph(reinterpret_cast<const char*>(movieFileName.c_str()));
        const auto offset = analyzer->SharedMetaData.MovieOffset;
        if (offset < 0) {
            // 先にシークして0.0から再生開始
            SeekMovieToGraph(movieBackground, int(-offset * 1000));
            movieCurrentPosition = 0;
        } else {
            // offset待って再生開始
            movieCurrentPosition = -offset;
        }
    }

    isReady = true;
    currentCharacterInstance->OnStart();
}

void ScenePlayer::SetPlayerResource(const string & name, SResource * resource)
{
    if (resources.find(name) != resources.end()) resources[name]->Release();
    resources[name] = resource;
}

void ScenePlayer::Play()
{
    if (!isLoadCompleted || !isReady) return;
    if (state < PlayingState::ReadyToStart) return;
    state = PlayingState::ReadyCounting;
}

double ScenePlayer::GetPlayingTime() const
{
    return currentTime;
}

void ScenePlayer::GetCurrentResult(DrawableResult *result) const
{
    currentResult->GetCurrentResult(result);
}

CharacterInstance* ScenePlayer::GetCharacterInstance() const
{
    currentCharacterInstance->AddRef();
    return currentCharacterInstance.get();
}

void ScenePlayer::MovePositionBySecond(const double sec)
{
    //実際に動いた時間で計算せよ
    if (state < PlayingState::BothOngoing && state != PlayingState::Paused) return;
    if (hasEnded) return;
    const auto gap = analyzer->SharedMetaData.WaveOffset - soundBufferingLatency;
    const auto oldBgmPos = bgmStream->GetPlayingPosition();
    const auto oldTime = currentTime;
    const auto newTime = oldTime + sec;
    auto newBgmPos = oldBgmPos + (newTime - oldTime);
    newBgmPos = max(0.0, newBgmPos);
    bgmStream->SetPlayingPosition(newBgmPos);
    currentTime = newBgmPos + gap;
    processor->MovePosition(currentTime - oldTime);
    SeekMovieToGraph(movieBackground, int((currentTime - oldTime + movieCurrentPosition) * 1000.0));
}

void ScenePlayer::MovePositionByMeasure(const int meas)
{
    if (state < PlayingState::BothOngoing && state != PlayingState::Paused) return;
    if (hasEnded) return;
    const auto gap = analyzer->SharedMetaData.WaveOffset - soundBufferingLatency;
    const auto oldBgmPos = bgmStream->GetPlayingPosition();
    const auto oldTime = currentTime;
    const int oldMeas = get<0>(analyzer->GetRelativeTime(currentTime));
    auto newTime = analyzer->GetAbsoluteTime(max(0, oldMeas + meas), 0);
    if (fabs(newTime - oldTime) <= 0.005) newTime = analyzer->GetAbsoluteTime(max(0, oldMeas + meas + (meas > 0 ? 1 : -1)), 0);
    auto newBgmPos = oldBgmPos + (newTime - oldTime);
    newBgmPos = max(0.0, newBgmPos);
    bgmStream->SetPlayingPosition(newBgmPos);
    currentTime = newBgmPos + gap;
    processor->MovePosition(currentTime - oldTime);
    SeekMovieToGraph(movieBackground, int((currentTime - oldTime + movieCurrentPosition) * 1000.0));
}

void ScenePlayer::Pause()
{
    if (state <= PlayingState::Paused || hasEnded) return;
    lastState = state;
    state = PlayingState::Paused;
    bgmStream->Pause();
    PauseMovieToGraph(movieBackground);
}

void ScenePlayer::Resume()
{
    if (state != PlayingState::Paused) return;
    state = lastState;
    bgmStream->Resume();
    PlayMovieToGraph(movieBackground);
}

void ScenePlayer::Reload()
{
    // TODO: 非同期ローディングに対応する方法を考える
    // TODO: 再生中リロードに対応
    if (state != PlayingState::Paused) return;
    // LoadWorker()で破壊される情報をとっておく
    const auto prevCurrentTime = currentTime;
    const auto prevOffset = analyzer->SharedMetaData.WaveOffset;
    const auto prevBgmPos = bgmStream->GetPlayingPosition();
    SoundManager::StopGlobal(bgmStream);
    delete bgmStream;

    SetMainWindowText(reinterpret_cast<const char*>(L"リロード中…"));
    LoadWorker();
    SetMainWindowText(reinterpret_cast<const char*>(ConvertUTF8ToUnicode(SU_APP_NAME " " SU_APP_VERSION).c_str()));

    const auto bgmMeantToBePlayedAt = prevBgmPos - (analyzer->SharedMetaData.WaveOffset - prevOffset);
    bgmStream->SetPlayingPosition(bgmMeantToBePlayedAt);
    currentTime = prevCurrentTime;
    state = PlayingState::Paused;
}

void ScenePlayer::SetJudgeCallback(asIScriptFunction *func) const
{
    if (!currentCharacterInstance) return;
    currentCharacterInstance->SetCallback(func);
}

void ScenePlayer::AdjustCamera(const double cy, const double cz, const double ctz)
{
    cameraY += cy;
    cameraZ += cz;
    cameraTargetZ += ctz;
}

void ScenePlayer::StoreResult() const
{
    currentResult->GetCurrentResult(&manager->lastResult);
}
