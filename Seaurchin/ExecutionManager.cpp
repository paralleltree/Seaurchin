#include "ExecutionManager.h"

#include "Config.h"
#include "Debug.h"
#include "Setting.h"
#include "Interfaces.h"

#include "ScriptResource.h"
#include "ScriptScene.h"
#include "ScriptSprite.h"
#include "ScenePlayer.h"
#include "CharacterInstance.h"

using namespace boost::filesystem;
using namespace std;


ExecutionManager::ExecutionManager(std::shared_ptr<Setting> setting)
{
    random_device seed;

    SharedSetting = setting;
    SettingManager = make_unique<Setting2::SettingItemManager>(SharedSetting);
    ScriptInterface = make_shared<AngelScript>();
    Sound = make_shared<SoundManager>();
    Random = make_shared<mt19937>(seed());
    SharedControlState = make_shared<ControlState>();
    Musics = make_shared<MusicsManager>(this);
    Characters = make_shared<CharacterManager>(this);
    Skills = make_shared<SkillManager>(this);
    Extensions = make_unique<ExtensionManager>();
}

void ExecutionManager::Initialize()
{
    std::ifstream slfile;
    string procline;
    path slpath = SharedSetting->GetRootDirectory() / SU_DATA_DIR / SU_SCRIPT_DIR / SU_SETTING_DEFINITION_FILE;
    SettingManager->LoadItemsFromToml(slpath);
    SettingManager->RetrieveAllValues();

    SharedControlState->Initialize();
    Extensions->LoadExtensions();
    Extensions->Initialize(ScriptInterface->GetEngine());

    MixerBGM = SSoundMixer::CreateMixer(Sound.get());
    MixerSE = SSoundMixer::CreateMixer(Sound.get());
    MixerBGM->AddRef();
    MixerSE->AddRef();

    InterfacesRegisterEnum(this);
    RegisterScriptResource(this);
    RegisterScriptSprite(this);
    RegisterScriptScene(this);
    RegisterScriptSkin(this);
    RegisterCharacterSkillTypes(ScriptInterface->GetEngine());
    RegisterPlayerScene(this);
    InterfacesRegisterSceneFunction(this);
    InterfacesRegisterGlobalFunction(this);
    RegisterGlobalManagementFunction();
    Extensions->RegisterInterfaces();

    Characters->LoadAllCharacters();
    Skills->LoadAllSkills();

    /*
    hImc = ImmGetContext(GetMainWindowHandle());
    if (!ImmGetOpenStatus(hImc)) ImmSetOpenStatus(hImc, TRUE);
    ImmGetConversionStatus(hImc, &ImmConversion, &ImmSentence);
    ImmSetConversionStatus(hImc, IME_CMODE_NATIVE | IME_CMODE_FULLSHAPE, ImmSentence);
    */
}

void ExecutionManager::Shutdown()
{
    if (Skin) Skin->Terminate();
    SettingManager->SaveAllValues();
    SharedControlState->Terminate();
    MixerBGM->Release();
    MixerSE->Release();
}

void ExecutionManager::RegisterGlobalManagementFunction()
{
    auto engine = ScriptInterface->GetEngine();
    MusicSelectionCursor::RegisterScriptInterface(engine);

    engine->RegisterGlobalFunction("void ExitApplication()", asFUNCTION(InterfacesExitApplication), asCALL_CDECL);
    engine->RegisterGlobalFunction("void WriteLog(const string &in)", asMETHOD(ExecutionManager, WriteLog), asCALL_THISCALL_ASGLOBAL, this);
    engine->RegisterGlobalFunction("void Fire(const string &in)", asMETHOD(ExecutionManager, Fire), asCALL_THISCALL_ASGLOBAL, this);
    engine->RegisterGlobalFunction(SU_IF_SETTING_ITEM "@ GetSettingItem(const string &in, const string &in)", asMETHOD(ExecutionManager, GetSettingItem), asCALL_THISCALL_ASGLOBAL, this);
    engine->RegisterGlobalFunction("bool ExistsData(const string &in)", asMETHOD(ExecutionManager, ExistsData), asCALL_THISCALL_ASGLOBAL, this);
    engine->RegisterGlobalFunction("void SetData(const string &in, const bool &in)", asMETHOD(ExecutionManager, SetData<bool>), asCALL_THISCALL_ASGLOBAL, this);
    engine->RegisterGlobalFunction("void SetData(const string &in, const int &in)", asMETHOD(ExecutionManager, SetData<int>), asCALL_THISCALL_ASGLOBAL, this);
    engine->RegisterGlobalFunction("void SetData(const string &in, const double &in)", asMETHOD(ExecutionManager, SetData<double>), asCALL_THISCALL_ASGLOBAL, this);
    engine->RegisterGlobalFunction("void SetData(const string &in, const string &in)", asMETHOD(ExecutionManager, SetData<string>), asCALL_THISCALL_ASGLOBAL, this);
    engine->RegisterGlobalFunction("bool GetBoolData(const string &in)", asMETHODPR(ExecutionManager, GetData<bool>, (const string&), bool), asCALL_THISCALL_ASGLOBAL, this);
    engine->RegisterGlobalFunction("int GetIntData(const string &in)", asMETHODPR(ExecutionManager, GetData<int>, (const string&), int), asCALL_THISCALL_ASGLOBAL, this);
    engine->RegisterGlobalFunction("double GetDoubleData(const string &in)", asMETHODPR(ExecutionManager, GetData<double>, (const string&), double), asCALL_THISCALL_ASGLOBAL, this);
    engine->RegisterGlobalFunction("string GetStringData(const string &in)", asMETHODPR(ExecutionManager, GetData<string>, (const string&), string), asCALL_THISCALL_ASGLOBAL, this);

    engine->RegisterGlobalFunction(SU_IF_CHARACTER_MANAGER "@ GetCharacterManager()", asMETHOD(ExecutionManager, GetCharacterManagerUnsafe), asCALL_THISCALL_ASGLOBAL, this);
    engine->RegisterGlobalFunction(SU_IF_SKILL_MANAGER "@ GetSkillManager()", asMETHOD(ExecutionManager, GetSkillManagerUnsafe), asCALL_THISCALL_ASGLOBAL, this);
    engine->RegisterGlobalFunction("bool Execute(const string &in)", asMETHODPR(ExecutionManager, ExecuteSkin, (const string&), bool), asCALL_THISCALL_ASGLOBAL, this);
    engine->RegisterGlobalFunction("bool ExecuteScene(" SU_IF_SCENE "@)", asMETHODPR(ExecutionManager, ExecuteScene, (asIScriptObject*), bool), asCALL_THISCALL_ASGLOBAL, this);
    engine->RegisterGlobalFunction("bool ExecuteScene(" SU_IF_COSCENE "@)", asMETHODPR(ExecutionManager, ExecuteScene, (asIScriptObject*), bool), asCALL_THISCALL_ASGLOBAL, this);
    engine->RegisterGlobalFunction("void ReloadMusic()", asMETHOD(ExecutionManager, ReloadMusic), asCALL_THISCALL_ASGLOBAL, this);
    engine->RegisterGlobalFunction(SU_IF_SOUNDMIXER "@ GetDefaultMixer(const string &in)", asMETHOD(ExecutionManager, GetDefaultMixer), asCALL_THISCALL_ASGLOBAL, this);
    engine->RegisterObjectBehaviour(SU_IF_MSCURSOR, asBEHAVE_FACTORY, SU_IF_MSCURSOR "@ f()", asMETHOD(MusicsManager, CreateMusicSelectionCursor), asCALL_THISCALL_ASGLOBAL, Musics.get());
    engine->RegisterObjectBehaviour(SU_IF_SCENE_PLAYER, asBEHAVE_FACTORY, SU_IF_SCENE_PLAYER "@ f()", asMETHOD(ExecutionManager, CreatePlayer), asCALL_THISCALL_ASGLOBAL, this);
}


void ExecutionManager::EnumerateSkins()
{
    using namespace boost;
    using namespace boost::filesystem;
    using namespace boost::xpressive;
    auto log = spdlog::get("main");

    path sepath = Setting::GetRootDirectory() / SU_DATA_DIR / SU_SKIN_DIR;

    for (const auto& fdata : make_iterator_range(directory_iterator(sepath), {})) {
        if (!is_directory(fdata)) continue;
        if (!CheckSkinStructure(fdata.path())) continue;
        SkinNames.push_back(fdata.path().filename().wstring());
    }
    log->info(u8"スキン総数: {0:d}", SkinNames.size());
}

bool ExecutionManager::CheckSkinStructure(boost::filesystem::path name)
{
    using namespace boost;
    using namespace boost::filesystem;

    if (!exists(name / SU_SKIN_MAIN_FILE)) return false;
    if (!exists(name / SU_SCRIPT_DIR / SU_SKIN_TITLE_FILE)) return false;
    if (!exists(name / SU_SCRIPT_DIR / SU_SKIN_SELECT_FILE)) return false;
    if (!exists(name / SU_SCRIPT_DIR / SU_SKIN_PLAY_FILE)) return false;
    if (!exists(name / SU_SCRIPT_DIR / SU_SKIN_RESULT_FILE)) return false;
    return true;
}

void ExecutionManager::ExecuteSkin()
{
    using namespace boost::filesystem;
    auto log = spdlog::get("main");

    auto sn = SharedSetting->ReadValue<string>(SU_SETTING_GENERAL, SU_SETTING_SKIN, "Default");
    if (find(SkinNames.begin(), SkinNames.end(), ConvertUTF8ToUnicode(sn)) == SkinNames.end()) {
        log->error(u8"スキン \"{0}\"が見つかりませんでした", sn);
        return;
    }
    auto skincfg = Setting::GetRootDirectory() / SU_DATA_DIR / SU_SKIN_DIR / ConvertUTF8ToUnicode(sn) / SU_SETTING_DEFINITION_FILE;
    if (exists(skincfg)) {
        log->info("スキンの設定定義ファイルが有効です");
        SettingManager->LoadItemsFromToml(skincfg);
        SettingManager->RetrieveAllValues();
    }

    Skin = unique_ptr<SkinHolder>(new SkinHolder(ConvertUTF8ToUnicode(sn), ScriptInterface, Sound));
    Skin->Initialize();
    log->info(u8"スキン読み込み完了");
    ExecuteSkin(ConvertUnicodeToUTF8(SU_SKIN_TITLE_FILE));
}

bool ExecutionManager::ExecuteSkin(const string &file)
{
    auto log = spdlog::get("main");
    auto obj = Skin->ExecuteSkinScript(ConvertUTF8ToUnicode(file));
    if (!obj) {
        log->error(u8"スクリプトをコンパイルできませんでした");
        return false;
    }
    auto s = CreateSceneFromScriptObject(obj);
    if (!s) {
        log->error(u8"{0}にEntryPointが見つかりませんでした", file);
        return false;
    }
    AddScene(s);
    return true;
}

bool ExecutionManager::ExecuteScene(asIScriptObject *sceneObject)
{
    auto log = spdlog::get("main");
    auto s = CreateSceneFromScriptObject(sceneObject);
    if (!s) return false;
    sceneObject->SetUserData(Skin.get(), SU_UDTYPE_SKIN);
    AddScene(s);
    // カウンタ加算が余計なので1回戻す
    sceneObject->Release();
    return true;
}

void ExecutionManager::ExecuteSystemMenu()
{
    using namespace boost;
    using namespace boost::filesystem;
    auto log = spdlog::get("main");

    path sysmf = Setting::GetRootDirectory() / SU_DATA_DIR / SU_SCRIPT_DIR / SU_SYSTEM_MENU_FILE;
    if (!exists(sysmf)) {
        log->error(u8"システムメニュースクリプトが見つかりませんでした");
        return;
    }

    ScriptInterface->StartBuildModule("SystemMenu", [](auto inc, auto from, auto sb) { return true; });
    ScriptInterface->LoadFile(sysmf.wstring());
    if (!ScriptInterface->FinishBuildModule()) {
        log->error(u8"システムメニュースクリプトをコンパイルできませんでした");
        return;
    }
    auto mod = ScriptInterface->GetLastModule();

    //エントリポイント検索
    int cnt = mod->GetObjectTypeCount();
    asITypeInfo *type = nullptr;
    for (int i = 0; i < cnt; i++) {
        auto cti = mod->GetObjectTypeByIndex(i);
        if (!ScriptInterface->CheckMetaData(cti, "EntryPoint")) continue;
        type = cti;
        type->AddRef();
        break;
    }
    if (!type) {
        log->error(u8"システムメニュースクリプトにEntryPointが見つかりませんでした");
        return;
    }

    AddScene(CreateSceneFromScriptType(type));

    type->Release();
}


//Tick
void ExecutionManager::Tick(double delta)
{
    SharedControlState->Update();

    //シーン操作
    for (auto& scene : ScenesPending) Scenes.push_back(scene);
    ScenesPending.clear();
    sort(Scenes.begin(), Scenes.end(), [](shared_ptr<Scene> sa, shared_ptr<Scene> sb) { return sa->GetIndex() < sb->GetIndex(); });
    auto i = Scenes.begin();
    while (i != Scenes.end()) {
        (*i)->Tick(delta);
        if ((*i)->IsDead()) {
            i = Scenes.erase(i);
        } else {
            i++;
        }
    }

    //後処理
    static double ps = 0;
    ps += delta;
    if (ps >= 1.0) {
        ps = 0;
        MixerBGM->Update();
        MixerSE->Update();
    }
    ScriptInterface->GetEngine()->GarbageCollect(asGC_ONE_STEP);
}

//Draw
void ExecutionManager::Draw()
{
    ClearDrawScreen();
    for (const auto& s : Scenes) s->Draw();
    ScreenFlip();
}

void ExecutionManager::AddScene(shared_ptr<Scene> scene)
{
    ScenesPending.push_back(scene);
    scene->SetManager(this);
    scene->Initialize();
}

shared_ptr<ScriptScene> ExecutionManager::CreateSceneFromScriptType(asITypeInfo *type)
{
    auto log = spdlog::get("main");
    shared_ptr<ScriptScene> ret;
    if (ScriptInterface->CheckImplementation(type, SU_IF_COSCENE)) {
        auto obj = ScriptInterface->InstantiateObject(type);
        return shared_ptr<ScriptScene>(new ScriptCoroutineScene(obj));
    } else if (ScriptInterface->CheckImplementation(type, SU_IF_SCENE))  //最後
    {
        auto obj = ScriptInterface->InstantiateObject(type);
        return shared_ptr<ScriptScene>(new ScriptScene(obj));
    } else {
        log->error("{0}クラスにScene系インターフェースが実装されていません", type->GetName());
        return nullptr;
    }
}

shared_ptr<ScriptScene> ExecutionManager::CreateSceneFromScriptObject(asIScriptObject *obj)
{
    auto log = spdlog::get("main");
    shared_ptr<ScriptScene> ret;
    auto type = obj->GetObjectType();
    if (ScriptInterface->CheckImplementation(type, SU_IF_COSCENE)) {
        return shared_ptr<ScriptScene>(new ScriptCoroutineScene(obj));
    } else if (ScriptInterface->CheckImplementation(type, SU_IF_SCENE))  //最後
    {
        return shared_ptr<ScriptScene>(new ScriptScene(obj));
    } else {
        log->error("{0}クラスにScene系インターフェースが実装されていません", type->GetName());
        return nullptr;
    }
}

std::tuple<bool, LRESULT> ExecutionManager::CustomWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    ostringstream buffer;
    switch (msg) {
        /*
            //IME
        case WM_INPUTLANGCHANGE:
            WriteDebugConsole("Input Language Changed\n");
            buffer << "CharSet:" << wParam << ", Locale:" << LOWORD(lParam);
            WriteDebugConsole(buffer.str().c_str());
            return make_tuple(true, TRUE);
        case WM_IME_SETCONTEXT:
            WriteDebugConsole("Input Set Context\n");
            return make_tuple(false, 0);
        case WM_IME_STARTCOMPOSITION:
            WriteDebugConsole("Input Start Composition\n");
            return make_tuple(false, 0);
        case WM_IME_COMPOSITION:
            WriteDebugConsole("Input Conposition\n");
            return make_tuple(false, 0);
        case WM_IME_ENDCOMPOSITION:
            WriteDebugConsole("Input End Composition\n");
            return make_tuple(false, 0);
        case WM_IME_NOTIFY:
            WriteDebugConsole("Input Notify\n");
            return make_tuple(false, 0);
            */
        default:
            return make_tuple(false, (LRESULT)nullptr);
    }

}
