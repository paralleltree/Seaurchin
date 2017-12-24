#include "Character.h"
#include "ExecutionManager.h"

void RegisterCharacterTypes(ExecutionManager *exm)
{
    auto engine = exm->GetScriptInterfaceUnsafe()->GetEngine();

    engine->RegisterInterface(SU_IF_ABILITY);
    engine->RegisterInterfaceMethod(SU_IF_ABILITY, "void Initialize()");
    engine->RegisterInterfaceMethod(SU_IF_ABILITY, "void OnStart()");
    engine->RegisterInterfaceMethod(SU_IF_ABILITY, "void OnFinish()");
    engine->RegisterInterfaceMethod(SU_IF_ABILITY, "void OnJusticeCritical()");
    engine->RegisterInterfaceMethod(SU_IF_ABILITY, "void OnJustice()");
    engine->RegisterInterfaceMethod(SU_IF_ABILITY, "void OnAttack()");
    engine->RegisterInterfaceMethod(SU_IF_ABILITY, "void OnMiss()");
}