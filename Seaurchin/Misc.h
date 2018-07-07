#pragma once

#include "Crc32.h"

#define BEGIN_DRAW_TRANSACTION(h) SetDrawScreen(h)
#define FINISH_DRAW_TRANSACTION SetDrawScreen(DX_SCREEN_BACK);

// AngelScriptÇ…ìoò^ÇµÇΩílå^ópÇÃîƒópèàóùÉAÉå

template <typename T>
void AngelScriptValueConstruct(void *address)
{
    new (address) T;
}

template <typename T>
void AngelScriptValueDestruct(void *address)
{
    static_cast<T*>(address)->~T();
}

template<typename From, typename To>
To* CastReferenceType(From *from)
{
    if (!from) return nullptr;
    To* result = dynamic_cast<To*>(from);
    if (result) result->AddRef();
    return result;
}

using PropList = std::vector<std::tuple<std::string, std::string>>;

//std::string ConvertUTF8ToShiftJis(const std::string &utf8str);
//std::string ConvertShiftJisToUTF8(const std::string &sjisstr);
std::wstring ConvertUTF8ToUnicode(const std::string &utf8str);
std::string ConvertUnicodeToUTF8(const std::wstring &utf16str);
void ScriptSceneWarnOutOf(const std::string &type, asIScriptContext *ctx);
double ToDouble(const char *str);
double NormalizedFmod(double x, double y);
int32_t ConvertInteger(const std::string &input);
uint32_t ConvertHexatridecimal(const std::string &input);
double ConvertFloat(const std::string &input);
bool ConvertBoolean(const std::string &input);
void SplitProps(const std::string &source, PropList &vec);