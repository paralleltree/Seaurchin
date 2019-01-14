#pragma once

#include "Scene.h"
#include "ScriptSprite.h"
#include "ScriptFunction.h"

#define SU_IF_SCENE "Scene"
#define SU_IF_COSCENE "CoroutineScene"
#define SU_IF_COROUTINE "Coroutine"

typedef struct
{
    std::string Name;
    void *Object;
    asIScriptContext *Context;
    asITypeInfo *Type;
    asIScriptFunction *Function;
    CoroutineWait Wait;
} Coroutine;

class ScriptScene : public Scene
{
    typedef Scene Base;
protected:
    asIScriptContext *context;
    asIScriptObject *sceneObject;
    asITypeInfo *sceneType;
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
    ~ScriptScene();

    void Initialize() override;

    void AddSprite(SSprite *sprite);
    void AddCoroutine(Coroutine *co);
    void Tick(double delta) override;
    void OnEvent(const std::string &message) override;
    void Draw() override;
    bool IsDead() override;
    void Disappear();

    friend void ScriptSceneKillCoroutine(const std::string &name);
};

class ScriptCoroutineScene : public ScriptScene
{
    typedef ScriptScene Base;
protected:
    asIScriptContext *runningContext;
    CoroutineWait wait;

public:
    ScriptCoroutineScene(asIScriptObject *scene);
    ~ScriptCoroutineScene();
    
    
    void Tick(double delta) override;
    void Initialize() override;

};

class ExecutionManager;
void RegisterScriptScene(ExecutionManager *exm);

int ScriptSceneGetIndex();
void ScriptSceneSetIndex(int index);
bool ScriptSceneIsKeyHeld(int keynum);
bool ScriptSceneIsKeyTriggered(int keynum);
void ScriptSceneAddScene(asIScriptObject *sceneObject);
void ScriptSceneAddSprite(SSprite *sprite);
void ScriptSceneRunCoroutine(asIScriptFunction *cofunc, const std::string &name);
void ScriptSceneKillCoroutine(const std::string &name);
void ScriptSceneDisappear();