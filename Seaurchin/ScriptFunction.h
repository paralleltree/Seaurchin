#pragma once

#include "ScriptResource.h"
#include "SoundManager.h"

enum class WaitType
{
    Frame,
    Time,
};

struct CoroutineWait
{
    WaitType Type;
    union
    {
        double time;
        int64_t frames;
    };
};

void YieldTime(double time);
void YieldFrames(int64_t frames);
SImage *LoadSystemImage(const std::string & file);
SFont *LoadSystemFont(const std::string & file);
SSound *LoadSystemSound(SoundManager *smng, const std::string & file);
void CreateImageFont(const std::string & fileName, const std::string & saveName, int size);
void EnumerateInstalledFonts();