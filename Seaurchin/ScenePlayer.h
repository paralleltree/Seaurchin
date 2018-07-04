#pragma once

#include "Config.h"
#include "Scene.h"
#include "ScriptSprite.h"
#include "ScriptResource.h"
#include "SusAnalyzer.h"
#include "ScoreProcessor.h"
#include "SoundManager.h"
#include "MusicsManager.h"
#include "Result.h"
#include "CharacterInstance.h"

#define SU_IF_SCENE_PLAYER "ScenePlayer"

#define SU_LANE_X_MIN -400.0
#define SU_LANE_X_MAX 400.0
#define SU_LANE_X_MIN_EXT -1600.0
#define SU_LANE_X_MAX_EXT 1600.0
#define SU_LANE_Z_MIN_EXT -300.0
#define SU_LANE_Z_MIN 0.0
#define SU_LANE_Z_MAX 3000.0
#define SU_LANE_Y_GROUND 0.0
#define SU_LANE_Y_AIR 240.0
#define SU_LANE_Y_AIRINDICATE 160.0
#define SU_LANE_ASPECT ((SU_LANE_Z_MAX - SU_LANE_Z_MIN) / (SU_LANE_X_MAX - SU_LANE_X_MIN))
#define SU_LANE_ASPECT_EXT ((SU_LANE_Z_MAX - SU_LANE_Z_MIN_EXT) / (SU_LANE_X_MAX - SU_LANE_X_MIN))

enum class JudgeType {
    ShortNormal = 0,
    ShortEx,
    SlideTap,
    Action,
};

enum class JudgeSoundType {
    Tap,
    ExTap,
    Flick,
    Air,
    AirDown,
    AirAction,
    Holding,
    HoldingStop,
    Sliding,
    SlidingStop,
};

enum class PlayingState {

    ScoreNotLoaded,     // 何も始まっていない
    BgmNotLoaded,       // 譜面だけ読み込んだ
    ReadyToStart,       // 読み込みが終わったので始められる
    Paused,             // ポーズ中
    ReadyCounting,      // BGM読み終わって前カウントしてる
    BgmPreceding,       // 前カウント中だけどBGM始まってる
    OnlyScoreOngoing,   // 譜面始まったけどBGMまだ
    BothOngoing,        // 両方再生してる
    ScoreLasting,       // 譜面残ってる
    BgmLasting,         // 曲残ってる
    Completed,          // 全部終わった
};

class ExecutionManager;
class ScenePlayer : public SSprite {
    friend class ScoreProcessor;
    friend class AutoPlayerProcessor;
    friend class PlayableProcessor;

protected:
    int reference = 0;
    int hGroundBuffer;
    int hBlank;
    ExecutionManager *manager;
    SoundManager *soundManager;
    boost::lockfree::queue<JudgeSoundType> judgeSoundQueue;
    std::thread judgeSoundThread;
    std::mutex asyncMutex;
    std::unique_ptr<SusAnalyzer> analyzer;
    std::map<std::string, SResource*> resources;
    std::multiset<SSprite*, SSprite::Comparator> sprites;
    std::vector<SSprite*> spritesPending;

    SoundStream *bgmStream;
    ScoreProcessor *processor;
    bool isLoadCompleted = false;
    bool isReady = false;
    bool isTerminating = false;
    bool usePrioritySort = false;
    bool hasEnded = false;

    double cameraZ = -340, cameraY = 620, cameraTargetZ = 580; // スクショから計測
    double laneBufferX = 1024;
    double laneBufferY = laneBufferX * SU_LANE_ASPECT;
    double laneBackgroundRoll = 0, laneBackgroundSpeed = 0;
    double widthPerLane = laneBufferX / 16;
    double cullingLimit = SU_LANE_ASPECT_EXT / SU_LANE_ASPECT;
    double noteImageBlockX = 64;
    double noteImageBlockY = 64;
    double scaleNoteY = 2.0;
    double actualNoteScaleX = (widthPerLane / 2) / noteImageBlockX;
    double actualNoteScaleY = actualNoteScaleX * scaleNoteY;

    SSound *soundTap, *soundExTap, *soundFlick, *soundAir, *soundAirDown, *soundAirAction, *soundHoldLoop, *soundSlideLoop;
    SImage *imageLaneGround, *imageLaneJudgeLine;
    SImage *imageTap, *imageExTap, *imageFlick, *imageHellTap;
    SImage *imageAirUp, *imageAirDown;
    SImage *imageHold, *imageHoldStrut;
    SImage *imageSlide, *imageSlideStrut;
    SImage *imageAirAction;
    SFont *fontCombo;
    SAnimatedImage *animeTap, *animeExTap, *animeSlideTap, *animeSlideLoop, *animeAirAction;
    STextSprite *textCombo;
    int movieBackground = 0;
    double movieCurrentPosition = 0.0;
    bool moviePlaying = false;
    std::wstring movieFileName = L"";
    unsigned int slideLineColor = GetColor(0, 200, 255);
    unsigned int airActionLineColor = GetColor(0, 255, 32);
    unsigned int airActionJudgeColor = GetColor(128, 255, 160);

    //Slideの重みが若干違うらしいけどそのへん許してね
    std::shared_ptr<Result> CurrentResult;
    DrawableResult PreviousStatus, Status;

    std::shared_ptr<CharacterInstance> CurrentCharacterInstance;

    // 曲の途中で変化するやつら
    DrawableNotesList data;
    DrawableNotesList seenData, judgeData;
    std::unordered_map<std::shared_ptr<SusDrawableNoteData>, SSprite*> SlideEffects;
    // 時間 横位置 Ex時間
    NoteCurvesList curveData;
    double CurrentTime = 0;
    double CurrentSoundTime = 0;
    double SeenDuration = 0.8;
    double HispeedMultiplier = 6;
    double PreloadingTime = 0.5;
    double BackingTime = 0.0;
    double NextMetronomeTime = 0.0;
    double ScoreDuration = 0.0;
    double SoundBufferingLatency = 0.030;   //TODO: 環境に若干寄り添う
    double SegmentsPerSecond = 20.0;
    PlayingState State = PlayingState::ScoreNotLoaded;
    PlayingState LastState;
    bool AirActionShown = false;
    bool MetronomeAvailable = true;

    void TickGraphics(double delta);
    void AddSprite(SSprite *sprite);
    void SetProcessorOptions(PlayableProcessor *processor);
    void LoadResources();
    void LoadWorker();
    void RemoveSlideEffect();
    void UpdateSlideEffect();
    void CalculateNotes(double time, double duration, double preced);
    void CalculateCurves(std::shared_ptr<SusDrawableNoteData> note);
    void DrawShortNotes(std::shared_ptr<SusDrawableNoteData> note);
    void DrawAirNotes(std::shared_ptr<SusDrawableNoteData> note);
    void DrawHoldNotes(std::shared_ptr<SusDrawableNoteData> note);
    void DrawSlideNotes(std::shared_ptr<SusDrawableNoteData> note);
    void DrawAirActionNotes(std::shared_ptr<SusDrawableNoteData> note);
    void DrawTap(int lane, int length, double relpos, int handle);
    void DrawMeasureLines();
    void DrawLaneBackground();
    void RefreshComboText();
    void Prepare3DDrawCall();

    void ProcessSound();
    void ProcessSoundQueue();

    void SpawnJudgeEffect(std::shared_ptr<SusDrawableNoteData> target, JudgeType type);
    void SpawnSlideLoopEffect(std::shared_ptr<SusDrawableNoteData> target);
    void EnqueueJudgeSound(JudgeSoundType type);

public:
    ScenePlayer(ExecutionManager *exm);
    ~ScenePlayer() override;

    void AdjustCamera(double cy, double cz, double ctz);
    void SetPlayerResource(const std::string &name, SResource *resource);
    void Tick(double delta) override;
    void Draw() override;
    void Finalize();

    void Initialize();
    void Load();
    bool IsLoadCompleted();
    void GetReady();
    void Play();
    double GetPlayingTime();
    CharacterInstance* GetCharacterInstance();
    void GetCurrentResult(DrawableResult *result);
    void MovePositionBySecond(double sec);
    void MovePositionByMeasure(int meas);
    void SetJudgeCallback(asIScriptFunction *func);
    void Pause();
    void Resume();
    void Reload();
    void StoreResult();
};

void RegisterPlayerScene(ExecutionManager *exm);