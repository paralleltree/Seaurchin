#include "CharacterInstance.h"
#include "Misc.h"
#include "Setting.h"

using namespace std;

CharacterInstance::CharacterInstance(shared_ptr<CharacterParameter> character, shared_ptr<SkillParameter> skill, shared_ptr<AngelScript> script)
{
    CharacterSource = character;
    SkillSource = skill;
    ScriptInterface = script;
    context = script->GetEngine()->CreateContext();
}

CharacterInstance::~CharacterInstance()
{
    context->Release();
    for (const auto &t : AbilityTypes) t->Release();
    for (const auto &o : Abilities) o->Release();
    if (ImageSet) ImageSet->Release();
}

shared_ptr<CharacterInstance> CharacterInstance::CreateInstance(shared_ptr<CharacterParameter> character, shared_ptr<SkillParameter> skill, shared_ptr<AngelScript> script)
{
    auto result = make_shared<CharacterInstance>(character, skill, script);
    result->LoadAbilities();
    result->AddRef();
    return result;
}

void CharacterInstance::SetResult(shared_ptr<Result> result)
{
    TargetResult = result;
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

void CharacterInstance::CreateImageSet()
{
    ImageSet = CharacterImageSet::CreateImageSet(CharacterSource);
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

void CharacterInstance::OnEvent(const char *name)
{
    for (int i = 0; i < Abilities.size(); ++i) {
        auto func = AbilityTypes[i]->GetMethodByDecl(name);
        if (!func) continue;
        context->Prepare(func);
        context->SetObject(Abilities[i]);
        context->SetArgAddress(0, TargetResult.get());
        context->Execute();
    }
}

void CharacterInstance::OnEvent(const char *name, AbilityNoteType type)
{
    for (int i = 0; i < Abilities.size(); ++i) {
        auto func = AbilityTypes[i]->GetMethodByDecl(name);
        if (!func) continue;
        context->Prepare(func);
        context->SetObject(Abilities[i]);
        context->SetArgAddress(0, TargetResult.get());
        context->SetArgDWord(1, (asDWORD)type);
        context->Execute();
    }
}

void CharacterInstance::OnStart()
{
    OnEvent("void OnStart(" SU_IF_RESULT "@)");
}

void CharacterInstance::OnFinish()
{
    OnEvent("void OnFinish(" SU_IF_RESULT "@)");
}

void CharacterInstance::OnJusticeCritical(AbilityNoteType type)
{
    OnEvent("void OnJusticeCritical(" SU_IF_RESULT "@, " SU_IF_NOTETYPE ")", type);
}

void CharacterInstance::OnJustice(AbilityNoteType type)
{
    OnEvent("void OnJustice(" SU_IF_RESULT "@, " SU_IF_NOTETYPE ")", type);
}

void CharacterInstance::OnAttack(AbilityNoteType type)
{
    OnEvent("void OnAttack(" SU_IF_RESULT "@, " SU_IF_NOTETYPE ")", type);
}

void CharacterInstance::OnMiss(AbilityNoteType type)
{
    OnEvent("void OnMiss(" SU_IF_RESULT "@, " SU_IF_NOTETYPE ")", type);
}

CharacterParameter* CharacterInstance::GetCharacterParameter()
{
    return CharacterSource.get();
}

SkillParameter* CharacterInstance::GetSkillParameter()
{
    return SkillSource.get();
}

CharacterImageSet* CharacterInstance::GetCharacterImages()
{
    ImageSet->AddRef();
    return ImageSet;
}

void RegisterCharacterSkillTypes(asIScriptEngine *engine)
{
    RegisterResultTypes(engine);
    RegisterSkillTypes(engine);
    RegisterCharacterTypes(engine);

    engine->RegisterObjectType(SU_IF_CHARACTER_INSTANCE, 0, asOBJ_REF);
    engine->RegisterObjectBehaviour(SU_IF_CHARACTER_INSTANCE, asBEHAVE_ADDREF, "void f()", asMETHOD(CharacterInstance, AddRef), asCALL_THISCALL);
    engine->RegisterObjectBehaviour(SU_IF_CHARACTER_INSTANCE, asBEHAVE_RELEASE, "void f()", asMETHOD(CharacterInstance, Release), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_CHARACTER_INSTANCE, SU_IF_CHARACTER_PARAM "@ GetCharacter()", asMETHOD(CharacterInstance, GetCharacterParameter), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_CHARACTER_INSTANCE, SU_IF_SKILL "@ GetSkill()", asMETHOD(CharacterInstance, GetCharacterParameter), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_CHARACTER_INSTANCE, SU_IF_CHARACTER_IMAGES "@ GetCharacterImages()", asMETHOD(CharacterInstance, GetCharacterParameter), asCALL_THISCALL);
}