#pragma once

class SSprite;
struct SpriteMoverData;
struct SpriteMoverArgument;

namespace MoverFunction
{
using Action = std::function<bool(SSprite*, SpriteMoverArgument&,  SpriteMoverData&, double)>;
extern std::unordered_map<std::string, Action> actions;

bool ActionMoveTo(SSprite* target, SpriteMoverArgument &args, SpriteMoverData &data, double delta);
bool ActionMoveBy(SSprite* target, SpriteMoverArgument &args, SpriteMoverData &data, double delta);
bool ActionAngleTo(SSprite* target, SpriteMoverArgument &args, SpriteMoverData &data, double delta);
bool ActionAngleBy(SSprite* target, SpriteMoverArgument &args, SpriteMoverData &data, double delta);
bool ActionScaleTo(SSprite* target, SpriteMoverArgument &args, SpriteMoverData &data, double delta);
bool ActionColor(SSprite* target, SpriteMoverArgument &args, SpriteMoverData &data, double delta);
bool ActionAlpha(SSprite* target, SpriteMoverArgument &args, SpriteMoverData &data, double delta);
bool ActionDeath(SSprite* target, SpriteMoverArgument &args, SpriteMoverData &data, double delta);
}

