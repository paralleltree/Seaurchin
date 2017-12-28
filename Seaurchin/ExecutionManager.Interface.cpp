#include "ExecutionManager.h"

using namespace std;

void ExecutionManager::ReloadMusic()
{
    Musics->Reload(true);
}

void ExecutionManager::Fire(const string & message)
{
    for (auto &scene : Scenes) scene->OnEvent(message);
}

void ExecutionManager::WriteLog(const string &message)
{
    auto log = spdlog::get("main");
    log->info(message);
}

ScenePlayer *ExecutionManager::CreatePlayer()
{
    auto player = new ScenePlayer(this);
    player->AddRef();
    return player;
}

SSoundMixer *ExecutionManager::GetDefaultMixer(const string &name)
{
    if (name == "BGM") {
        MixerBGM->AddRef();
        return MixerBGM;
    }
    if (name == "SE") {
        MixerSE->AddRef();
        return MixerSE;
    }
    return nullptr;
}

SSettingItem *ExecutionManager::GetSettingItem(const string &group, const string &key)
{
    auto si = SettingManager->GetSettingItem(group, key);
    auto result = new SSettingItem(si);
    result->AddRef();
    return result;
}