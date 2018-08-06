#pragma once

enum class ControllerSource {
    RawKeyboard,
    RawTouch,
    IntegratedSliders,
    IntegratedAir,
};

enum class AirControlSource {
    AirUp,
    AirDown,
    AirHold,
    AirAction,
};

struct ControllerFingerState {
    int Id;
    WacomMTFingerState State;
    int SliderPosition;
};

class ControlState final {
    friend int WacomFingerCallback(WacomMTFingerCollection *fingerPacket, void *userData);

private:
    char keyboardCurrent[256];
    char keyboardLast[256];
    char keyboardTrigger[256];
    char integratedSliderCurrent[16];
    char integratedSliderLast[16];
    char integratedSliderTrigger[16];
    char integratedAir[4];
    std::vector<int> sliderKeyboardInputCombinations[16];
    uint32_t sliderKeyboardPrevious[16];
    uint32_t sliderKeyboardCurrent[16];
    uint32_t sliderKeyboardTrigger[16];
    std::vector<int> airStringKeyboardInputCombinations[4];
    uint32_t airStringKeyboard[4];

    bool isWacomDeviceAvailable = false;
    int *wacomDeviceIds = nullptr;
    WacomMTCapability *wacomDeviceCapabilities = nullptr;
    std::unordered_map<int, std::shared_ptr<ControllerFingerState>> currentFingers;
    std::mutex fingerMutex;
    void InitializeWacomTouchDevice();
    void UpdateWacomTouchDeviceFinger(WacomMTFingerCollection *fingers);

public:
    void Initialize();
    void Terminate();
    void Update();

    bool GetTriggerState(ControllerSource source, int number);
    bool GetCurrentState(ControllerSource source, int number);
    bool GetLastState(ControllerSource source, int number);
    void SetSliderKeyCombination(int sliderNumber, const std::vector<int>& keys);
    void SetAirStringKeyCombination(int airNumber, const std::vector<int>& keys);
};