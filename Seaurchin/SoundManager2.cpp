#include "SoundManager2.h"

using namespace std;
using namespace SoLoud;

SoundManager2::SoundManager2()
{
    SoloudInstance = make_shared<Soloud>();
    SoloudInstance->init();
}

SoundManager2::~SoundManager2()
{
    SoloudInstance->deinit();
}
