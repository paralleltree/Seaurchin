#pragma once
#include "Easing.h"
#include "Misc.h"
#include "MoverFunction.h"

struct SpriteMoverArgument {
    easing::EasingFunction Ease;
    float X;
    float Y;
    float Z;
    double Duration;
    double Wait;

    SpriteMoverArgument();
};

struct SpriteMoverData {
    float Extra1;
    float Extra2;
    float Extra3;
    double Now;
    bool IsStarted;

    SpriteMoverData();
};

struct SpriteMoverObject {
    mover_function::Action Function;
    SpriteMoverArgument Argument;
    SpriteMoverData Data;
};

class SSprite;

class ScriptSpriteMover2 final {
private:
    SSprite *target;
    std::list<std::unique_ptr<SpriteMoverObject>> moves;

public:
    ScriptSpriteMover2(SSprite *target);
    ~ScriptSpriteMover2();

    void Tick(double delta);

    void AddMove(const std::string &params);
    void Abort(bool completeMove);

    std::unique_ptr<SpriteMoverObject> BuildMoverObject(const std::string &func, const PropList &props) const;
};
