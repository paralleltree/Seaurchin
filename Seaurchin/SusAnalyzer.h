#pragma once

#define SU_NOTE_LONG_MASK  0b00000000001110000000
#define SU_NOTE_SHORT_MASK 0b00000000000001111110

enum class SusNoteType : uint16_t {
	Undefined = 0,  // BPMノーツなど
    Tap,            // Tap
    ExTap,          // ExTap
	Flick,          // Flick
	Air,            // Air
    HellTap,        // AIR: Hell Tap
    AwesomeExTap,   // STAR PLUS: やべーExTap (https://twitter.com/chunithm/status/967959264055648256)

    Hold = 7,
    Slide,
	AirAction,

    Start = 10,
    Step,
	Control,
    End,

    Up = 14,
    Down,
    Left,
    Right,

    Injection = 18,
    Invisible,
    Unused3,
};

struct SusRelativeNoteTime {
    uint32_t Measure;
    uint32_t Tick;

    bool operator<(const SusRelativeNoteTime& b) const
    {
        return Measure < b.Measure || Tick < b.Tick;
    }
    bool operator>(const SusRelativeNoteTime& b) const
    {
        return Measure > b.Measure || Tick > b.Tick;
    }
    bool operator==(const SusRelativeNoteTime& b) const
    {
        return Measure == b.Measure && Tick == b.Tick;
    }
    bool operator!=(const SusRelativeNoteTime& b) const
    {
        return Measure != b.Measure || Tick != b.Tick;
    }
};


struct SusHispeedData {
    enum class Visibility {
        Keep = -1,
        Invisible,
        Visible,
    };
    const static double KeepSpeed;

    Visibility VisibilityState = Visibility::Visible;
    double Speed = 1.0;

};

class SusHispeedTimeline final {
private:
    std::vector<std::pair<SusRelativeNoteTime, SusHispeedData>> keys;
    std::vector<std::tuple<double, double, SusHispeedData>> Data;
    std::function<double(uint32_t, uint32_t)> RelToAbs;

public:
    SusHispeedTimeline(std::function<double(uint32_t, uint32_t)> func);
    void AddKeysByString(const std::string &def, std::function<std::shared_ptr<SusHispeedTimeline>(uint32_t)> resolver);
    void AddKeyByData(uint32_t meas, uint32_t tick, double hs);
    void AddKeyByData(uint32_t meas, uint32_t tick, bool vis);
    void Finialize();
    std::tuple<bool, double> GetRawDrawStateAt(double time);
    double GetSpeedAt(double time);
};

class SusNoteExtraAttribute final {
public:
    uint32_t Priority;
    double HeightScale;

    void Apply(const std::string &props);
};

enum class SusMetaDataFlags : uint16_t {
    DisableMetronome,
    EnableDrawPriority,
};

struct SusMetaData {
    std::string UTitle = u8"";
    std::string USubTitle = u8"";
    std::string UArtist = u8"";
    std::string UJacketFileName = u8"";
    std::string UDesigner = u8"";
    std::string USongId = u8"";
    std::string UWaveFileName = u8"";
    std::string UBackgroundFileName = u8"";
    std::string UMovieFileName = u8"";
    double WaveOffset = 0;
    double MovieOffset = 0;
    double BaseBpm = 0;
    double ShowBpm = -1;
    double ScoreDuration = 0;
    uint32_t Level = 0;
    uint32_t DifficultyType = 0;
    std::string UExtraDifficulty = "";
    std::bitset<8> ExtraFlags;

    void Reset()
    {
        USongId = "";
        UTitle = USubTitle = u8"";
        UArtist = UDesigner = u8"";
        UJacketFileName = UWaveFileName = UBackgroundFileName = "";
        WaveOffset = 0;
        BaseBpm = 0;
        Level = 0;
        ShowBpm = -1;
        ScoreDuration = 0;
        DifficultyType = 0;
        UExtraDifficulty = u8"";
        ExtraFlags.reset();
    }
};


struct SusRawNoteData {
    std::bitset<32> Type;
    std::shared_ptr<SusHispeedTimeline> Timeline;
    union {
        uint16_t DefinitionNumber;
        struct {
            uint8_t StartLane;
            uint8_t Length;
        } NotePosition;
    };
    uint8_t Extra;
    std::shared_ptr<SusNoteExtraAttribute> ExtraAttribute;
};

struct SusDrawableNoteData {
    std::bitset<32> Type;
    std::bitset<8> OnTheFlyData;
    std::shared_ptr<SusHispeedTimeline> Timeline;
    std::shared_ptr<SusNoteExtraAttribute> ExtraAttribute;
    uint8_t StartLane;
    uint8_t Length;

    //実描画位置
    double ModifiedPosition;
    double StartTimeEx;
    double DurationEx;
    //描画"始める"時刻
    double StartTime;
    //描画が"続く"時刻
    double Duration;
    //スライド・AA用制御データ
    std::vector<std::shared_ptr<SusDrawableNoteData>> ExtraData;

    std::tuple<bool, double> GetStateAt(double time);
};


using DrawableNotesList = std::vector<std::shared_ptr<SusDrawableNoteData>>;
using NoteCurvesList = std::unordered_map<std::shared_ptr<SusDrawableNoteData>, std::vector<std::tuple<double, double>>>;

//BMS派生フォーマットことSUS(SeaUrchinScore)の解析
class SusAnalyzer final {
private:
    static boost::xpressive::sregex RegexSusCommand;
    static boost::xpressive::sregex RegexSusData;

    double DefaultBeats = 4.0;
    double DefaultBpm = 120.0;
    uint32_t DefaultHispeedNumber = std::numeric_limits<uint32_t>::max();
    uint32_t DefaultExtraAttributeNumber = std::numeric_limits<uint32_t>::max();
    uint32_t TicksPerBeat;
    double LongInjectionPerBeat;
    int SegmentsPerSecond;
    std::function<std::shared_ptr<SusHispeedTimeline>(uint32_t)> TimelineResolver = nullptr;
    std::vector<std::function<void(std::string, std::string)>> ErrorCallbacks;
    std::vector<std::tuple<SusRelativeNoteTime, SusRawNoteData>> Notes;
    std::vector<std::tuple<SusRelativeNoteTime, SusRawNoteData>> BpmChanges;
    std::unordered_map<uint32_t, double> BpmDefinitions;
	std::unordered_map<uint32_t, float> BeatsDefinitions;
    std::unordered_map<uint32_t, std::shared_ptr<SusHispeedTimeline>> HispeedDefinitions;
    std::shared_ptr<SusHispeedTimeline> HispeedToApply;
    std::unordered_map<uint32_t, std::shared_ptr<SusNoteExtraAttribute>> ExtraAttributes;
    std::shared_ptr<SusNoteExtraAttribute> ExtraAttributeToApply;

    void ProcessCommand(const boost::xpressive::smatch &result, bool onlyMeta, uint32_t line);
    void ProcessRequest(const std::string &cmd, uint32_t line);
    void ProcessData(const boost::xpressive::smatch &result, uint32_t line);
    void MakeMessage(uint32_t line, const std::string &message);
    void MakeMessage(uint32_t meas, uint32_t tick, uint32_t lane, const std::string &message);
    void CalculateCurves(std::shared_ptr<SusDrawableNoteData> note, NoteCurvesList &curveData);

public:
    SusMetaData SharedMetaData;
    std::vector<std::tuple<double, double>> SharedBpmChanges;

    SusAnalyzer(uint32_t tpb);
    ~SusAnalyzer();

    void Reset();
    void SetMessageCallBack(std::function<void(std::string, std::string)> func);
    void LoadFromFile(const std::wstring &fileName, bool analyzeOnlyMetaData = false);
    void RenderScoreData(DrawableNotesList &data, NoteCurvesList &curveData);
    float GetBeatsAt(uint32_t measure);
    double GetBpmAt(uint32_t measure, uint32_t tick);
    double GetAbsoluteTime(uint32_t measure, uint32_t tick);
    std::tuple<uint32_t, uint32_t> GetRelativeTime(double time);
    uint32_t GetRelativeTicks(uint32_t measure, uint32_t tick);
};