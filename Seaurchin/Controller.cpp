#include "Controller.h"

using namespace std;

static int WacomFingerCallback(WacomMTFingerCollection *fingerPacket, void *userData);

void ControlState::Initialize()
{
    ZeroMemory(keyboardCurrent, sizeof(char) * 256);
    ZeroMemory(keyboardLast, sizeof(char) * 256);
    ZeroMemory(keyboardTrigger, sizeof(char) * 256);
    ZeroMemory(integratedSliderCurrent, sizeof(char) * 16);
    ZeroMemory(integratedSliderLast, sizeof(char) * 16);
    ZeroMemory(integratedSliderTrigger, sizeof(char) * 16);
    ZeroMemory(integratedAir, sizeof(char) * 4);

    sliderKeyboardNumbers[0] = KEY_INPUT_A;
    sliderKeyboardNumbers[1] = KEY_INPUT_Z;
    sliderKeyboardNumbers[2] = KEY_INPUT_S;
    sliderKeyboardNumbers[3] = KEY_INPUT_X;
    sliderKeyboardNumbers[4] = KEY_INPUT_D;
    sliderKeyboardNumbers[5] = KEY_INPUT_C;
    sliderKeyboardNumbers[6] = KEY_INPUT_F;
    sliderKeyboardNumbers[7] = KEY_INPUT_V;
    sliderKeyboardNumbers[8] = KEY_INPUT_G;
    sliderKeyboardNumbers[9] = KEY_INPUT_B;
    sliderKeyboardNumbers[10] = KEY_INPUT_H;
    sliderKeyboardNumbers[11] = KEY_INPUT_N;
    sliderKeyboardNumbers[12] = KEY_INPUT_J;
    sliderKeyboardNumbers[13] = KEY_INPUT_M;
    sliderKeyboardNumbers[14] = KEY_INPUT_K;
    sliderKeyboardNumbers[15] = KEY_INPUT_COMMA;
    airStringKeyboardNumbers[size_t(AirControlSource::AirUp)] = KEY_INPUT_PGUP;
    airStringKeyboardNumbers[size_t(AirControlSource::AirDown)] = KEY_INPUT_PGDN;
    airStringKeyboardNumbers[size_t(AirControlSource::AirHold)] = KEY_INPUT_HOME;
    airStringKeyboardNumbers[size_t(AirControlSource::AirAction)] = KEY_INPUT_END;

    InitializeWacomTouchDevice();
}

void ControlState::Terminate()
{
    if (isWacomDeviceAvailable) {
        WacomMTUnRegisterFingerReadCallback(wacomDeviceIds[0], nullptr, WMTProcessingModeNone, this);
        WacomMTQuit();
        UnloadWacomMTLib();
        delete[] wacomDeviceCapabilities;
        delete[] wacomDeviceIds;
    }
}

void ControlState::Update()
{
    memcpy_s(keyboardLast, sizeof(char) * 256, keyboardCurrent, sizeof(char) * 256);
    GetHitKeyStateAll(keyboardCurrent);
    for (int i = 0; i < 256; i++) keyboardTrigger[i] = !keyboardLast[i] && keyboardCurrent[i];

    for (int i = 0; i < 16; i++) integratedSliderLast[i] = integratedSliderCurrent[i];
    for (int i = 0; i < 16; i++) integratedSliderCurrent[i] = keyboardCurrent[sliderKeyboardNumbers[i]];
    integratedAir[size_t(AirControlSource::AirUp)] = keyboardTrigger[airStringKeyboardNumbers[size_t(AirControlSource::AirUp)]];
    integratedAir[size_t(AirControlSource::AirDown)] = keyboardTrigger[airStringKeyboardNumbers[size_t(AirControlSource::AirDown)]];
    integratedAir[size_t(AirControlSource::AirHold)] = keyboardCurrent[airStringKeyboardNumbers[size_t(AirControlSource::AirHold)]];
    integratedAir[size_t(AirControlSource::AirAction)] = keyboardTrigger[airStringKeyboardNumbers[size_t(AirControlSource::AirAction)]];

    {
        lock_guard<mutex> lock(fingerMutex);
        for (auto &finger : currentFingers) integratedSliderCurrent[finger.second->SliderPosition] = 1;
    }

    for (int i = 0; i < 16; i++) integratedSliderTrigger[i] = !integratedSliderLast[i] && integratedSliderCurrent[i];
}

bool ControlState::GetTriggerState(const ControllerSource source, const int number)
{
    switch (source) {
        case ControllerSource::RawKeyboard:
            if (number < 0 || number >= 256) return false;
            return keyboardTrigger[number];
        case ControllerSource::IntegratedSliders:
            if (number < 0 || number >= 16) return false;
            return integratedSliderTrigger[number];
        case ControllerSource::RawTouch:
            return false;
        case ControllerSource::IntegratedAir:
            if (number < 0 || number >= 4) return false;
            return integratedAir[number];
    }
    return false;
}

bool ControlState::GetCurrentState(const ControllerSource source, const int number)
{
    switch (source) {
        case ControllerSource::RawKeyboard:
            if (number < 0 || number >= 256) return false;
            return keyboardCurrent[number];
        case ControllerSource::IntegratedSliders:
            if (number < 0 || number >= 16) return false;
            return integratedSliderCurrent[number];
        case ControllerSource::RawTouch:
            return false;
        case ControllerSource::IntegratedAir:
            if (number < 0 || number >= 4) return false;
            return integratedAir[number];
    }
    return false;
}

bool ControlState::GetLastState(const ControllerSource source, const int number)
{
    switch (source) {
        case ControllerSource::RawKeyboard:
            if (number < 0 || number >= 256) return false;
            return keyboardLast[number];
        case ControllerSource::IntegratedSliders:
            if (number < 0 || number >= 16) return false;
            return integratedSliderLast[number];
        case ControllerSource::RawTouch:
            return false;
        case ControllerSource::IntegratedAir:
            if (number < 0 || number >= 4) return false;
            return integratedAir[number];
    }
    return false;
}

void ControlState::SetSliderKey(const int sliderNumber, const int keyboardNumber)
{
    if (sliderNumber < 0 || sliderNumber >= 16) return;
    if (keyboardNumber < 0 || keyboardNumber >= 256) return;
    sliderKeyboardNumbers[sliderNumber] = keyboardNumber;
}

void ControlState::InitializeWacomTouchDevice()
{
    auto log = spdlog::get("main");
    isWacomDeviceAvailable = false;
    if (!LoadWacomMTLib()) {
        log->info(u8"Wacomドライバがありませんでした");
        return;
    }
    if (WacomMTInitialize(WACOM_MULTI_TOUCH_API_VERSION)) {
        log->warn(u8"Wacomドライバの初期化に失敗しました");
        return;
    }
    log->info(u8"Wacomドライバ利用可能");

    const auto devices = WacomMTGetAttachedDeviceIDs(nullptr, 0);
    if (devices <= 0) {
        log->info(u8"Wacomデバイスがありませんでした");
        return;
    }
    wacomDeviceIds = new int[devices];
    wacomDeviceCapabilities = new WacomMTCapability[devices];
    WacomMTGetAttachedDeviceIDs(wacomDeviceIds, devices * sizeof(int));
    for (int i = 0; i < devices; i++) {
        WacomMTCapability cap = { 0 };
        WacomMTGetDeviceCapabilities(wacomDeviceIds[i], &cap);
        wacomDeviceCapabilities[i] = cap;

        log->info(u8"デバイスID {0:2d}: {1:d}", wacomDeviceIds[i], wacomDeviceCapabilities[i].CapabilityFlags);
    }

    WacomMTRegisterFingerReadCallback(wacomDeviceIds[0], nullptr, WMTProcessingModeNone, WacomFingerCallback, this);
    isWacomDeviceAvailable = true;
}

void ControlState::UpdateWacomTouchDeviceFinger(WacomMTFingerCollection *fingers)
{
    const auto cap = wacomDeviceCapabilities[0];
    for (int i = 0; i < fingers->FingerCount; i++) {
        const auto finger = fingers->Fingers[i];
        if (!finger.Confidence) continue;
        switch (finger.TouchState) {
            case WMTFingerStateNone:
                break;
            case WMTFingerStateDown: {
                lock_guard<mutex> lock(fingerMutex);
                auto data = make_shared<ControllerFingerState>();
                data->Id = finger.FingerID;
                data->State = WMTFingerStateDown;
                data->SliderPosition = floor(finger.X / cap.LogicalWidth * 16);
                currentFingers[finger.FingerID] = data;
                break;
            }
            case WMTFingerStateHold: {
                lock_guard<mutex> lock(fingerMutex);
                auto data = currentFingers[finger.FingerID];
                if (!data) {
                    auto data = make_shared<ControllerFingerState>();
                    data->Id = finger.FingerID;
                    data->State = WMTFingerStateDown;
                    data->SliderPosition = floor(finger.X / cap.LogicalWidth * 16);
                    currentFingers[finger.FingerID] = data;
                    break;
                }
                data->State = WMTFingerStateHold;
                data->SliderPosition = floor(finger.X / cap.LogicalWidth * 16);
                break;
            }
            case WMTFingerStateUp: {
                lock_guard<mutex> lock(fingerMutex);
                currentFingers.erase(finger.FingerID);
                break;
            }
        }
    }
}

// Wacom Multi-Touch Callbacks

int WacomFingerCallback(WacomMTFingerCollection *fingerPacket, void *userData)
{
    auto controller = static_cast<ControlState*>(userData);
    controller->UpdateWacomTouchDeviceFinger(fingerPacket);
    return 0;
}