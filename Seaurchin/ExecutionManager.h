#pragma once

#include "Setting.h"
#include "AngelScriptManager.h"
#include "Scene.h"
#include "ScriptScene.h"
#include "SkinHolder.h"
#include "MusicsManager.h"
#include "ExtensionManager.h"
#include "SoundManager.h"
#include "ScenePlayer.h"
#include "Controller.h"
#include "Character.h"
#include "Skill.h"

class ExecutionManager final {
    friend class ScenePlayer;

private:
    std::shared_ptr<Setting> sharedSetting;
    std::unique_ptr<setting2::SettingItemManager> settingManager;
    std::shared_ptr<AngelScript> scriptInterface;
    std::vector<std::shared_ptr<Scene>> scenes;
    std::vector<std::shared_ptr<Scene>> scenesPending;
    std::shared_ptr<ControlState> sharedControlState;
    std::vector<std::wstring> skinNames;
    std::unique_ptr<SkinHolder> skin;
    std::unique_ptr<ExtensionManager> extensions;
    std::shared_ptr<std::mt19937> random;
    std::shared_ptr<SoundManager> sound;
    std::shared_ptr<MusicsManager> musics;
    std::shared_ptr<CharacterManager> characters;
    std::shared_ptr<SkillManager> skills;
    std::unordered_map<std::string, boost::any> optionalData;
    DrawableResult lastResult;
    HIMC hImc;
    HANDLE hCommunicationPipe;
    DWORD immConversion, immSentence;
    SSoundMixer *mixerBgm, *mixerSe;

public:
    explicit ExecutionManager(const std::shared_ptr<Setting>& setting);

    void EnumerateSkins();
    void Tick(double delta);
    void Draw();
    void Initialize();
    void Shutdown() const;
    void AddScene(const std::shared_ptr<Scene>& scene);
    std::shared_ptr<ScriptScene> CreateSceneFromScriptType(asITypeInfo *type) const;
    std::shared_ptr<ScriptScene> CreateSceneFromScriptObject(asIScriptObject *obj) const;
    int GetSceneCount() const { return scenes.size(); }

    std::shared_ptr<MusicsManager> GetMusicsManager() const { return musics; }
    std::shared_ptr<ControlState> GetControlStateSafe() const { return sharedControlState; }
    std::shared_ptr<Setting> GetSettingInstanceSafe() const { return sharedSetting; }
    std::shared_ptr<AngelScript> GetScriptInterfaceSafe() const { return scriptInterface; }
    ControlState* GetControlStateUnsafe() const { return sharedControlState.get(); }
    AngelScript* GetScriptInterfaceUnsafe() const { return scriptInterface.get(); }
    SoundManager* GetSoundManagerUnsafe() const { return sound.get(); }
    std::shared_ptr<CharacterManager> GetCharacterManagerSafe() const { return characters; }
    std::shared_ptr<SkillManager> GetSkillManagerSafe() const { return skills; }
    CharacterManager* GetCharacterManagerUnsafe() const { return characters.get(); }
    SkillManager* GetSkillManagerUnsafe() const { return skills.get(); }

    std::tuple<bool, LRESULT> CustomWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
    void ExecuteSkin();
    bool ExecuteSkin(const std::string &file);
    bool ExecuteScene(asIScriptObject *sceneObject);
    void ExecuteSystemMenu();
    void ReloadMusic() const;
    void Fire(const std::string &message);
    void WriteLog(const std::string &message) const;
    ScenePlayer *CreatePlayer();
    SSoundMixer *GetDefaultMixer(const std::string &name) const;
    SSettingItem *GetSettingItem(const std::string &group, const std::string &key) const;
    void GetStoredResult(DrawableResult *result) const;

    template<typename T>
    void SetData(const std::string &name, const T& data);
    template<typename T>
    T GetData(const std::string &name);
    template<typename T>
    T GetData(const std::string &name, const T& defaultValue);
    bool ExistsData(const std::string &name) { return optionalData.find(name) != optionalData.end(); }

private:
    bool CheckSkinStructure(const boost::filesystem::path& name) const;
    void RegisterGlobalManagementFunction();
};

template<typename T>
void ExecutionManager::SetData(const std::string &name, const T & data)
{
    optionalData[name] = data;
}

template<typename T>
T ExecutionManager::GetData(const std::string &name)
{
    auto it = optionalData.find(name);
    return it == optionalData.end() ? T() : boost::any_cast<T>(it->second);
}

template<typename T>
T ExecutionManager::GetData(const std::string &name, const T& defaultValue)
{
    auto it = optionalData.find(name);
    return it == optionalData.end() ? defaultValue : boost::any_cast<T>(it->second);
}