#include "MoverFunction.h"
#include "ScriptSprite.h"
#include "ScriptSpriteMover2.h"

using namespace std;

namespace mover_function
{
unordered_map<string, Action> actions =
{
    { "move_to", ActionMoveTo },
    { "move_by", ActionMoveBy },
    { "angle_to", ActionAngleTo },
    { "angle_by", ActionAngleBy },
    { "scale_to", ActionScaleTo },
    { "alpha", ActionAlpha },
    { "death", ActionDeath }
};

bool ActionMoveTo(SSprite* target, SpriteMoverArgument &args, SpriteMoverData &data, const double delta)
{
    if (delta == 0) {
        data.Extra1 = target->Transform.X;
        data.Extra2 = target->Transform.Y;
        return false;
    } else if (delta >= 0) {
        if (!isnan(args.X)) target->Transform.X = args.Ease(data.Now, args.Duration, data.Extra1, args.X - data.Extra1);
        if (!isnan(args.Y)) target->Transform.Y = args.Ease(data.Now, args.Duration, data.Extra2, args.Y - data.Extra2);
        return false;
    } else {
        if (!isnan(args.X)) target->Transform.X = args.X;
        if (!isnan(args.Y)) target->Transform.Y = args.Y;
        return true;
    }
}

bool ActionMoveBy(SSprite* target, SpriteMoverArgument &args, SpriteMoverData &data, const double delta)
{
    if (delta == 0) {
        data.Extra1 = target->Transform.X;
        data.Extra2 = target->Transform.Y;
        return false;
    } else if (delta >= 0) {
        if (!isnan(args.X)) target->Transform.X = args.Ease(data.Now, args.Duration, data.Extra1, args.X);
        if (!isnan(args.Y)) target->Transform.Y = args.Ease(data.Now, args.Duration, data.Extra2, args.Y);
        return false;
    } else {
        if (!isnan(args.X)) target->Transform.X = data.Extra1 + args.X;
        if (!isnan(args.Y)) target->Transform.Y = data.Extra2 + args.Y;
        return true;
    }
}

bool ActionAngleTo(SSprite* target, SpriteMoverArgument &args, SpriteMoverData &data, const double delta)
{
    if (delta == 0) {
        if (isnan(args.X)) return true;
        data.Extra1 = target->Transform.Angle;
        return false;
    } else if (delta >= 0) {
        target->Transform.Angle = args.Ease(data.Now, args.Duration, data.Extra1, args.X - data.Extra1);
        return false;
    } else {
        target->Transform.Angle = args.X;
        return true;
    }
}

bool ActionAngleBy(SSprite* target, SpriteMoverArgument &args, SpriteMoverData &data, const double delta)
{
    if (delta == 0) {
        if (isnan(args.X)) return true;
        data.Extra1 = target->Transform.Angle;
        return false;
    } else if (delta >= 0) {
        target->Transform.Angle = args.Ease(data.Now, args.Duration, data.Extra1, args.X);
        return false;
    } else {
        target->Transform.Angle = data.Extra1 + args.X;
        return true;
    }
}

bool ActionScaleTo(SSprite* target, SpriteMoverArgument &args, SpriteMoverData &data, const double delta)
{
    if (delta == 0) {
        data.Extra1 = target->Transform.ScaleX;
        data.Extra2 = target->Transform.ScaleY;
        return false;
    } else if (delta >= 0) {
        if (!isnan(args.X)) target->Transform.ScaleX = args.Ease(data.Now, args.Duration, data.Extra1, args.X - data.Extra1);
        if (!isnan(args.Y)) target->Transform.ScaleY = args.Ease(data.Now, args.Duration, data.Extra2, args.Y - data.Extra2);
        return false;
    } else {
        if (!isnan(args.X)) target->Transform.ScaleX = args.X;
        if (!isnan(args.Y)) target->Transform.ScaleY = args.Y;
        return true;
    }
}

bool ActionColor(SSprite* target, SpriteMoverArgument &args, SpriteMoverData &data, const double delta)
{
    if (delta == 0) {
        if (!isnan(args.X)) data.Extra1 = target->Color.R;
        if (!isnan(args.Y)) data.Extra2 = target->Color.G;
        if (!isnan(args.Z)) data.Extra3 = target->Color.B;
        return false;
    } else if (delta >= 0) {
        if (!isnan(args.X)) target->Color.R = uint8_t(args.Ease(data.Now, args.Duration, data.Extra1, args.X - data.Extra1));
        if (!isnan(args.Y)) target->Color.G = uint8_t(args.Ease(data.Now, args.Duration, data.Extra2, args.Y - data.Extra2));
        if (!isnan(args.Z)) target->Color.B = uint8_t(args.Ease(data.Now, args.Duration, data.Extra3, args.Z - data.Extra3));
        return false;
    } else {
        if (!isnan(args.X)) target->Color.R = uint8_t(args.X);
        if (!isnan(args.Y)) target->Color.G = uint8_t(args.Y);
        if (!isnan(args.Z)) target->Color.B = uint8_t(args.Z);
        return true;
    }
}

bool ActionAlpha(SSprite* target, SpriteMoverArgument &args, SpriteMoverData &data, const double delta)
{
    if (delta == 0) {
        if (isnan(args.X) || isnan(args.Y)) return true;
        target->Color.A = uint8_t(args.X * 255.0);
        return false;
    } else if (delta >= 0) {
        target->Color.A = uint8_t(255.0 * args.Ease(data.Now, args.Duration, args.X, args.Y - args.X));
        return false;
    } else {
        target->Color.A = uint8_t(args.Y * 255.0);
        return true;
    }
}

bool ActionDeath(SSprite* target, SpriteMoverArgument &args, SpriteMoverData &data, const double delta)
{
    if (delta >= 0) {
        return false;
    } else {
        target->Dismiss();
        return true;
    }
}

}