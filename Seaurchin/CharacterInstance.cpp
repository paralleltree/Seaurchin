#include "CharacterInstance.h"
#include "Misc.h"
#include "Setting.h"

using namespace std;

CharacterInstance::CharacterInstance(shared_ptr<CharacterParameter> character, shared_ptr<SkillParameter> skill, shared_ptr<AngelScript> script)
{
    CharacterSource = character;
    SkillSource = skill;
    ScriptInterface = script;
}

CharacterInstance::~CharacterInstance()
{
    context->Release();
    for (const auto &t : AbilityTypes) t->Release();
    for (const auto &o : Abilities) o->Release();
    if (ImageFull) ImageFull->Release();
    if (ImageSmall) ImageSmall->Release();
    if (ImageFace) ImageFace->Release();

}

void CharacterInstance::MakeCharacterImages()
{
    using namespace boost::filesystem;
    // 内部的にはRenderTarget
    path imgroot = Setting::GetRootDirectory() / SU_SKILL_DIR / SU_CHARACTER_DIR;
    auto root = (imgroot / ConvertUTF8ToUnicode(CharacterSource->ImagePath)).wstring();
    int hBase = LoadGraph(reinterpret_cast<const char*>(root.c_str()));
    int hSmall = MakeGraph(SU_CHAR_SMALL_WIDTH, SU_CHAR_SMALL_WIDTH);
    int hFace = MakeGraph(SU_CHAR_FACE_SIZE, SU_CHAR_FACE_SIZE);
    BEGIN_DRAW_TRANSACTION(hSmall);
    DrawRectExtendGraph(
        0, 0, SU_CHAR_SMALL_WIDTH, SU_CHAR_SMALL_HEIGHT,
        CharacterSource->Metric.SmallRange[0], CharacterSource->Metric.SmallRange[1],
        CharacterSource->Metric.SmallRange[2], CharacterSource->Metric.SmallRange[3],
        hBase, TRUE);
    BEGIN_DRAW_TRANSACTION(hFace);
    DrawRectExtendGraph(
        0, 0, SU_CHAR_FACE_SIZE, SU_CHAR_FACE_SIZE,
        CharacterSource->Metric.FaceRange[0], CharacterSource->Metric.FaceRange[1],
        CharacterSource->Metric.FaceRange[2], CharacterSource->Metric.FaceRange[3],
        hBase, TRUE);
    FINISH_DRAW_TRANSACTION;
    ImageFull = new SImage(hBase);
    ImageFull->AddRef();
    ImageSmall = new SImage(hSmall);
    ImageSmall->AddRef();
    ImageFace = new SImage(hFace);
    ImageFace->AddRef();
}

void CharacterInstance::LoadAbilities()
{
    using namespace boost::algorithm;
    using namespace boost::filesystem;
    auto log = spdlog::get("main");
    auto abroot = Setting::GetRootDirectory() / SU_SKILL_DIR / SU_ABILITY_DIR;

    for (const auto &def : SkillSource->Abilities) {
        vector<string> params;
        auto scrpath = abroot / (def.Name + ".as");

        auto abo = LoadAbilityObject(scrpath);
        if (!abo) continue;
        auto abt = abo->GetObjectType();
        abt->AddRef();
        Abilities.push_back(abo);
        AbilityTypes.push_back(abt);

        auto init = abt->GetMethodByDecl("void Initialize(dictionary@)");
        if (!init) continue;

        auto args = CScriptDictionary::Create(ScriptInterface->GetEngine());
        for (const auto &arg : def.Arguments) {
            auto key = arg.first;
            auto value = arg.second;
            auto &vid = value.type();
            if (vid == typeid(int)) {
                asINT64 avalue = boost::any_cast<int>(value);
                args->Set(key, avalue);
            } else if (vid == typeid(double)) {
                double avalue = boost::any_cast<double>(value);
                args->Set(key, avalue);
            } else if (vid == typeid(string)) {
                string avalue = boost::any_cast<string>(value);
                args->Set(key, &avalue, ScriptInterface->GetEngine()->GetTypeIdByDecl("string"));
            }
        }

        context->Prepare(init);
        context->SetObject(abo);
        context->SetArgAddress(0, args);
        context->Execute();
        log->info(u8"アビリティー " + ConvertUnicodeToUTF8(scrpath.c_str()));
    }

}

asIScriptObject* CharacterInstance::LoadAbilityObject(boost::filesystem::path filepath)
{
    using namespace boost::filesystem;
    auto log = spdlog::get("main");
    auto abroot = Setting::GetRootDirectory() / SU_SKILL_DIR / SU_ABILITY_DIR;
    //お茶を濁せ
    auto modulename = ConvertUnicodeToUTF8(filepath.c_str());
    auto mod = ScriptInterface->GetExistModule(modulename);
    if (!mod) {
        ScriptInterface->StartBuildModule(modulename.c_str(), [=](wstring inc, wstring from, CWScriptBuilder *b) {
            if (!exists(abroot / inc)) return false;
            b->AddSectionFromFile((abroot / inc).wstring().c_str());
            return true;
        });
        ScriptInterface->LoadFile(filepath.wstring().c_str());
        if (!ScriptInterface->FinishBuildModule()) {
            ScriptInterface->GetLastModule()->Discard();
            return nullptr;
        }
        mod = ScriptInterface->GetLastModule();
    }

    //エントリポイント検索
    int cnt = mod->GetObjectTypeCount();
    asITypeInfo *type = nullptr;
    for (int i = 0; i < cnt; i++) {
        auto cti = mod->GetObjectTypeByIndex(i);
        if (!(ScriptInterface->CheckMetaData(cti, "EntryPoint") || cti->GetUserData(SU_UDTYPE_ENTRYPOINT))) continue;
        type = cti;
        type->SetUserData((void*)0xFFFFFFFF, SU_UDTYPE_ENTRYPOINT);
        type->AddRef();
        break;
    }
    if (!type) {
        log->critical(u8"アビリティーにEntryPointがありません");
        return nullptr;
    }

    auto obj = ScriptInterface->InstantiateObject(type);
    obj->AddRef();
    type->Release();
    return obj;
}

shared_ptr<CharacterInstance> CharacterInstance::CreateInstance(shared_ptr<CharacterParameter> character, shared_ptr<SkillParameter> skill, shared_ptr<AngelScript> script)
{
    auto result = make_shared<CharacterInstance>(character, skill);
    result->MakeCharacterImages();
    result->LoadAbilities();
    return result;
}

