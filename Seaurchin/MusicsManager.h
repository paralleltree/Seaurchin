#pragma once

#include "SusAnalyzer.h"
#include "Setting.h"

#define SU_IF_MSCURSOR "MusicCursor"
#define SU_IF_MSCSTATE "CursorState"

struct MusicScoreInfo final {
    uint16_t Difficulty = 0;
    uint16_t Level = 0;
    double BpmToShow = 0.0;
    std::string DifficultyName;
    std::string Designer;
    boost::filesystem::path Path;
    boost::filesystem::path WavePath;
    boost::filesystem::path BackgroundPath;
};

struct MusicMetaInfo final {
    std::string SongId;
    std::string Name;
    std::string Artist;
    boost::filesystem::path JacketPath;
    std::vector<std::shared_ptr<MusicScoreInfo>> Scores;
};


class CategoryInfo final {
private:

    std::string name;
    boost::filesystem::path categoryPath;

public:
    std::vector<std::shared_ptr<MusicMetaInfo>> Musics;

    explicit CategoryInfo(boost::filesystem::path cpath);
    ~CategoryInfo();

    std::string GetName() const { return name; }
    void Reload(bool recreateCache) const;
};

//musicにsはつかないって？知るかバカ
class MusicSelectionCursor;
enum class MusicSelectionState {
    OutOfFunction = 0,
    Category,
    Music,
    Variant,
    Confirmed,

    Error,
    Success,
};

class ExecutionManager;
class MusicsManager final {
    friend class MusicSelectionCursor;
private:
    std::shared_ptr<Setting> sharedSetting;
    ExecutionManager *manager;

    bool loading = false;
    std::mutex flagMutex;
    std::unique_ptr<SusAnalyzer> analyzer;
    std::vector<std::shared_ptr<CategoryInfo>> categories;
    void CreateMusicCache();

public:
    explicit MusicsManager(ExecutionManager *exm);
    ~MusicsManager();

    static void Initialize();
    void Reload(bool recreateCache);
    bool IsReloading();
    boost::filesystem::path GetSelectedScorePath();

    MusicSelectionCursor *CreateMusicSelectionCursor();
    const std::vector<std::shared_ptr<CategoryInfo>> &GetCategories() const { return categories; }
};

class MusicSelectionCursor final {
    friend class MusicsManager;
private:
    int refcount = 0;

    MusicsManager *manager;
    int32_t categoryIndex;
    int32_t musicIndex;
    uint16_t variantIndex;
    MusicSelectionState state;

    std::shared_ptr<MusicMetaInfo> GetMusicAt(int32_t relative) const;
    std::shared_ptr<MusicScoreInfo> GetScoreVariantAt(int32_t relative) const;

public:
    MusicSelectionCursor(MusicsManager *manager);
    void AddRef() { refcount++; }
    void Release() { if (--refcount == 0) delete this; }

    std::string GetPrimaryString(int32_t relativeIndex) const;
    std::string GetCategoryName(int32_t relativeIndex) const;
    std::string GetMusicName(int32_t relativeIndex) const;
    std::string GetArtistName(int32_t relativeIndex) const;
    std::string GetMusicJacketFileName(int32_t relativeIndex) const;
    std::string GetBackgroundFileName(int32_t relativeIndex) const;
    int GetDifficulty(int32_t relativeIndex) const;
    int GetLevel(int32_t relativeIndex) const;
    double GetBpm(int32_t relativeIndex) const;
    std::string GetExtraLevel(int32_t relativeIndex) const;
    std::string GetDesignerName(int32_t relativeIndex) const;

    MusicSelectionState Enter();
    MusicSelectionState Exit();
    static MusicSelectionState Start();
    MusicSelectionState Next();
    MusicSelectionState Previous();
    MusicSelectionState NextVariant();
    MusicSelectionState PreviousVariant();
    MusicSelectionState GetState() const;

    static void RegisterScriptInterface(asIScriptEngine *engine);
};

struct MusicRawData final {
    std::string SongId;
    std::string Name;
    std::string Artist;
    std::vector<std::tuple<std::string, std::string, std::string, std::string>> Scores;
};