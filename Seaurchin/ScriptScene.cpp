#include "ScriptScene.h"

#include "Config.h"
#include "ExecutionManager.h"
#include "Misc.h"

using namespace std;
using namespace boost::filesystem;

ScriptScene::ScriptScene(asIScriptObject *scene)
    : sceneObject(scene)
    , sceneType(scene->GetObjectType())
    , context(scene->GetEngine()->CreateContext())
{
    sceneObject->AddRef();
    sceneType->AddRef();
    context->SetUserData(this, SU_UDTYPE_SCENE);
}

ScriptScene::~ScriptScene()
{
    for (auto &i : sprites) i->Release();
    sprites.clear();
    for (auto &i : spritesPending) i->Release();
    spritesPending.clear();

    KillCoroutine("");

    context->Release();
    sceneType->Release();
    sceneObject->Release();
}

asIScriptFunction* ScriptScene::GetMainMethod()
{
    return sceneType->GetMethodByDecl("void Tick(double)");
}

void ScriptScene::Initialize()
{
    {
        const auto func = sceneType->GetMethodByDecl("void Initialize()");
        context->Prepare(func);
        context->SetObject(sceneObject);
        context->Execute();
    }

    {
        const auto func = this->GetMainMethod();
        context->Prepare(func);
        context->SetObject(sceneObject);
    }
}

void ScriptScene::AddSprite(SSprite *sprite)
{
    spritesPending.push_back(sprite);
}

void ScriptScene::AddCoroutine(Coroutine * co)
{
    coroutinesPending.push_back(co);
}

void ScriptScene::KillCoroutine(const string &name)
{
    if (name.empty()) {
        for (auto& i : coroutinesPending) delete i;
        coroutinesPending.clear();
        for (auto& i : coroutines) delete i;
        coroutines.clear();
    } else {
        auto i = coroutinesPending.begin();
        while (i != coroutinesPending.end()) {
            const auto c = *i;
            if (c->Name == name) {
                delete c;
                i = coroutinesPending.erase(i);
            } else {
                ++i;
            }
        }
        i = coroutines.begin();
        while (i != coroutines.end()) {
            const auto c = *i;
            if (c->Name == name) {
                delete c;
                i = coroutines.erase(i);
            } else {
                ++i;
            }
        }
    }
}

void ScriptScene::Tick(const double delta)
{
    TickSprite(delta);
    TickCoroutine(delta);
    context->SetArgDouble(0, delta);
    context->Execute();
}

void ScriptScene::OnEvent(const string &message)
{
    auto msg = message;
    const auto func = sceneType->GetMethodByDecl("void OnEvent(const string &in)");
    if (!func) return;
    auto evc = manager->GetScriptInterfaceUnsafe()->GetEngine()->CreateContext();
    evc->Prepare(func);
    evc->SetUserData(this, SU_UDTYPE_SCENE);
    evc->SetObject(sceneObject);
    evc->SetArgAddress(0, static_cast<void*>(&msg));
    evc->Execute();
    evc->Unprepare();
    evc->Release();
}

void ScriptScene::Draw()
{
    DrawSprite();
}

bool ScriptScene::IsDead()
{
    return finished;
}

void ScriptScene::Disappear()
{
    finished = true;
}

void ScriptScene::Dispose()
{
    for (auto it = callbacks.begin(); it != callbacks.end(); ++it)         {
        (*it)->Dispose();
    }
    callbacks.clear();
}

void ScriptScene::RegistDisposalCallback(CallbackObject *callback)
{
    callbacks.push_back(callback);
}

void ScriptScene::TickCoroutine(const double delta)
{
    for (auto& coroutine : coroutinesPending) {
        coroutine->SetSceneInstance(this);
        coroutines.push_back(coroutine);
    }
    coroutinesPending.clear();

    auto i = coroutines.begin();
    while (i != coroutines.end()) {
        auto c = *i;

        if (c->Wait.Tick(delta)) {
            ++i;
            continue;
        }

        const auto result = c->Execute();
        if (result == asEXECUTION_FINISHED) {
            delete c;
            i = coroutines.erase(i);
        } else if (result == asEXECUTION_EXCEPTION) {
            auto log = spdlog::get("main");
            int col;
            const char *at;
            const auto row = c->GetContext()->GetExceptionLineNumber(&col, &at);
            log->error(u8"{0} ({1:d}行{2:d}列): {3}", at, row, col, c->GetContext()->GetExceptionString());
            abort();
        } else {
            ++i;
        }
    }
}

void ScriptScene::TickSprite(const double delta)
{
    for (auto& sprite : spritesPending) sprites.emplace(sprite);
    spritesPending.clear();
    auto i = sprites.begin();
    while (i != sprites.end()) {
        (*i)->Tick(delta);
        if ((*i)->IsDead) {
            (*i)->Release();
            i = sprites.erase(i);
        } else {
            ++i;
        }
    }
}

void ScriptScene::DrawSprite()
{
    for (auto& i : sprites) i->Draw();
}

ScriptCoroutineScene::ScriptCoroutineScene(asIScriptObject *scene)
    : Base(scene)
    , wait(CoroutineWait{ WaitType::Time, 0 })
{
    context->SetUserData(&wait, SU_UDTYPE_WAIT);
}

ScriptCoroutineScene::~ScriptCoroutineScene()
{
}

asIScriptFunction* ScriptCoroutineScene::GetMainMethod()
{
    return sceneType->GetMethodByDecl("void Run()");
}

void ScriptCoroutineScene::Tick(const double delta)
{
    TickSprite(delta);
    TickCoroutine(delta);

    //Run()
    if (wait.Tick(delta)) return;

    const auto result = context->Execute();
    if (result != asEXECUTION_SUSPENDED)
        finished = true;
}

void RegisterScriptScene(ExecutionManager *exm)
{
    auto engine = exm->GetScriptInterfaceUnsafe()->GetEngine();

    engine->RegisterFuncdef("void " SU_IF_COROUTINE "()");

    engine->RegisterInterface(SU_IF_SCENE);
    engine->RegisterInterfaceMethod(SU_IF_SCENE, "void Initialize()");
    engine->RegisterInterfaceMethod(SU_IF_SCENE, "void Tick(double)");

    engine->RegisterInterface(SU_IF_COSCENE);
    engine->RegisterInterfaceMethod(SU_IF_COSCENE, "void Initialize()");
    engine->RegisterInterfaceMethod(SU_IF_COSCENE, "void Run()");

    engine->RegisterGlobalFunction("int GetIndex()", asFUNCTION(ScriptSceneGetIndex), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetIndex(int)", asFUNCTION(ScriptSceneSetIndex), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsKeyHeld(int)", asFUNCTION(ScriptSceneIsKeyHeld), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsKeyTriggered(int)", asFUNCTION(ScriptSceneIsKeyTriggered), asCALL_CDECL);
    engine->RegisterGlobalFunction("void RunCoroutine(" SU_IF_COROUTINE "@, const string &in)", asFUNCTION(ScriptSceneRunCoroutine), asCALL_CDECL);
    engine->RegisterGlobalFunction("void KillCoroutine(const string &in)", asFUNCTION(ScriptSceneKillCoroutine), asCALL_CDECL);
    engine->RegisterGlobalFunction("void Disappear()", asFUNCTION(ScriptSceneDisappear), asCALL_CDECL);
    engine->RegisterGlobalFunction("void AddSprite(" SU_IF_SPRITE "@)", asFUNCTION(ScriptSceneAddSprite), asCALL_CDECL);
}

// Scene用メソッド

void ScriptSceneSetIndex(const int index)
{
    const auto ctx = asGetActiveContext();
    const auto psc = static_cast<ScriptScene*>(ctx->GetUserData(SU_UDTYPE_SCENE));
    if (!psc) {
        ScriptSceneWarnOutOf("SetIndex", "Scene Class", ctx);
        return;
    }
    psc->SetIndex(index);
}

int ScriptSceneGetIndex()
{
    const auto ctx = asGetActiveContext();
    const auto psc = static_cast<ScriptScene*>(ctx->GetUserData(SU_UDTYPE_SCENE));
    if (!psc) {
        ScriptSceneWarnOutOf("GetIndex", "Scene Class", ctx);
        return 0;
    }
    return psc->GetIndex();
}

bool ScriptSceneIsKeyHeld(const int keynum)
{
    const auto ctx = asGetActiveContext();
    const auto psc = static_cast<ScriptScene*>(ctx->GetUserData(SU_UDTYPE_SCENE));
    if (!psc) {
        ScriptSceneWarnOutOf("IsKeyHeld", "Scene Class", ctx);
        return false;
    }
    return psc->GetManager()->GetControlStateUnsafe()->GetCurrentState(ControllerSource::RawKeyboard, keynum);
}

bool ScriptSceneIsKeyTriggered(const int keynum)
{
    const auto ctx = asGetActiveContext();
    const auto psc = static_cast<ScriptScene*>(ctx->GetUserData(SU_UDTYPE_SCENE));
    if (!psc) {
        ScriptSceneWarnOutOf("IsKeyTriggered", "Scene Class", ctx);
        return false;
    }
    return  psc->GetManager()->GetControlStateUnsafe()->GetTriggerState(ControllerSource::RawKeyboard, keynum);
}

void ScriptSceneAddSprite(SSprite * sprite)
{
    if (!sprite) return;

    const auto ctx = asGetActiveContext();
    auto psc = static_cast<ScriptCoroutineScene*>(ctx->GetUserData(SU_UDTYPE_SCENE));
    if (!psc) {
        ScriptSceneWarnOutOf("AddSprite", "Scene Class", ctx);
        return;
    }
    {
        sprite->AddRef();
        psc->AddSprite(sprite);
    }

    sprite->Release();
}

void ScriptSceneRunCoroutine(asIScriptFunction *cofunc, const string &name)
{
    if (!cofunc) return;

    auto const ctx = asGetActiveContext();
    auto const psc = static_cast<ScriptScene*>(ctx->GetUserData(SU_UDTYPE_SCENE));
    if (!psc) {
        ScriptSceneWarnOutOf("RunCoroutine", "Scene Class", ctx);
        return;
    }

    if (cofunc->GetFuncType() != asFUNC_DELEGATE) {
        // TODO: エラー丁寧にだすべき
        ScriptSceneWarnOutOf("RunCoroutine", "Scene Class", ctx);
    } else {
        cofunc->AddRef();
        psc->AddCoroutine(new Coroutine(name, cofunc, ctx->GetEngine()));
    }

    cofunc->Release();
}

void ScriptSceneKillCoroutine(const std::string &name)
{
    const auto ctx = asGetActiveContext();
    auto psc = static_cast<ScriptScene*>(ctx->GetUserData(SU_UDTYPE_SCENE));
    if (!psc) {
        ScriptSceneWarnOutOf("KillCoroutine", "Scene Class", ctx);
        return;
    }
    psc->KillCoroutine(name);
}

void ScriptSceneDisappear()
{
    const auto ctx = asGetActiveContext();
    auto psc = static_cast<ScriptCoroutineScene*>(ctx->GetUserData(SU_UDTYPE_SCENE));
    if (!psc) {
        ScriptSceneWarnOutOf("Disappear", "Scene Class", ctx);
        return;
    }
    psc->Disappear();
}


Coroutine::Coroutine(const std::string &name, const asIScriptFunction* cofunc, asIScriptEngine* engine)
    : Name(name)
    , Wait(CoroutineWait { WaitType::Time, 0 })
{
    BOOST_ASSERT(cofunc);
    BOOST_ASSERT(cofunc->GetFuncType() == asFUNC_DELEGATE);

    function = cofunc->GetDelegateFunction();
    function->AddRef();

    object = cofunc->GetDelegateObject();
    static_cast<asIScriptObject*>(object)->AddRef();

    type = cofunc->GetDelegateObjectType();
    type->AddRef();

    context = engine->CreateContext();
    context->SetUserData(&Wait, SU_UDTYPE_WAIT);
    context->Prepare(function);
    context->SetObject(object);

    cofunc->Release();
}

Coroutine::~Coroutine()
{
    auto e = context->GetEngine();
    context->Unprepare();
    context->Release();
    function->Release();
    e->ReleaseScriptObject(object, type);
    type->Release();
}
