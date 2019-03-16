#include "Main.h"
#include "resource.h"
#include "Debug.h"
#include "Setting.h"
#include "ExecutionManager.h"
#include "SceneDebug.h"

using namespace std;
using namespace std::chrono;

void PreInitialize(HINSTANCE hInstance);
void Initialize();
void Run();
void Terminate();
LRESULT CALLBACK CustomWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

shared_ptr<Setting> setting;
shared_ptr<Logger> logger;
unique_ptr<ExecutionManager> manager;
WNDPROC dxlibWndProc;
HWND hDxlibWnd;

int WINAPI WinMain(const HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    PreInitialize(hInstance);
    Initialize();

    Run();

    Terminate();
    return 0;
}

void PreInitialize(HINSTANCE hInstance)
{
    logger = make_shared<Logger>();
    logger->Initialize();

    setting = make_shared<Setting>(hInstance);
    setting->Load(SU_SETTING_FILE);
    const auto vs = setting->ReadValue<bool>("Graphic", "WaitVSync", false);
    const auto fs = setting->ReadValue<bool>("Graphic", "Fullscreen", false);

    SetUseCharCodeFormat(DX_CHARCODEFORMAT_UTF16LE);
    ChangeWindowMode(fs ? FALSE : TRUE);
    SetMainWindowText(reinterpret_cast<const char*>(ConvertUTF8ToUnicode(SU_APP_NAME " " SU_APP_VERSION).c_str()));
    SetAlwaysRunFlag(TRUE);
    SetWaitVSyncFlag(vs ? TRUE : FALSE);
    SetWindowIconID(IDI_ICON1);
    SetUseFPUPreserveFlag(TRUE);
    SetGraphMode(SU_RES_WIDTH, SU_RES_HEIGHT, 32);
    SetFullSceneAntiAliasingMode(2, 2);
}

void Initialize()
{
    if (DxLib_Init() == -1) abort();
    logger->LogInfo(u8"DxLib初期化OK");

    //WndProc差し替え
    hDxlibWnd = GetMainWindowHandle();
    dxlibWndProc = WNDPROC(GetWindowLong(hDxlibWnd, GWL_WNDPROC));
    SetWindowLong(hDxlibWnd, GWL_WNDPROC, LONG(CustomWindowProc));
    //D3D設定
    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(TRUE);
    SetDrawScreen(DX_SCREEN_BACK);

    manager = make_unique<ExecutionManager>(setting);
    manager->Initialize();
}

void Run()
{
    if (CheckHitKey(KEY_INPUT_F2)) {
        manager->ExecuteSystemMenu();
    } else {
        manager->EnumerateSkins();
        manager->ExecuteSkin();
    }
    manager->AddScene(static_pointer_cast<Scene>(make_shared<SceneDebug>()));


    auto start = high_resolution_clock::now();
    auto pstart = start;
    while (ProcessMessage() != -1) {
        pstart = start;
        start = high_resolution_clock::now();
        const auto delta = duration_cast<nanoseconds>(start - pstart).count() / 1000000000.0;
        manager->Tick(delta);
        manager->Draw();
    }
}

void Terminate()
{
    manager->Shutdown();
    manager.reset(nullptr);
    setting->Save();
    setting.reset();
    DxLib_End();
    logger->Terminate();
}

LRESULT CALLBACK CustomWindowProc(const HWND hWnd, const UINT msg, const WPARAM wParam, const LPARAM lParam)
{
    bool processed;
    LRESULT result;
    tie(processed, result) = manager->CustomWindowProc(hWnd, msg, wParam, lParam);

    if (processed) {
        return result;
    } else {
        return CallWindowProc(dxlibWndProc, hWnd, msg, wParam, lParam);
    }
}
