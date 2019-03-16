#pragma once


class ExecutionManager;
class Scene {
protected:
    int index;
    ExecutionManager *manager;

public:
    Scene();
    virtual ~Scene();

    void SetIndex(const int i) { index = i; }
    int GetIndex() const { return index; }
    void SetManager(ExecutionManager *info) { manager = info; }
    ExecutionManager* GetManager() const { return manager; }

    virtual asIScriptFunction* GetMainMethod() { return nullptr; };

    virtual void Initialize();
    virtual void Tick(double delta);
    virtual void OnEvent(const std::string &message);
    virtual void Draw();
    virtual bool IsDead();
    virtual void Disappear();
};
