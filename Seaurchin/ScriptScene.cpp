#include "ScriptScene.h"

#include "Config.h"
#include "ExecutionManager.h"
#include "Misc.h"

using namespace std;
using namespace boost::filesystem;

ScriptScene::ScriptScene(asIScriptObject *scene)
{
    sceneObject = scene;
    sceneObject->AddRef();

    sceneType = sceneObject->GetObjectType();
    sceneType->AddRef();

    auto eng = sceneObject->GetEngine();
    context = eng->CreateContext();
    context->SetUserData(this, SU_UDTYPE_SCENE);
}

ScriptScene::~ScriptScene()
{
    for (auto &i : sprites) i->Release();
    context->Release();
    sceneType->Release();
    sceneObject->Release();
}

void ScriptScene::Initialize()
{
    const auto func = sceneType->GetMethodByDecl("void Initialize()");
    context->Prepare(func);
    context->SetObject(sceneObject);
    context->Execute();
}

void ScriptScene::AddSprite(SSprite *sprite)
{
    //sprite->AddRef();
    spritesPending.push_back(sprite);
}

void ScriptScene::AddCoroutine(Coroutine * co)
{
    coroutinesPending.push_back(co);
}

void ScriptScene::Tick(const double delta)
{
    TickSprite(delta);
    TickCoroutine(delta);
    const auto func = sceneType->GetMethodByDecl("void Tick(double)");
    context->Prepare(func);
    context->SetObject(sceneObject);
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
    evc->Release();
}

void ScriptScene::Draw()
{
    DrawSprite();
    const auto func = sceneType->GetMethodByDecl("void Draw()");
    context->Prepare(func);
    context->SetObject(sceneObject);
    context->Execute();
}

bool ScriptScene::IsDead()
{
    return finished;
}

void ScriptScene::Disappear()
{
    finished = true;
}

void ScriptScene::TickCoroutine(const double delta)
{
    for (auto& coroutine : coroutinesPending) {
        coroutine->Context = context->GetEngine()->CreateContext();
        coroutine->Context->SetUserData(this, SU_UDTYPE_SCENE);
        coroutine->Context->SetUserData(&coroutine->Wait, SU_UDTYPE_WAIT);
        coroutine->Context->Prepare(coroutine->Function);
        coroutine->Context->SetObject(coroutine->Object);
        coroutines.push_back(coroutine);
    }
    coroutinesPending.clear();

    auto i = coroutines.begin();
    while (i != coroutines.end()) {
        auto c = *i;
        switch (c->Wait.Type) {
            case WaitType::Frame:
                c->Wait.frames -= 1;
                if (c->Wait.frames > 0) {
                    ++i;
                    continue;
                }
                break;
            case WaitType::Time:
                c->Wait.time -= delta;
                if (c->Wait.time > 0.0) {
                    ++i;
                    continue;
                }
                break;
            default:
                spdlog::get("main")->critical(u8"コルーチンのWaitステータスが不正です");
                abort();
        }
        const auto result = c->Context->Execute();
        if (result == asEXECUTION_FINISHED) {
            auto e = c->Context->GetEngine();
            c->Context->Release();
            c->Function->Release();
            e->ReleaseScriptObject(c->Object, c->Type);
            delete c;
            i = coroutines.erase(i);
        } else if (result == asEXECUTION_EXCEPTION) {
            auto log = spdlog::get("main");
            int col;
            const char *at;
            const auto row = c->Context->GetExceptionLineNumber(&col, &at);
            ostringstream str;
            log->error(u8"{0} ({1:d}行{2:d}列): {3}", at, row, col, c->Context->GetExceptionString());
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

ScriptCoroutineScene::ScriptCoroutineScene(asIScriptObject *scene) : Base(scene)
{
    auto eng = sceneObject->GetEngine();
    runningContext = eng->CreateContext();
    runningContext->SetUserData(this, SU_UDTYPE_SCENE);
    runningContext->SetUserData(&wait, SU_UDTYPE_WAIT);
    wait.Type = WaitType::Time;
    wait.time = 0.0;
}

ScriptCoroutineScene::~ScriptCoroutineScene()
{
    runningContext->Release();
    for (auto& i : coroutines) {
        auto e = i->Context->GetEngine();
        i->Context->Release();
        i->Function->Release();
        e->ReleaseScriptObject(i->Object, i->Type);
        delete i;
    }
    coroutines.clear();
}

void ScriptCoroutineScene::Tick(const double delta)
{
    TickSprite(delta);
    TickCoroutine(delta);
    //Run()
    switch (wait.Type) {
        case WaitType::Frame:
            wait.frames -= 1;
            if (wait.frames > 0) return;
            break;
        case WaitType::Time:
            wait.time -= delta;
            if (wait.time > 0.0) return;
            break;
    }
    const auto result = runningContext->Execute();
    if (result != asEXECUTION_SUSPENDED)
        finished = true;
}

void ScriptCoroutineScene::Initialize()
{
    Base::Initialize();
    const auto func = sceneType->GetMethodByDecl("void Run()");
    runningContext->Prepare(func);
    runningContext->SetObject(sceneObject);
}

void RegisterScriptScene(ExecutionManager *exm)
{
    auto engine = exm->GetScriptInterfaceUnsafe()->GetEngine();

    engine->RegisterFuncdef("void " SU_IF_COROUTINE "()");

    engine->RegisterInterface(SU_IF_SCENE);
    engine->RegisterInterfaceMethod(SU_IF_SCENE, "void Initialize()");
    engine->RegisterInterfaceMethod(SU_IF_SCENE, "void Tick(double)");
    engine->RegisterInterfaceMethod(SU_IF_SCENE, "void Draw()");

    engine->RegisterInterface(SU_IF_COSCENE);
    engine->RegisterInterfaceMethod(SU_IF_COSCENE, "void Initialize()");
    engine->RegisterInterfaceMethod(SU_IF_COSCENE, "void Run()");
    engine->RegisterInterfaceMethod(SU_IF_COSCENE, "void Draw()");

    engine->RegisterGlobalFunction("int GetIndex()", asFUNCTION(ScriptSceneGetIndex), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetIndex(int)", asFUNCTION(ScriptSceneSetIndex), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsKeyHeld(int)", asFUNCTION(ScriptSceneIsKeyHeld), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsKeyTriggered(int)", asFUNCTION(ScriptSceneIsKeyTriggered), asCALL_CDECL);
    engine->RegisterGlobalFunction("void RunCoroutine(" SU_IF_COROUTINE "@, const string &in)", asFUNCTION(ScriptSceneRunCoroutine), asCALL_CDECL);
    engine->RegisterGlobalFunction("void KillCoroutine(const string &in)", asFUNCTION(ScriptSceneKillCoroutine), asCALL_CDECL);
    engine->RegisterGlobalFunction("void Disappear()", asFUNCTION(ScriptSceneDisappear), asCALL_CDECL);
    engine->RegisterGlobalFunction("void AddSprite(" SU_IF_SPRITE "@)", asFUNCTION(ScriptSceneAddSprite), asCALL_CDECL);
    engine->RegisterGlobalFunction("void AddScene(" SU_IF_SCENE "@)", asFUNCTION(ScriptSceneAddScene), asCALL_CDECL);
    engine->RegisterGlobalFunction("void AddScene(" SU_IF_COSCENE "@)", asFUNCTION(ScriptSceneAddScene), asCALL_CDECL);
}

void ScriptSceneSetIndex(const int index)
{
    const auto ctx = asGetActiveContext();
    const auto psc = static_cast<ScriptScene*>(ctx->GetUserData(SU_UDTYPE_SCENE));
    if (!psc) {
        ScriptSceneWarnOutOf("Scene Class", ctx);
        return;
    }
    psc->SetIndex(index);
}

int ScriptSceneGetIndex()
{
    const auto ctx = asGetActiveContext();
    const auto psc = static_cast<ScriptScene*>(ctx->GetUserData(SU_UDTYPE_SCENE));
    if (!psc) {
        ScriptSceneWarnOutOf("Scene Class", ctx);
        return 0;
    }
    return psc->GetIndex();
}

// Scene用メソッド

bool ScriptSceneIsKeyHeld(const int keynum)
{
    const auto ctx = asGetActiveContext();
    const auto psc = static_cast<ScriptScene*>(ctx->GetUserData(SU_UDTYPE_SCENE));
    if (!psc) {
        ScriptSceneWarnOutOf("Scene Class", ctx);
        return false;
    }
    return psc->GetManager()->GetControlStateUnsafe()->GetCurrentState(ControllerSource::RawKeyboard, keynum);
}

bool ScriptSceneIsKeyTriggered(const int keynum)
{
    const auto ctx = asGetActiveContext();
    const auto psc = static_cast<ScriptScene*>(ctx->GetUserData(SU_UDTYPE_SCENE));
    if (!psc) {
        ScriptSceneWarnOutOf("Scene Class", ctx);
        return false;
    }
    return  psc->GetManager()->GetControlStateUnsafe()->GetTriggerState(ControllerSource::RawKeyboard, keynum);
}

void ScriptSceneAddScene(asIScriptObject *sceneObject)
{
    const auto ctx = asGetActiveContext();
    const auto psc = static_cast<ScriptScene*>(ctx->GetUserData(SU_UDTYPE_SCENE));
    if (!psc) {
        ScriptSceneWarnOutOf("Scene Class", ctx);
        return;
    }
    psc->GetManager()->CreateSceneFromScriptObject(sceneObject);
}

void ScriptSceneAddSprite(SSprite * sprite)
{
    const auto ctx = asGetActiveContext();
    auto psc = static_cast<ScriptCoroutineScene*>(ctx->GetUserData(SU_UDTYPE_SCENE));
    if (!psc) {
        ScriptSceneWarnOutOf("Scene Class", ctx);
        return;
    }
    psc->AddSprite(sprite);
}

void ScriptSceneRunCoroutine(asIScriptFunction *cofunc, const string &name)
{
    const auto ctx = asGetActiveContext();
    auto psc = static_cast<ScriptCoroutineScene*>(ctx->GetUserData(SU_UDTYPE_SCENE));
    if (!psc) {
        ScriptSceneWarnOutOf("Scene Class", ctx);
        return;
    }
    if (!cofunc || cofunc->GetFuncType() != asFUNC_DELEGATE) return;
    auto *c = new Coroutine;
    c->Name = name;
    c->Function = cofunc->GetDelegateFunction();
    c->Function->AddRef();
    c->Object = cofunc->GetDelegateObject();
    c->Type = cofunc->GetDelegateObjectType();
    c->Wait.Type = WaitType::Time;
    c->Wait.time = 0;
    psc->AddCoroutine(c);
}

void ScriptSceneKillCoroutine(const std::string &name)
{
    const auto ctx = asGetActiveContext();
    auto psc = static_cast<ScriptCoroutineScene*>(ctx->GetUserData(SU_UDTYPE_SCENE));
    if (!psc) {
        ScriptSceneWarnOutOf("Scene Class", ctx);
        return;
    }
    if (name.empty()) {
        for (auto& i : psc->coroutines) {
            auto e = i->Context->GetEngine();
            i->Context->Release();
            i->Function->Release();
            e->ReleaseScriptObject(i->Object, i->Type);
            delete i;
        }
        psc->coroutines.clear();
    } else {
        auto i = psc->coroutines.begin();
        while (i != psc->coroutines.end()) {
            const auto c = *i;
            if (c->Name == name) {
                auto e = c->Context->GetEngine();
                c->Context->Release();
                c->Function->Release();
                e->ReleaseScriptObject(c->Object, c->Type);
                delete c;
                i = psc->coroutines.erase(i);
            } else {
                ++i;
            }
        }
    }
}

void ScriptSceneDisappear()
{
    const auto ctx = asGetActiveContext();
    auto psc = static_cast<ScriptCoroutineScene*>(ctx->GetUserData(SU_UDTYPE_SCENE));
    if (!psc) {
        ScriptSceneWarnOutOf("Scene Class", ctx);
        return;
    }
    psc->Disappear();
}
