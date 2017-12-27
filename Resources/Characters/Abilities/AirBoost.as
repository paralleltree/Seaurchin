[EntryPoint]
class DamageGuard : Ability {
  int air, aa, other;
  void Initialize(dictionary@ params) {
    params.get("air", air);
    params.get("airact", aa);
    params.get("other", other);
  }
  
  void OnStart(Result@ result) {
    
  }
  
  void OnFinish(Result@ result) {
    
  }
  
  void OnJusticeCritical(Result@ result, NoteType type) {
    Execute(result, type);
  }
  
  void OnJustice(Result@ result, NoteType type) {
    Execute(result, type);
  }
  
  void OnAttack(Result@ result, NoteType type) {
    Execute(result, type);
  }
  
  void OnMiss(Result@ result, NoteType type) {
    Execute(result, type);
  }
  
  void Execute(Result@ result, NoteType type) {
    switch (type) {
      case NoteType::Air:
        result.BoostGaugeByValue(air);
        break;
      case NoteType::AirAction:
        result.BoostGaugeByValue(aa);
        break;
      default:
        result.BoostGaugeByValue(other);
        break;
    }
  }
}