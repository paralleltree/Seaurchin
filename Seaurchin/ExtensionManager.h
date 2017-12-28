#pragma once

class ExtensionManager final {
private:
    std::vector<HINSTANCE> DllInstances;
    void LoadDll(std::wstring path);

public:
    ExtensionManager();
    ~ExtensionManager();

    void LoadExtensions();
    void Initialize(asIScriptEngine *engine);
    void RegisterInterfaces();
};