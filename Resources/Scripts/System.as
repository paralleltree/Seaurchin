[EntryPoint]
class SystemMenu : CoroutineScene {
  void Initialize() {
    
  }
  
  void Run() {
    while(true) {
      if (IsKeyTriggered(Key::INPUT_0)) {
        CreateImageFont("C:\\Windows\\Fonts\\mgenplus-2pp-regular.ttf", "Normal32", 32);
      }
      if (IsKeyTriggered(Key::INPUT_1)) {
        CreateImageFont("C:\\Windows\\Fonts\\mgenplus-2pp-regular.ttf", "Normal64", 64);
      }
      if (IsKeyTriggered(Key::INPUT_2)) {
        CreateImageFont("C:\\Windows\\Fonts\\MagicRing.ttf", "Latin128", 128);
      }
      YieldFrame(1);
    }
  }
  
  void Draw() {
    
  }
}