#pragma once

#include "ScriptResource.h"
#include "SoundManager.h"

enum class WaitType {
    Frame,
    Time,
};

struct CoroutineWait {
    WaitType Type;
    union {
        double time;
        int64_t frames;
    };

    bool Tick(double delta) {
        switch (Type) {
        case WaitType::Frame:
            if(frames > 0) --frames;
            return frames > 0;
        case WaitType::Time:
            if(time > 0.0) time -= delta;
            return time > 0.0;
        default:
            spdlog::get("main")->critical(u8"CoroutineWaitのステータスが不正です");
            abort();
        }
        return false;
    }
};

void YieldTime(double time);
void YieldFrames(int64_t frames);
SImage *LoadSystemImage(const std::string & file);
SFont *LoadSystemFont(const std::string & file);
SSound *LoadSystemSound(SoundManager *smng, const std::string & file);
void CreateImageFont(const std::string & fileName, const std::string & saveName, int size);
void EnumerateInstalledFonts();
