#include "ScriptScene.h"

#include "Config.h"
#include "ExecutionManager.h"
#include "Misc.h"

using namespace std;
using namespace boost::filesystem;

ScriptScene::ScriptScene(asIScriptObject *scene)
    : sceneObject(scene)
    , initMethod(nullptr)
    , mainMethod(nullptr)
    , eventMethod(nullptr)
    , finished(false)
{
    sceneObject->AddRef();
}

ScriptScene::~ScriptScene()
{
    for (auto &i : sprites) i->Release();
    sprites.clear();
    for (auto &i : spritesPending) i->Release();
    spritesPending.clear();

    KillCoroutine("");

    delete initMethod;
    delete mainMethod;
    delete eventMethod;

    sceneObject->Release();
}

void ScriptScene::Initialize()
{
    const auto sceneType = sceneObject->GetObjectType();
    auto engine = sceneObject->GetEngine();

    {
        auto func = sceneType->GetMethodByDecl("void Initialize()");
        if (func) {
            sceneObject->AddRef();
            func->AddRef();
            initMethod = new MethodObject(engine, sceneObject, func);
            initMethod->Context->SetUserData(this, SU_UDTYPE_SCENE);
        }
    }

    {
        auto func = sceneType->GetMethodByDecl(this->GetMainMethodDecl());
        if (func) {
            sceneObject->AddRef();
            func->AddRef();
            mainMethod = new MethodObject(engine, sceneObject, func);
            mainMethod->Context->SetUserData(this, SU_UDTYPE_SCENE);
        }
    }

    {
        auto func = sceneType->GetMethodByDecl("void OnEvent(const string &in)");
        if (func) {
            sceneObject->AddRef();
            func->AddRef();
            eventMethod = new MethodObject(engine, sceneObject, func);
            eventMethod->Context->SetUserData(this, SU_UDTYPE_SCENE);
        }
    }

    if (!initMethod) return;

    initMethod->Context->Prepare(initMethod->Function);
    initMethod->Context->SetObject(initMethod->Object);
    initMethod->Context->Execute();
    initMethod->Context->Unprepare();
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

    if (!mainMethod) return;

    mainMethod->Context->Prepare(mainMethod->Function);
    mainMethod->Context->SetObject(mainMethod->Object);
    mainMethod->Context->SetArgDouble(0, delta);
    mainMethod->Context->Execute();
    mainMethod->Context->Unprepare();
}

void ScriptScene::OnEvent(const string &message)
{
    if (!eventMethod) return;

    auto msg = message;
    eventMethod->Context->Prepare(eventMethod->Function);
    eventMethod->Context->SetObject(eventMethod->Object);
    eventMethod->Context->SetArgAddress(0, static_cast<void*>(&msg));
    eventMethod->Context->Execute();
    eventMethod->Context->Unprepare();
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
        (*it)->Release();
    }
    callbacks.clear();
}

void ScriptScene::RegistDisposalCallback(CallbackObject *callback)
{
    callbacks.push_back(callback);
}

void ScriptScene::TickCoroutine(const double delta)
{
    if (!coroutinesPending.empty()) {
        for (auto& coroutine : coroutinesPending) {
            coroutine->SetSceneInstance(this);
            coroutines.push_back(coroutine);
        }
        coroutinesPending.clear();
    }

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
    if (!spritesPending.empty()) {
        for (auto& sprite : spritesPending) sprites.emplace(sprite);
        spritesPending.clear();
    }

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
{}

ScriptCoroutineScene::~ScriptCoroutineScene()
{
    mainMethod->Context->Unprepare();
}

void ScriptCoroutineScene::Initialize()
{
    Base::Initialize();

    mainMethod->Context->SetUserData(&wait, SU_UDTYPE_WAIT);
    mainMethod->Context->Prepare(mainMethod->Function);
    mainMethod->Context->SetObject(mainMethod->Object);
}

void ScriptCoroutineScene::Tick(const double delta)
{
    TickSprite(delta);
    TickCoroutine(delta);

    //Run()
    if (wait.Tick(delta)) return;

    if (!mainMethod) {
        finished = true;
        return;
    }

    const auto result = mainMethod->Context->Execute();
    if (result != asEXECUTION_SUSPENDED) {
        mainMethod->Context->Unprepare();
        finished = true;
    }
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
