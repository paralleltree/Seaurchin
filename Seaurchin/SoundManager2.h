#pragma once

class SoundManager2 final {
private:
    std::shared_ptr<SoLoud::Soloud> SoloudInstance;

public:
    SoundManager2();
    ~SoundManager2();
};