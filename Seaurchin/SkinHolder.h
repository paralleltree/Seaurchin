#pragma once

#include "AngelScriptManager.h"
#include "SoundManager.h"
#include "ScriptResource.h"

#define SU_IF_SKIN "Skin"

class SkinHolder final {
private:
    std::shared_ptr<AngelScript> scriptInterface;
    std::shared_ptr<SoundManager> soundInterface;
    std::wstring skinName;
    boost::filesystem::path skinRoot;
    std::unordered_map<std::string, SImage*> images;
    std::unordered_map<std::string, SFont*> fonts;
    std::unordered_map<std::string, SSound*> sounds;
    std::unordered_map<std::string, SAnimatedImage*> animatedImages;
    //std::unordered_map<std::string, shared_ptr<Image>> Images;

    static bool IncludeScript(std::wstring include, std::wstring from, CWScriptBuilder *builder);

public:
    SkinHolder(const std::wstring &name, std::shared_ptr<AngelScript> script, std::shared_ptr<SoundManager> sound);
    ~SkinHolder();

    void Initialize();
    void Terminate();
    asIScriptObject* ExecuteSkinScript(const std::wstring &file);
    void LoadSkinImage(const std::string &key, const std::string &filename);
    void LoadSkinFont(const std::string &key, const std::string &filename);
    void LoadSkinSound(const std::string &key, const std::string &filename);
    void LoadSkinAnime(const std::string &key, const std::string &filename, int x, int y, int w, int h, int c, double time);
    SImage* GetSkinImage(const std::string &key);
    SFont* GetSkinFont(const std::string &key);
    SSound* GetSkinSound(const std::string &key);
    SAnimatedImage* GetSkinAnime(const std::string &key);
};

class ExecutionManager;
void RegisterScriptSkin(ExecutionManager *exm);
SkinHolder* GetSkinObject();