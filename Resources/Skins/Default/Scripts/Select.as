[EntryPoint]
class Title : CoroutineScene {
  Skin@ skin;
  MusicCursor@ cursor;
  Font@ font32, font64, fontLatin;
  Image@ imgWhite, imgBarMusic, imgMusicFrame, imgLevel;
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
    @imgLevel = skin.GetImage("LevelFrame");
  }
  
  //ここからコルーチン
  array<TextSprite@> titles(5);
  array<TextSprite@> artists(5), levelnums(5), leveltypes(5);
  array<Container@> musics(5);
  array<Sprite@> jackets(5), frames(5), levels(5);
  void Main() {
    for(int i = 0; i < 5; i++) {
      @musics[i] = Container();
      musics[i].Apply("y: 360, z:2, x:" + (-320.0 + 480 * i));
      
      @frames[i] = Sprite(imgMusicFrame);
      frames[i].Apply("origX:200, origY:300");
      
      @jackets[i] = Sprite();
      jackets[i].Apply("scaleX:0.5, scaleY:0.5, x:-160, y:-260");
      
      @titles[i] = TextSprite(font64, "");
      titles[i].Apply("y:108, r:0, g:0, b:0");
      titles[i].SetRangeScroll(300, 40, 64);
      titles[i].SetAlignment(TextAlign::Center, TextAlign::Top);
      
      @artists[i] = TextSprite(font64, "");
      artists[i].Apply("y:182, r:0, g:0, b:0");
      artists[i].SetRangeScroll(300, 40, 64);
      artists[i].SetAlignment(TextAlign::Center, TextAlign::Top);
      
      @levels[i] = Sprite(imgLevel);
      levels[i].Apply("x:100, y:-10");
      @levelnums[i] = TextSprite(font64, "");
      levelnums[i].SetAlignment(TextAlign::Center, TextAlign::Top);
      levelnums[i].Apply("x:148, y:26, r:0, g:0, b:0");
      @leveltypes[i] = TextSprite(font32, "");
      leveltypes[i].SetAlignment(TextAlign::Center, TextAlign::Top);
      leveltypes[i].Apply("x:148, y:-8, scaleX:0.8");
      
      musics[i].AddChild(frames[i]);
      musics[i].AddChild(jackets[i]);
      musics[i].AddChild(titles[i]);
      musics[i].AddChild(artists[i]);
      musics[i].AddChild(levels[i]);
      musics[i].AddChild(levelnums[i]);
      musics[i].AddChild(leveltypes[i]);
      
      AddSprite(musics[i]);
    }
    InitCursor();
    while(true) {
      YieldFrame(1);
    }
  }
  
  void UpdateInfoAt(int obj, int index) {
    titles[obj].SetText(cursor.GetPrimaryString(index));
    
    if (isCategory) {
      jackets[obj].Apply("alpha:0");
      artists[obj].Apply("alpha:0");
      levels[obj].Apply("alpha:0");
      levelnums[obj].Apply("alpha:0");
      leveltypes[obj].Apply("alpha:0");
    } else {
      jackets[obj].Apply("alpha:1");
      artists[obj].Apply("alpha:1");
      levels[obj].Apply("alpha:1");
      levelnums[obj].Apply("alpha:1");
      leveltypes[obj].Apply("alpha:1");
      
      jackets[obj].SetImage(Image(cursor.GetMusicJacketFileName(index)));
      artists[obj].SetText(cursor.GetArtistName(index));
      
      int dt = cursor.GetDifficulty(index);
      string st = "";
      int stars = cursor.GetLevel(index);
      switch(dt) {
        case 0:
          leveltypes[obj].SetText("BASIC");
          levelnums[obj].SetText("" + stars);
          break;
        case 1:
          leveltypes[obj].SetText("ADVANCED");
          levelnums[obj].SetText("" + stars);
          break;
        case 2:
          leveltypes[obj].SetText("EXPERT");
          levelnums[obj].SetText("" + stars);
          break;
        case 3:
          leveltypes[obj].SetText("MASTER");
          levelnums[obj].SetText("" + stars);
          break;
        case 4:
          for(int i = 0; i < stars; i++) st += "★";
          leveltypes[obj].SetText(st);
          levelnums[obj].SetText(cursor.GetExtraLevel(index));
          break;
      }
    }
  }
  
  bool isCategory = true;
  int center = 2;
  
  void InitCursor() {
    int add = (7 - center) % 5;
    for(int i = 0; i < 5; i++) UpdateInfoAt(i, (i + add) % 5 - 2);
  }
  
  void UpdateCursor(int adjust) {
    for(int i = 0; i < 5; i++) {
      musics[i].AbortMove();
      int add = (7 - center) % 5;
      musics[i].Apply("x:" + (640 + 480 * ((i + add) % 5 - 2)));
    }
    int flew = (5 + center - adjust * 2) % 5;
    musics[flew].Apply("x:" + (640 + 480 * adjust * 3));
    
    UpdateInfoAt(flew, adjust * 2);
    
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
      } else if (IsKeyTriggered(Key::INPUT_UP)) { 
        cursor.NextVariant();
        UpdateCursor(0);
      } else if (IsKeyTriggered(Key::INPUT_DOWN)) {
        cursor.PreviousVariant();
        UpdateCursor(0);
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
  Character@ ch;
  CharacterImages@ cim;
  SkillManager@ sm;
  Skill@ sk;
  
  Sprite@ spBack, spImage, spIcon;
  TextSprite@ spTitle, spInfo, spName, spSkill, spDescription;
  Container@ container;
  
  void Initialize() {
    @skin = GetSkin();
    @cm = GetCharacterManager();
    @sm = GetSkillManager();
    @container = Container();
    
    @spBack = Sprite(skin.GetImage("White"));
    spBack.Apply("r:0, g:0, b:0, alpha:0.8");
    
    @spTitle = TextSprite(skin.GetFont("Normal64"), "キャラクター・スキル設定");
    spTitle.SetAlignment(TextAlign::Center, TextAlign::Top);
    spTitle.Apply("x:640, y:12");
    
    @spInfo = TextSprite(skin.GetFont("Normal32"), "カーソルキー左右でキャラクター変更、上下でスキル変更");
    spInfo.SetAlignment(TextAlign::Center, TextAlign::Top);
    spInfo.Apply("x:640, y:688");
    
    @spName = TextSprite(skin.GetFont("Normal64"), "");
    spName.SetAlignment(TextAlign::Center, TextAlign::Top);
    spName.Apply("x:320, y:600");
    
    @spImage = Sprite();
    spImage.Apply("x:320, y:256");
    
    @spIcon = Sprite();
    spIcon.Apply("x:960, y:256, origX:48, origY:48, scaleX: 1.25, scaleY:1.25");
    
    @spSkill = TextSprite(skin.GetFont("Normal64"), "");
    spSkill.SetAlignment(TextAlign::Center, TextAlign::Top);
    spSkill.Apply("x:960, y:360, scaleX:0.75, scaleY:0.75");
    
    @spDescription = TextSprite(skin.GetFont("Normal64"), "");
    spDescription.SetAlignment(TextAlign::Center, TextAlign::Top);
    spDescription.SetRich(true);
    spDescription.Apply("x:960, y:440, scaleX:0.75, scaleY:0.75");
    
    container.AddChild(spBack);
    container.AddChild(spTitle);
    container.AddChild(spInfo);
    
    container.AddChild(spName);
    container.AddChild(spImage);
    
    container.AddChild(spIcon);
    container.AddChild(spSkill);
    container.AddChild(spDescription);
    
    AddSprite(container);
    container.Apply("x:-1280");
    
    UpdateInfo();
    UpdateSkill();
  }
  
  void Run() {
    RunCoroutine(Coroutine(KeyInput), "Select:KeyInput");
    while(true) YieldTime(1);
  }
  
  void UpdateInfo() {
    @ch = cm.GetCharacter(0);
    @cim = cm.CreateCharacterImages(0);
    
    spName.SetText(ch.Name);
    //spDescription.SetText(cm.GetDescription(0));
    cim.ApplyFullImage(spImage);
  }
  
  void UpdateSkill() {
    @sk = sm.GetSkill(0);
    spSkill.SetText(sk.Name);
    spDescription.SetText(sk.GetDescription(0));
    spIcon.SetImage(Image(sk.IconPath));
  }
  
  void Draw() {
    
  }
  
  void KeyInput() {
    while(true) {
      if (IsKeyTriggered(Key::INPUT_TAB)) {
        if (isEnabled) {
          Fire("Select:Enable");
          container.AddMove("move_to(x:-1280, time:0.5, ease:out_sine)");
        } else {
          Fire("Select:Disable");
          container.AddMove("move_to(x:0, time:0.5, ease:out_sine)");
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
        if (IsKeyTriggered(Key::INPUT_UP)) {
          sm.Previous();
          FadeSkill();
        }
        if (IsKeyTriggered(Key::INPUT_DOWN)) {
          sm.Next();
          FadeSkill();
        }
      }
      YieldFrame(1);
    }
  }
  
  void FadeChar() {
    spName.AddMove("alpha(x:1, y:0, time:0.2)");
    spImage.AddMove("alpha(x:1, y:0, time:0.2)");
    YieldTime(0.2);
    UpdateInfo();
    spName.AddMove("alpha(x:0, y:1, time:0.2)");
    spImage.AddMove("alpha(x:0, y:1, time:0.2)");
  }
  
  void FadeSkill() {
    spIcon.AddMove("alpha(x:1, y:0, time:0.2)");
    spSkill.AddMove("alpha(x:1, y:0, time:0.2)");
    spDescription.AddMove("alpha(x:1, y:0, time:0.2)");
    YieldTime(0.2);
    UpdateSkill();
    spIcon.AddMove("alpha(x:0, y:1, time:0.2)");
    spSkill.AddMove("alpha(x:0, y:1, time:0.2)");
    spDescription.AddMove("alpha(x:0, y:1, time:0.2)");
  }
  
  void OnEvent(const string &in event) {
    if (event == "Select:End") Disappear();
  }
}