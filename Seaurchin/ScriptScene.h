#pragma once

#include "Config.h"
#include "Scene.h"
#include "ScriptSprite.h"
#include "ScriptFunction.h"

#define SU_IF_SCENE "Scene"
#define SU_IF_COSCENE "CoroutineScene"
#define SU_IF_COROUTINE "Coroutine"

class Coroutine {
public:
    Coroutine(const std::string &name, const asIScriptFunction* cofunc, asIScriptEngine* engine);
    ~Coroutine();

    asIScriptContext* GetContext() const { return context; }
    void SetSceneInstance(Scene* scene) { context->SetUserData(scene, SU_UDTYPE_SCENE); }
    int Execute() { return context->Execute(); }

public:
    std::string Name;
    CoroutineWait Wait;

private:
    asIScriptContext *context;
    void *object;
    asITypeInfo *type;
    asIScriptFunction *function;
};

class ScriptScene : public Scene {
    typedef Scene Base;
protected:
    asIScriptContext * const context;
    asIScriptObject * const sceneObject;
    asITypeInfo * const sceneType;

    std::multiset<SSprite*, SSprite::Comparator> sprites;
    std::vector<SSprite*> spritesPending;
    std::list<Coroutine*> coroutines;
    std::list<Coroutine*> coroutinesPending;
    bool finished = false;

    void TickCoroutine(double delta);
    void TickSprite(double delta);
    void DrawSprite();

public:
    ScriptScene(asIScriptObject *scene);
    virtual ~ScriptScene();

    asIScriptObject* GetSceneObject() const { return sceneObject; }
    asITypeInfo* GetSceneType() const { return sceneType; }
    asIScriptFunction* GetMainMethod() override;

    void Initialize() override;

    void AddSprite(SSprite *sprite);
    void AddCoroutine(Coroutine *co);
    void KillCoroutine(const std::string &name);
    void Tick(double delta) override;
    void OnEvent(const std::string &message) override;
    void Draw() override;
    bool IsDead() override;
    void Disappear() override;
};

class ScriptCoroutineScene : public ScriptScene {
    typedef ScriptScene Base;
protected:
    CoroutineWait wait;

public:
    ScriptCoroutineScene(asIScriptObject *scene);
    virtual ~ScriptCoroutineScene();

    asIScriptFunction* GetMainMethod() override;

    void Tick(double delta) override;
};

class ExecutionManager;
void RegisterScriptScene(ExecutionManager *exm);

int ScriptSceneGetIndex();
void ScriptSceneSetIndex(int index);
bool ScriptSceneIsKeyHeld(int keynum);
bool ScriptSceneIsKeyTriggered(int keynum);
void ScriptSceneAddSprite(SSprite *sprite);
void ScriptSceneRunCoroutine(asIScriptFunction *cofunc, const std::string &name);
void ScriptSceneKillCoroutine(const std::string &name);
void ScriptSceneDisappear();
