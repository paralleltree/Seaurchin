[EntryPoint]
class Title : CoroutineScene {
  Skin@ skin;
  MusicCursor@ cursor;
  Font@ font32, font64, fontLatin;
  Image@ imgWhite, imgBarMusic, imgMusicFrame;
  CharacterSelect scCharacterSelect;
  
  void Initialize() {
    LoadResources();
    AddSprite(Sprite(imgWhite));
  }
  
  void Run() {
    ReloadMusic();
    @cursor = MusicCursor();
    ExecuteScene(scCharacterSelect);
    RunCoroutine(Coroutine(Main), "Select:Main");
    RunCoroutine(Coroutine(KeyInput), "Select:KeyInput");
    while(true) YieldTime(30);
  }
  
  void Draw() {
    
  }
  
  void LoadResources() {
    @skin = GetSkin();
    @fontLatin = skin.GetFont("Latin128");
    @font32 = skin.GetFont("Normal32");
    @font64 = skin.GetFont("Normal64");
    @imgWhite = skin.GetImage("White");
    @imgBarMusic = skin.GetImage("CursorMusic");
    @imgMusicFrame = skin.GetImage("MusicSelectFrame");
  }
  
  //ここからコルーチン
  array<TextSprite@> titles(19);
  array<SynthSprite@> musics(5);
  void Main() {
    for(int i = 0; i < 5; i++) {
      @musics[i] = SynthSprite(400, 600);
      musics[i].Apply("origX:200, origY:300, y: 360, z:2, x:" + (-320.0 + 480 * i));
      AddSprite(musics[i]);
    }
    InitCursor();
    while(true) YieldFrame(1);
  }
  
  Sprite@ spj;
  TextSprite@ txt;
  bool isCategory = true;
  void InitCursor() {
    @spj = Sprite();
    @txt = TextSprite(font64, "");
    spj.Apply("scaleX:0.5, scaleY:0.5, x:40, y:40");
    txt.SetAlignment(TextAlign::Center, TextAlign::Top);
    for(int i = 0; i < 5; i++) {
      int add = (7 - center) % 5;
      musics[i].Clear();
      musics[i].Transfer(imgMusicFrame, 0, 0);
      
      //タイトル
      txt.Apply("x:200, y:408, r:0, g:0, b:0");
      txt.SetText(cursor.GetPrimaryString((i + add) % 5 - 2));
      musics[i].Transfer(txt);
      if (!isCategory) {
        //ジャケ
        Image@ jacket = Image(cursor.GetMusicJacketFileName((i + add) % 5 - 2));
        spj.SetImage(jacket);
        musics[i].Transfer(spj);
        
        //アーティスト
        txt.Apply("x:200, y:472, r:0, g:0, b:0");
        txt.SetText(cursor.GetArtistName((i + add) % 5 - 2));
        musics[i].Transfer(txt);
      }
    }
  }
  int center = 2;
  void UpdateCursor(int adjust) {
    for(int i = 0; i < 5; i++) {
      musics[i].AbortMove();
      int add = (7 - center) % 5;
      musics[i].Apply("x:" + (640 + 480 * ((i + add) % 5 - 2)));
    }
    int flew = (5 + center - adjust * 2) % 5;
    musics[flew].Apply("x:" + (640 + 480 * adjust * 3));
    musics[flew].Clear();
    musics[flew].Transfer(imgMusicFrame, 0, 0);
    
    //タイトル
    txt.Apply("x:200, y:408, r:0, g:0, b:0");
    txt.SetText(cursor.GetPrimaryString(adjust * 2));
    musics[flew].Transfer(txt);
    if (!isCategory) {
      //ジャケ
      Image@ jacket = Image(cursor.GetMusicJacketFileName(adjust * 2));
      spj.SetImage(jacket);
      musics[flew].Transfer(spj);
      //アーティスト
      txt.Apply("x:200, y:472, r:0, g:0, b:0");
      txt.SetText(cursor.GetArtistName(adjust * 2));
      musics[flew].Transfer(txt);
    }
    center = (5 + center + adjust) % 5;
    for(int i = 0; i < 5; i++) musics[i].AddMove("move_by(x:" + (480 * -adjust) + ", time:0.2, ease:out_quad)");
  }
  
  bool isEnabled = true;
  void KeyInput() {
    while(true) {
      if (!isEnabled) {
        YieldFrame(1);
        continue;
      }
      
      if (IsKeyTriggered(Key::INPUT_RETURN)) {
        if (cursor.Enter() == CursorState::Confirmed) {
          if (Execute("Play.as")) {
            Fire("Select:End");
            Disappear();
          }
        } else {
          isCategory = cursor.GetState() == CursorState::Category;
          InitCursor();
        }
      } else if (IsKeyTriggered(Key::INPUT_ESCAPE)) {
        cursor.Exit();
        auto state = cursor.GetState();
        if (state == CursorState::OutOfFunction) {
          if (Execute("Title.as")) Disappear();
        } else {
          isCategory = cursor.GetState() == CursorState::Category;
          InitCursor();
        }
      } else if (IsKeyTriggered(Key::INPUT_RIGHT)) { 
        cursor.Next();
        UpdateCursor(+1);
      } else if (IsKeyTriggered(Key::INPUT_LEFT)) {
        cursor.Previous();
        UpdateCursor(-1);
      }
      
      if (IsKeyTriggered(Key::INPUT_A)) {
        SetData("AutoPlay", 1);
        ShowMessage("オートプレイ: ON");
      }
      if (IsKeyTriggered(Key::INPUT_M)) {
        SetData("AutoPlay", 0);
        ShowMessage("オートプレイ: OFF");
      }
      if (IsKeyTriggered(Key::INPUT_S)) {
        SetData("AutoPlay", 2);
        ShowMessage("オートプレイ: Air/Air-Actionのみ");
      }
      
      YieldFrame(1);
    }
  }
  
  void OnEvent(const string &in event) {
    if (event == "Select:Disable") {
      isEnabled = false;
    }
    if (event == "Select:Enable") {
      isEnabled = true;
    }
  }
  
  void ShowMessage(string mes) {
    TextSprite@ spmes = TextSprite(font32, mes);
    spmes.Apply("y:720, z:20, r:0, g:0, b:0");
    spmes.AddMove("move_by(y:-32, time:0.5, ease:out_sine)");
    spmes.AddMove("move_by(y:32, time:0.5, wait:1.0, ease:in_sine)");
    spmes.AddMove("death(wait:2.0)");
    AddSprite(spmes);
  }
}

class CharacterSelect : CoroutineScene {
  bool isEnabled;
  Skin@ skin;
  CharacterManager@ cm;
  
  Sprite@ spBack, spImage;
  TextSprite@ spTitle, spInfo, spName, spSkill, spDescription;
  
  void Initialize() {
    @skin = GetSkin();
    @cm = GetCharacterManager();
    
    @spBack = Sprite(skin.GetImage("White"));
    spBack.Apply("r:0, g:0, b:0, alpha:0");
    
    @spTitle = TextSprite(skin.GetFont("Normal64"), "キャラクター設定");
    spTitle.SetAlignment(TextAlign::Center, TextAlign::Top);
    spTitle.Apply("x:-640, y:12");
    
    @spInfo = TextSprite(skin.GetFont("Normal32"), "カーソルキー左右で変更");
    spInfo.SetAlignment(TextAlign::Center, TextAlign::Top);
    spInfo.Apply("x:-640, y:688");
    
    @spName = TextSprite(skin.GetFont("Normal64"), cm.GetName(0));
    spName.SetAlignment(TextAlign::Center, TextAlign::Top);
    spName.Apply("x:-640, y:360");
    
    @spImage = Sprite(Image(cm.GetImagePath(0)));
    spImage.Apply("x:-640, y:200, origX:150");
    
    @spDescription = TextSprite(skin.GetFont("Normal64"), cm.GetDescription(0));
    spDescription.SetAlignment(TextAlign::Center, TextAlign::Top);
    spDescription.SetRich(true);
    spDescription.Apply("x:-640, y:440, scaleX:0.75, scaleY:0.75");
    
    AddSprite(spBack);
    AddSprite(spTitle);
    AddSprite(spInfo);
    AddSprite(spName);
    AddSprite(spImage);
    AddSprite(spDescription);
  }
  
  void Run() {
    RunCoroutine(Coroutine(KeyInput), "Select:KeyInput");
    while(true) YieldTime(1);
  }
  
  void UpdateInfo() {
    spName.SetText(cm.GetName(0));
    spDescription.SetText(cm.GetDescription(0));
    spImage.SetImage(Image(cm.GetImagePath(0)));
  }
  
  void Draw() {
    
  }
  
  void KeyInput() {
    while(true) {
      if (IsKeyTriggered(Key::INPUT_TAB)) {
        if (isEnabled) {
          Fire("Select:Enable");
          spBack.AddMove("alpha(x:0.8, y:0, time:0.5)");
          spTitle.AddMove("move_to(x:-640, time:0.5, ease:out_sine)");
          spInfo.AddMove("move_to(x:-640, time:0.5, ease:out_sine)");
          spName.AddMove("move_to(x:-640, time:0.5, ease:out_sine)");
          spImage.AddMove("move_to(x:-640, time:0.5, ease:out_sine)");
          spDescription.AddMove("move_to(x:-640, time:0.5, ease:out_sine)");
        } else {
          Fire("Select:Disable");
          spBack.AddMove("alpha(x:0, y:0.8, time:0.5)");
          spTitle.AddMove("move_to(x:640, time:0.5, ease:out_sine)");
          spInfo.AddMove("move_to(x:640, time:0.5, ease:out_sine)");
          spName.AddMove("move_to(x:640, time:0.5, ease:out_sine)");
          spImage.AddMove("move_to(x:640, time:0.5, ease:out_sine)");
          spDescription.AddMove("move_to(x:640, time:0.5, ease:out_sine)");
        }
        isEnabled = !isEnabled;
      }
      if (isEnabled) {
        if (IsKeyTriggered(Key::INPUT_LEFT)) {
          cm.Previous();
          FadeChar();
        }
        if (IsKeyTriggered(Key::INPUT_RIGHT)) {
          cm.Next();
          FadeChar();
        }
      }
      YieldFrame(1);
    }
  }
  
  void FadeChar() {
    spDescription.AddMove("alpha(x:1, y:0, time:0.2)");
    spName.AddMove("alpha(x:1, y:0, time:0.2)");
    spImage.AddMove("alpha(x:1, y:0, time:0.2)");
    YieldTime(0.2);
    UpdateInfo();
    spDescription.AddMove("alpha(x:0, y:1, time:0.2)");
    spName.AddMove("alpha(x:0, y:1, time:0.2)");
    spImage.AddMove("alpha(x:0, y:1, time:0.2)");
  }
  
  void OnEvent(const string &in event) {
    if (event == "Select:End") Disappear();
  }
}