[EntryPoint]
class Boost : Ability {
  double ratio;
  void Initialize(dictionary@ params) {
    params.get("bonus", ratio);
  }
  
  void OnStart(Result@ result) {
    
  }
  
  void OnFinish(Result@ result) {
    
  }
  
  void OnJusticeCritical(Result@ result, NoteType type) {
    result.BoostGaugeJusticeCritical(ratio);
  }
  
  void OnJustice(Result@ result, NoteType type) {
    result.BoostGaugeJustice(ratio);
  }
  
  void OnAttack(Result@ result, NoteType type) {
    result.BoostGaugeAttack(ratio);
  }
  
  void OnMiss(Result@ result, NoteType type) {
    
  }
}