#pragma once

#include "Scene.h"
#include "ScriptSprite.h"
#include "ScriptResource.h"
#include "SusAnalyzer.h"
#include "ScoreProcessor.h"
#include "SoundManager.h"
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
    HoldStep,
    HoldingStop,
    Sliding,
    SlideStep,
    SlidingStop,
    AirHolding,
    AirHoldingStop,
    Metronome,
};

enum class PlayingState {
    ScoreNotLoaded,
    BgmNotLoaded,
    ReadyToStart,
    Paused,
    ReadyCounting,
    BgmPreceding,
    OnlyScoreOngoing,
    BothOngoing,
    ScoreLasting,
    BgmLasting,
    Completed,
};

enum class AirDrawType {
    Air,
    AirActionStart,
    AirActionStep,
    AirActionCover,
};

struct AirDrawQuery {
    double Z = 0.0;
    AirDrawType Type = AirDrawType::Air;
    std::shared_ptr<SusDrawableNoteData> Note, PreviousNote;
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

    double cameraZ = -340, cameraY = 620, cameraTargetZ = 580;
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

    SSound *soundTap, *soundExTap, *soundFlick, *soundAir, *soundAirDown, *soundAirAction, *soundAirLoop;
    SSound *soundHoldLoop, *soundSlideLoop, *soundHoldStep, *soundSlideStep;
    SSound *soundMetronome;
    SImage *imageLaneGround, *imageLaneJudgeLine;
    SImage *imageTap, *imageExTap, *imageFlick, *imageHellTap;
    SImage *imageAir, *imageAirUp, *imageAirDown;
    SImage *imageHold, *imageHoldStep, *imageHoldStrut;
    SImage *imageSlide, *imageSlideStep, *imageSlideStrut;
    SImage *imageAirAction;
    int imageExtendedSlideStrut;
    SFont *fontCombo;
    SAnimatedImage *animeTap, *animeExTap, *animeSlideTap, *animeSlideLoop, *animeAirAction;
    STextSprite *textCombo;
    int movieBackground = 0;
    double movieCurrentPosition = 0.0;
    bool moviePlaying = false;
    std::wstring movieFileName = L"";
    unsigned int slideLineColor = GetColor(0, 200, 255);
    unsigned int airActionJudgeColor = GetColor(128, 255, 160);
    bool showSlideLine, showAirActionJudge;
    int segmentsPerSecond;
    std::vector<VERTEX2D> slideVertices;
    std::vector<uint16_t> slideIndices;

    std::shared_ptr<Result> currentResult;
    DrawableResult previousStatus, status;

    std::shared_ptr<CharacterInstance> currentCharacterInstance;

    DrawableNotesList data;
    DrawableNotesList seenData, judgeData;
    std::unordered_map<std::shared_ptr<SusDrawableNoteData>, SSprite*> slideEffects;
    NoteCurvesList curveData;
    double currentTime = 0;
    double currentSoundTime = 0;
    double seenDuration = 0.8;
    double hispeedMultiplier = 6;
    double preloadingTime = 0.5;
    double backingTime = 0.0;
    double nextMetronomeTime = 0.0;
    double scoreDuration = 0.0;
    double soundBufferingLatency = 0.030;
    double airRollSpeed = 1.5;
    PlayingState state = PlayingState::ScoreNotLoaded;
    PlayingState lastState;
    bool airActionShown = false;
    bool metronomeAvailable = true;

    void TickGraphics(double delta);
    void AddSprite(SSprite *sprite);
    void SetProcessorOptions(PlayableProcessor *processor) const;
    void LoadResources();
    void LoadWorker();
    void RemoveSlideEffect();
    void UpdateSlideEffect();
    void CalculateNotes(double time, double duration, double preced);
    void DrawShortNotes(const std::shared_ptr<SusDrawableNoteData>& note) const;
    void DrawAirNotes(const AirDrawQuery &query) const;
    void DrawHoldNotes(const std::shared_ptr<SusDrawableNoteData>& note) const;
    void DrawSlideNotes(const std::shared_ptr<SusDrawableNoteData>& note);
    void DrawAirActionStart(const AirDrawQuery &query) const;
    void DrawAirActionStep(const AirDrawQuery &query) const;
    void DrawAirActionStepBox(const AirDrawQuery &query) const;
    void DrawAirActionCover(const AirDrawQuery &query);
    void DrawTap(const float lane, int length, double relpos, int handle) const;
    void DrawMeasureLine(const std::shared_ptr<SusDrawableNoteData>& note) const;
    void DrawLaneDivisionLines() const;
    void DrawLaneBackground() const;
    void RefreshComboText() const;
    void Prepare3DDrawCall() const;
    void DrawAerialNotes(const std::vector<std::shared_ptr<SusDrawableNoteData>>& notes);

    void ProcessSound();
    void ProcessSoundQueue();

    void SpawnJudgeEffect(const std::shared_ptr<SusDrawableNoteData>& target, JudgeType type);
    void SpawnSlideLoopEffect(const std::shared_ptr<SusDrawableNoteData>& target);
    void EnqueueJudgeSound(JudgeSoundType type);

public:
    explicit ScenePlayer(ExecutionManager *exm);
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
    double GetPlayingTime() const;
    CharacterInstance* GetCharacterInstance() const;
    void GetCurrentResult(DrawableResult *result) const;
    void MovePositionBySecond(double sec);
    void MovePositionByMeasure(int meas);
    void SetJudgeCallback(asIScriptFunction *func) const;
    void Pause();
    void Resume();
    void Reload();
    void StoreResult() const;
};

void RegisterPlayerScene(ExecutionManager *exm);
