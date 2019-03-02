#include "MusicsManager.h"
#include "ExecutionManager.h"
#include "Config.h"
#include "Misc.h"

using namespace std;
using namespace boost;
using namespace filesystem;
using namespace xpressive;

//path sepath = Setting::GetRootDirectory() / SU_DATA_DIR / SU_SKIN_DIR;

MusicsManager::MusicsManager(ExecutionManager *exm)
    : manager(exm)
    , analyzer(make_unique<SusAnalyzer>(192))
{
}

MusicsManager::~MusicsManager()
{}

void MusicsManager::Initialize()
{}

void MusicsManager::Reload(bool recreateCache)
{   /*
    if (recreateCache) {
        thread loadthread([this] { CreateMusicCache(); });
        loadthread.detach();
    }
    */
    CreateMusicCache();
}

bool MusicsManager::IsReloading()
{
    flagMutex.lock();
    const auto state = loading;
    flagMutex.unlock();
    return state;
}

path MusicsManager::GetSelectedScorePath()
{
    const auto ci = manager->GetData<int>("Selected:Category");
    const auto mi = manager->GetData<int>("Selected:Music");
    const auto vi = manager->GetData<int>("Selected:Variant");
    auto result = Setting::GetRootDirectory() / SU_MUSIC_DIR / ConvertUTF8ToUnicode(categories[ci]->GetName());
    result /= categories[ci]->Musics[mi]->Scores[vi]->Path;
    return result;
}

void MusicsManager::CreateMusicCache()
{
    flagMutex.lock();
    loading = true;
    flagMutex.unlock();

    const auto mlpath = Setting::GetRootDirectory() / SU_MUSIC_DIR;
    for (const auto& fdata : make_iterator_range(directory_iterator(mlpath), {})) {
        if (!is_directory(fdata)) continue;

        auto category = make_shared<CategoryInfo>(fdata);
        for (const auto& mdir : make_iterator_range(directory_iterator(fdata), {})) {
            if (!is_directory(mdir)) continue;
            for (const auto& file : make_iterator_range(directory_iterator(mdir), {})) {
                if (is_directory(file)) continue;
                if (file.path().extension() != ".sus") continue;     //これ大文字どうすんの
                analyzer->Reset();
                analyzer->LoadFromFile(file.path().wstring(), true);
                auto music = find_if(category->Musics.begin(), category->Musics.end(), [&](const std::shared_ptr<MusicMetaInfo> info) {
                    return info->SongId == analyzer->SharedMetaData.USongId;
                });
                if (music == category->Musics.end()) {
                    music = category->Musics.insert(category->Musics.begin(), make_shared<MusicMetaInfo>());
                    (*music)->SongId = analyzer->SharedMetaData.USongId;
                    (*music)->Name = analyzer->SharedMetaData.UTitle;
                    (*music)->Artist = analyzer->SharedMetaData.UArtist;
                    (*music)->JacketPath = mdir.path().filename() / ConvertUTF8ToUnicode(analyzer->SharedMetaData.UJacketFileName);
                }
                auto score = make_shared<MusicScoreInfo>();
                score->Path = mdir.path().filename() / file.path().filename();
                score->BackgroundPath = ConvertUTF8ToUnicode(analyzer->SharedMetaData.UBackgroundFileName);
                score->WavePath = ConvertUTF8ToUnicode(analyzer->SharedMetaData.UWaveFileName);
                score->Designer = analyzer->SharedMetaData.UDesigner;
                score->BpmToShow = analyzer->SharedMetaData.ShowBpm;
                score->Difficulty = analyzer->SharedMetaData.DifficultyType;
                score->DifficultyName = analyzer->SharedMetaData.UExtraDifficulty;
                score->Level = analyzer->SharedMetaData.Level;
                (*music)->Scores.push_back(score);
            }
        }
        categories.push_back(category);
    }

    flagMutex.lock();
    loading = false;
    flagMutex.unlock();
}

MusicSelectionCursor *MusicsManager::CreateMusicSelectionCursor()
{
    auto result = new MusicSelectionCursor(this);
    result->AddRef();
    return result;
}

// CategoryInfo ---------------------------

CategoryInfo::CategoryInfo(const path cpath)
{
    categoryPath = cpath;
    name = ConvertUnicodeToUTF8(categoryPath.filename().wstring());
}

CategoryInfo::~CategoryInfo()
{}

// ReSharper disable once CppMemberFunctionMayBeStatic
void CategoryInfo::Reload(bool recreateCache) const
{

}

//MusicSelectionCursor ------------------------------------------

std::shared_ptr<CategoryInfo> MusicSelectionCursor::GetCategoryAt(const int32_t relative) const
{
    const auto categorySize = GetCategorySize();
    if (categorySize <= 0) return nullptr;
    auto actual = relative + categoryIndex;
    while (actual < 0) actual += categorySize;
    return manager->categories[actual % categorySize];
}

std::shared_ptr<MusicMetaInfo> MusicSelectionCursor::GetMusicAt(const int32_t relative) const
{
    const auto musicSize = GetMusicSize(0);
    if (musicSize <= 0) return nullptr;
    auto actual = relative + musicIndex;
    while (actual < 0) actual += musicSize;
    return GetCategoryAt(0)->Musics[actual % musicSize];
}

std::shared_ptr<MusicScoreInfo> MusicSelectionCursor::GetScoreVariantAt(const int32_t relative) const
{
    const auto music = GetMusicAt(relative);
    if (!music) return nullptr;
    const auto variant = min(int32_t(variantIndex), GetVariantSize(relative) - 1);
    if (variant < 0) return nullptr;
    return music->Scores[variant];
}

MusicSelectionCursor::MusicSelectionCursor(MusicsManager *manager)
    : manager(manager)
    , categoryIndex(0)
    , musicIndex(-1)
    , variantIndex(-1)
    , state(MusicSelectionState::Category)
{}

std::string MusicSelectionCursor::GetPrimaryString(const int32_t relativeIndex) const
{
    switch (state) {
        case MusicSelectionState::Category:
            return GetCategoryName(relativeIndex);
        case MusicSelectionState::Music:
            return GetMusicName(relativeIndex);
        default:
            return "";
    }
}

string MusicSelectionCursor::GetCategoryName(const int32_t relativeIndex) const
{
    const auto category = GetCategoryAt(relativeIndex);
    return category ? category->GetName() : "Unavailable";
}

string MusicSelectionCursor::GetMusicName(const int32_t relativeIndex) const
{
    const auto music = GetMusicAt(relativeIndex);
    return music ? music->Name : "Unavailable!";
}

string MusicSelectionCursor::GetArtistName(const int32_t relativeIndex) const
{
    const auto music = GetMusicAt(relativeIndex);
    return music ? music->Artist : "Unavailable!";
}

string MusicSelectionCursor::GetMusicJacketFileName(const int32_t relativeIndex) const
{
    const auto music = GetMusicAt(relativeIndex);
    if (!music || music->JacketPath.empty()) return "";
    const auto result = (Setting::GetRootDirectory() / SU_MUSIC_DIR / ConvertUTF8ToUnicode(GetCategoryName(0)) / music->JacketPath).wstring();
    return ConvertUnicodeToUTF8(result);
}

string MusicSelectionCursor::GetBackgroundFileName(const int32_t relativeIndex) const
{
    const auto variant = GetScoreVariantAt(relativeIndex);
    if (!variant || variant->BackgroundPath.empty()) return "";
    auto result = Setting::GetRootDirectory() / SU_MUSIC_DIR / ConvertUTF8ToUnicode(GetCategoryName(0)) / variant->Path.parent_path() / variant->BackgroundPath;
    return ConvertUnicodeToUTF8(result.wstring());
}

int MusicSelectionCursor::GetDifficulty(const int32_t relativeIndex) const
{
    const auto variant = GetScoreVariantAt(relativeIndex);
    return variant ? variant->Difficulty : 0;
}

int MusicSelectionCursor::GetLevel(const int32_t relativeIndex) const
{
    const auto variant = GetScoreVariantAt(relativeIndex);
    return variant ? variant->Level : 0;
}

double MusicSelectionCursor::GetBpm(const int32_t relativeIndex) const
{
    const auto variant = GetScoreVariantAt(relativeIndex);
    return variant ? variant->BpmToShow : 0;
}

std::string MusicSelectionCursor::GetExtraLevel(const int32_t relativeIndex) const
{
    const auto variant = GetScoreVariantAt(relativeIndex);
    return variant ? variant->DifficultyName : "";
}

std::string MusicSelectionCursor::GetDesignerName(const int32_t relativeIndex) const
{
    const auto variant = GetScoreVariantAt(relativeIndex);
    return variant ? variant->Designer : "";
}

MusicSelectionState MusicSelectionCursor::Enter()
{
    switch (state) {
        case MusicSelectionState::Category:
            if (GetCategorySize() <= 0) return MusicSelectionState::OutOfFunction;
            state = MusicSelectionState::Music;
            musicIndex = 0;
            variantIndex = 0;
            return state;
        case MusicSelectionState::Music:
            //選曲終了
            manager->manager->SetData<int>("Selected:Category", categoryIndex);
            manager->manager->SetData<int>("Selected:Music", musicIndex);
            manager->manager->SetData<int>("Selected:Variant", variantIndex);
            manager->manager->SetData("Player:Jacket", GetMusicJacketFileName(0));
            manager->manager->SetData("Player:Background", GetBackgroundFileName(0));
            return MusicSelectionState::Confirmed;
        default:
            return MusicSelectionState::Success;
    }
}

MusicSelectionState MusicSelectionCursor::Exit()
{
    switch (state) {
        case MusicSelectionState::Category:
            state = MusicSelectionState::OutOfFunction;
            break;
        case MusicSelectionState::Music:
            state = MusicSelectionState::Category;
            break;
        default:
            return state;
    }
    return state;
}

MusicSelectionState MusicSelectionCursor::Start()
{
    return MusicSelectionState::Success;
}

MusicSelectionState MusicSelectionCursor::Next()
{
    switch (state) {
        case MusicSelectionState::Category: {
            const auto categorySize = GetCategorySize();
            if (categorySize <= 0) return MusicSelectionState::Error;
            categoryIndex = (categoryIndex + 1) % categorySize;
            break;

        }
        case MusicSelectionState::Music: {
            const auto musicSize = GetMusicSize(0);
            if (musicSize <= 0) return MusicSelectionState::Error;
            musicIndex = (musicIndex + 1) % musicSize;
            const auto nextVariant = min(int32_t(variantIndex), GetVariantSize(0) - 1);
            if (nextVariant < 0) return MusicSelectionState::Error;
            variantIndex = uint16_t(nextVariant);
            break;
        }
        default: break;
    }
    return MusicSelectionState::Success;
}

MusicSelectionState MusicSelectionCursor::Previous()
{
    switch (state) {
        case MusicSelectionState::Category: {
            const auto categorySize = GetCategorySize();
            if (categorySize <= 0) return MusicSelectionState::Error;
            categoryIndex = (categoryIndex + categorySize - 1) % categorySize;
            break;

        }
        case MusicSelectionState::Music: {
            const auto musicSize = GetMusicSize(0);
            if (musicSize <= 0) return MusicSelectionState::Error;
            musicIndex = (musicIndex + musicSize - 1) % musicSize;
            const auto nextVariant = min(int32_t(variantIndex), GetVariantSize(0) - 1);
            if (nextVariant < 0) return MusicSelectionState::Error;
            variantIndex = uint16_t(nextVariant);
            break;
        }
        default: break;
    }
    return MusicSelectionState::Success;
}

MusicSelectionState MusicSelectionCursor::NextVariant()
{
    switch (state) {
        case MusicSelectionState::Category:
            return MusicSelectionState::Error;
        case MusicSelectionState::Music: {
            const auto variantSize = GetVariantSize(0);
            if (variantSize <= 0) return MusicSelectionState::Error;
            variantIndex = (variantIndex + 1) % variantSize;
            break;
        }
        default: break;

    }
    return MusicSelectionState::Success;
}

MusicSelectionState MusicSelectionCursor::PreviousVariant()
{
    switch (state) {
        case MusicSelectionState::Category:
            return MusicSelectionState::Error;
        case MusicSelectionState::Music: {
            const auto variantSize = GetVariantSize(0);
            if (variantSize <= 0) return MusicSelectionState::Error;
            variantIndex = (variantIndex + variantSize - 1) % variantSize;
        }
        default:
            return MusicSelectionState::Success;
    }
}

MusicSelectionState MusicSelectionCursor::GetState() const
{
    return state;
}

int32_t MusicSelectionCursor::GetCategorySize() const
{
    return int32_t(manager->categories.size());
}

int32_t MusicSelectionCursor::GetMusicSize(int32_t relativeIndex) const
{
    const auto category = GetCategoryAt(relativeIndex);
    return (category) ? int32_t(category->Musics.size()) : 0;
}

int32_t MusicSelectionCursor::GetVariantSize(int32_t relativeIndex) const
{
    const auto music = GetMusicAt(relativeIndex);
    return (music) ? int32_t(music->Scores.size()) : 0;
}

void MusicSelectionCursor::RegisterScriptInterface(asIScriptEngine *engine)
{
    engine->RegisterEnum(SU_IF_MSCSTATE);
    engine->RegisterEnumValue(SU_IF_MSCSTATE, "OutOfFunction", int(MusicSelectionState::OutOfFunction));
    engine->RegisterEnumValue(SU_IF_MSCSTATE, "Category", int(MusicSelectionState::Category));
    engine->RegisterEnumValue(SU_IF_MSCSTATE, "Music", int(MusicSelectionState::Music));
    engine->RegisterEnumValue(SU_IF_MSCSTATE, "Confirmed", int(MusicSelectionState::Confirmed));
    engine->RegisterEnumValue(SU_IF_MSCSTATE, "Error", int(MusicSelectionState::Error));
    engine->RegisterEnumValue(SU_IF_MSCSTATE, "Success", int(MusicSelectionState::Success));

    engine->RegisterObjectType(SU_IF_MSCURSOR, 0, asOBJ_REF);
    engine->RegisterObjectBehaviour(SU_IF_MSCURSOR, asBEHAVE_ADDREF, "void f()", asMETHOD(MusicSelectionCursor, AddRef), asCALL_THISCALL);
    engine->RegisterObjectBehaviour(SU_IF_MSCURSOR, asBEHAVE_RELEASE, "void f()", asMETHOD(MusicSelectionCursor, Release), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_MSCURSOR, "string GetPrimaryString(int)", asMETHOD(MusicSelectionCursor, GetPrimaryString), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_MSCURSOR, "string GetCategoryName(int)", asMETHOD(MusicSelectionCursor, GetCategoryName), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_MSCURSOR, "string GetMusicName(int)", asMETHOD(MusicSelectionCursor, GetMusicName), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_MSCURSOR, "string GetArtistName(int)", asMETHOD(MusicSelectionCursor, GetArtistName), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_MSCURSOR, "string GetMusicJacketFileName(int)", asMETHOD(MusicSelectionCursor, GetMusicJacketFileName), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_MSCURSOR, "string GetBackgroundFileName(int)", asMETHOD(MusicSelectionCursor, GetBackgroundFileName), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_MSCURSOR, "int GetDifficulty(int)", asMETHOD(MusicSelectionCursor, GetDifficulty), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_MSCURSOR, "int GetLevel(int)", asMETHOD(MusicSelectionCursor, GetLevel), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_MSCURSOR, "double GetBpm(int)", asMETHOD(MusicSelectionCursor, GetBpm), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_MSCURSOR, "string GetExtraLevel(int)", asMETHOD(MusicSelectionCursor, GetExtraLevel), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_MSCURSOR, "string GetDesignerName(int)", asMETHOD(MusicSelectionCursor, GetDesignerName), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_MSCURSOR, SU_IF_MSCSTATE " Next()", asMETHOD(MusicSelectionCursor, Next), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_MSCURSOR, SU_IF_MSCSTATE " Previous()", asMETHOD(MusicSelectionCursor, Previous), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_MSCURSOR, SU_IF_MSCSTATE " NextVariant()", asMETHOD(MusicSelectionCursor, NextVariant), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_MSCURSOR, SU_IF_MSCSTATE " PreviousVariant()", asMETHOD(MusicSelectionCursor, PreviousVariant), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_MSCURSOR, SU_IF_MSCSTATE " Enter()", asMETHOD(MusicSelectionCursor, Enter), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_MSCURSOR, SU_IF_MSCSTATE " Exit()", asMETHOD(MusicSelectionCursor, Exit), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_MSCURSOR, SU_IF_MSCSTATE " GetState()", asMETHOD(MusicSelectionCursor, GetState), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_MSCURSOR, "int GetCategorySize()", asMETHOD(MusicSelectionCursor, GetCategorySize), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_MSCURSOR, "int GetMusicSize(int)", asMETHOD(MusicSelectionCursor, GetMusicSize), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_MSCURSOR, "int GetVariantSize(int)", asMETHOD(MusicSelectionCursor, GetVariantSize), asCALL_THISCALL);
}
