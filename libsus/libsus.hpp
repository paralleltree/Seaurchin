#pragma once

#include <stdint.h>

namespace libsus
{

enum class NoteType {
    Undefined = 0,
    Tap,
    ExTap,
    AwesomeExTap,
    Flick,
    HellTap,
    Air,
    Hold,
    Slide,
    AirAction,
};

enum class NoteExtraType {
    None = 0,
    Start,
    End,
    Step,
    Control,
    Invisible,
};

struct RelativeTimestamp {
    uint32_t Measure;
    uint32_t Tick;
};

struct RelativeNote {
    RelativeTimestamp Timestamp;
    NoteType Type;
    NoteExtraType ExtraType;
    uint8_t Position;
    uint8_t Width;
};

}