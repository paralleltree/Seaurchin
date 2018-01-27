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

CharacterImageSet* CharacterManager::CreateCharacterImages(int relative)
{
    int ri = Selected + relative;
    while (ri < 0) ri += Characters.size();
    auto cp = Characters[ri % Characters.size()];
    return CharacterImageSet::CreateImageSet(cp);
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
        log->error(u8"キャラクター {0} は不正なファイルです", ConvertUnicodeToUTF8(file.wstring()));
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
        if (sr && sr->is<vector<int>>()) {
            auto arr = sr->as<vector<int>>();
            for (int i = 0; i < 4; i++) result->Metric.SmallRange[i] = arr[i];
        } else {
            result->Metric.SmallRange[0] = 0;
            result->Metric.SmallRange[1] = 0;
            result->Metric.SmallRange[2] = 280;
            result->Metric.SmallRange[3] = 170;
        }

        auto fr = root.find("Metric.FaceRange");
        if (fr && fr->is<vector<int>>()) {
            auto arr = fr->as<vector<int>>();
            for (int i = 0; i < 4; i++) result->Metric.FaceRange[i] = arr[i];
        } else {
            result->Metric.FaceRange[0] = 0;
            result->Metric.FaceRange[1] = 0;
            result->Metric.FaceRange[2] = 128;
            result->Metric.FaceRange[3] = 128;
        }
    } catch (exception) {
        log->error(u8"キャラクター {0} の読み込みに失敗しました", ConvertUnicodeToUTF8(file.wstring()));
        return;
    }
    Characters.push_back(result);
}


CharacterImageSet::CharacterImageSet(shared_ptr<CharacterParameter> param)
{
    Parameter = param;
    LoadAllImage();
}

CharacterImageSet::~CharacterImageSet()
{
    ImageFull->Release();
    ImageSmall->Release();
    ImageFace->Release();
}

void CharacterImageSet::ApplyFullImage(SSprite *sprite)
{
    auto cx = Parameter->Metric.FaceOrigin[0], cy = Parameter->Metric.FaceOrigin[1];
    auto sc = Parameter->Metric.WholeScale;
    ostringstream ss;
    ss << "origX:" << cx << ", origY:" << cy << ", scaleX:" << sc << ", scaleY:" << sc;

    ImageFull->AddRef();
    sprite->set_Image(ImageFull);
    sprite->Apply(ss.str());
    sprite->Release();
}

void CharacterImageSet::ApplySmallImage(SSprite *sprite)
{
    ImageSmall->AddRef();
    sprite->set_Image(ImageSmall);
    sprite->Release();
}

void CharacterImageSet::ApplyFaceImage(SSprite *sprite)
{
    ImageFace->AddRef();
    sprite->set_Image(ImageFace);
    sprite->Release();
}

CharacterImageSet *CharacterImageSet::CreateImageSet(std::shared_ptr<CharacterParameter> param)
{
    auto result = new CharacterImageSet(param);
    result->AddRef();
    return result;
}

void CharacterImageSet::LoadAllImage()
{
    auto root = ConvertUTF8ToUnicode(Parameter->ImagePath);
    int hBase = LoadGraph(reinterpret_cast<const char*>(root.c_str()));
    int hSmall = MakeScreen(SU_CHAR_SMALL_WIDTH, SU_CHAR_SMALL_WIDTH, 1);
    int hFace = MakeScreen(SU_CHAR_FACE_SIZE, SU_CHAR_FACE_SIZE, 1);
    BEGIN_DRAW_TRANSACTION(hSmall);
    DrawRectExtendGraph(
        0, 0, SU_CHAR_SMALL_WIDTH, SU_CHAR_SMALL_HEIGHT,
        Parameter->Metric.SmallRange[0], Parameter->Metric.SmallRange[1],
        Parameter->Metric.SmallRange[2], Parameter->Metric.SmallRange[3],
        hBase, TRUE);
    BEGIN_DRAW_TRANSACTION(hFace);
    DrawRectExtendGraph(
        0, 0, SU_CHAR_FACE_SIZE, SU_CHAR_FACE_SIZE,
        Parameter->Metric.FaceRange[0], Parameter->Metric.FaceRange[1],
        Parameter->Metric.FaceRange[2], Parameter->Metric.FaceRange[3],
        hBase, TRUE);
    FINISH_DRAW_TRANSACTION;
    ImageFull = new SImage(hBase);
    ImageFull->AddRef();
    ImageSmall = new SImage(hSmall);
    ImageSmall->AddRef();
    ImageFace = new SImage(hFace);
    ImageFace->AddRef();
}

void CharacterImageSet::RegisterType(asIScriptEngine *engine)
{
    engine->RegisterObjectType(SU_IF_CHARACTER_IMAGES, 0, asOBJ_REF);
    engine->RegisterObjectBehaviour(SU_IF_CHARACTER_IMAGES, asBEHAVE_ADDREF, "void f()", asMETHOD(CharacterImageSet, AddRef), asCALL_THISCALL);
    engine->RegisterObjectBehaviour(SU_IF_CHARACTER_IMAGES, asBEHAVE_RELEASE, "void f()", asMETHOD(CharacterImageSet, Release), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_CHARACTER_IMAGES, "void ApplyFullImage(" SU_IF_SPRITE "@)", asMETHOD(CharacterImageSet, ApplyFullImage), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_CHARACTER_IMAGES, "void ApplySmallImage(" SU_IF_SPRITE "@)", asMETHOD(CharacterImageSet, ApplySmallImage), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_CHARACTER_IMAGES, "void ApplyFaceImage(" SU_IF_SPRITE "@)", asMETHOD(CharacterImageSet, ApplyFaceImage), asCALL_THISCALL);
}

void RegisterCharacterTypes(asIScriptEngine *engine)
{
    CharacterImageSet::RegisterType(engine);

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
    engine->RegisterObjectMethod(SU_IF_CHARACTER_MANAGER, SU_IF_CHARACTER_IMAGES "@ CreateCharacterImages(int)", asMETHOD(CharacterManager, CreateCharacterImages), asCALL_THISCALL);
}
