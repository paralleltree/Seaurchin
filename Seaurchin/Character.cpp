#include "Character.h"
#include "ExecutionManager.h"
#include "Result.h"
#include "Misc.h"
#include "Setting.h"

using namespace std;

CharacterManager::CharacterManager(ExecutionManager *exm)
{
    manager = exm;
    Selected = -1;
}

void CharacterManager::LoadAllCharacters()
{
    using namespace boost;
    using namespace boost::filesystem;
    using namespace boost::xpressive;
    auto log = spdlog::get("main");

    path sepath = Setting::GetRootDirectory() / SU_SKILL_DIR / SU_CHARACTER_DIR;

    for (const auto& fdata : make_iterator_range(directory_iterator(sepath), {})) {
        if (is_directory(fdata)) continue;
        auto filename = ConvertUnicodeToUTF8(fdata.path().wstring());
        if (!ends_with(filename, ".toml")) continue;
        LoadFromToml(fdata.path());
    }
    log->info(u8"キャラクター総数: {0:d}", Characters.size());
    Selected = 0;
}

void CharacterManager::Next()
{
    Selected = (Selected + Characters.size() + 1) % Characters.size();
}

void CharacterManager::Previous()
{
    Selected = (Selected + Characters.size() - 1) % Characters.size();
}

CharacterParameter* CharacterManager::GetCharacterParameter(int relative)
{
    int ri = Selected + relative;
    while (ri < 0) ri += Characters.size();
    return Characters[ri % Characters.size()].get();
}

shared_ptr<CharacterParameter> CharacterManager::GetCharacterParameterSafe(int relative)
{
    int ri = Selected + relative;
    while (ri < 0) ri += Characters.size();
    return Characters[ri % Characters.size()];
}


void CharacterManager::LoadFromToml(boost::filesystem::path file)
{
    using namespace boost::filesystem;

    auto log = spdlog::get("main");
    auto result = make_shared<CharacterParameter>();

    std::ifstream ifs(file.wstring(), ios::in);
    auto pr = toml::parse(ifs);
    ifs.close();
    if (!pr.valid()) {
        log->error(u8"スキル {0} は不正なファイルです", ConvertUnicodeToUTF8(file.wstring()));
        log->error(pr.errorReason);
        return;
    }
    auto &root = pr.value;

    try {
        result->Name = root.get<string>("Name");
        auto imgpath = Setting::GetRootDirectory() / SU_SKILL_DIR / SU_CHARACTER_DIR / ConvertUTF8ToUnicode(root.get<string>("Image"));
        result->ImagePath = ConvertUnicodeToUTF8(imgpath.wstring());

        auto ws = root.find("Metric.WholeScale");
        result->Metric.WholeScale = (ws && ws->is<double>()) ? ws->as<double>() : 1.0;
        
        auto fo = root.find("Metric.Face");
        if (fo && fo->is<vector<int>>()) {
            auto arr = fo->as<vector<int>>();
            for (int i = 0; i < 2; i++) result->Metric.FaceOrigin[i] = arr[i];
        } else {
            result->Metric.FaceOrigin[0] = 0;
            result->Metric.FaceOrigin[1] = 0;
        }

        auto sr = root.find("Metric.SmallRange");
        if (fo && fo->is<vector<int>>()) {
            auto arr = fo->as<vector<int>>();
            for (int i = 0; i < 4; i++) result->Metric.SmallRange[i] = arr[i];
        } else {
            result->Metric.SmallRange[0] = 0;
            result->Metric.SmallRange[1] = 0;
            result->Metric.SmallRange[2] = 280;
            result->Metric.SmallRange[3] = 170;
        }

        auto sr = root.find("Metric.FaceRange");
        if (fo && fo->is<vector<int>>()) {
            auto arr = fo->as<vector<int>>();
            for (int i = 0; i < 4; i++) result->Metric.FaceRange[i] = arr[i];
        } else {
            result->Metric.FaceRange[0] = 0;
            result->Metric.FaceRange[1] = 0;
            result->Metric.FaceRange[2] = 128;
            result->Metric.FaceRange[3] = 128;
        }
    } catch (exception) {
        log->error(u8"スキル {0} の読み込みに失敗しました", ConvertUnicodeToUTF8(file.wstring()));
        return;
    }
}

void RegisterCharacterTypes(asIScriptEngine *engine)
{
    engine->RegisterObjectType(SU_IF_CHARACTER_METRIC, sizeof(CharacterImageMetric), asOBJ_VALUE | asOBJ_POD);
    engine->RegisterObjectProperty(SU_IF_CHARACTER_METRIC, "double WholeScale", asOFFSET(CharacterImageMetric, WholeScale));
    engine->RegisterObjectMethod(SU_IF_CHARACTER_METRIC, "int get_FaceOrigin(uint)", asMETHOD(CharacterImageMetric, get_FaceOrigin), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_CHARACTER_METRIC, "int get_SmalLRange(uint)", asMETHOD(CharacterImageMetric, get_SmallRange), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_CHARACTER_METRIC, "int get_FaceRange(uint)", asMETHOD(CharacterImageMetric, get_FaceRange), asCALL_THISCALL);

    engine->RegisterObjectType(SU_IF_CHARACTER_PARAM, 0, asOBJ_REF | asOBJ_NOCOUNT);
    engine->RegisterObjectProperty(SU_IF_CHARACTER_PARAM, "string Name", asOFFSET(CharacterParameter, Name));
    engine->RegisterObjectProperty(SU_IF_CHARACTER_PARAM, "string ImagePath", asOFFSET(CharacterParameter, ImagePath));
    engine->RegisterObjectProperty(SU_IF_CHARACTER_PARAM, SU_IF_CHARACTER_METRIC " Metric", asOFFSET(CharacterParameter, Name));
    
    engine->RegisterObjectType(SU_IF_CHARACTER_MANAGER, 0, asOBJ_REF | asOBJ_NOCOUNT);
    engine->RegisterObjectMethod(SU_IF_CHARACTER_MANAGER, "void Next()", asMETHOD(CharacterManager, Next), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_CHARACTER_MANAGER, "void Previous()", asMETHOD(CharacterManager, Previous), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_CHARACTER_MANAGER, SU_IF_CHARACTER_PARAM "@ GetCharacter(int)", asMETHOD(CharacterManager, GetCharacterParameter), asCALL_THISCALL);
}
