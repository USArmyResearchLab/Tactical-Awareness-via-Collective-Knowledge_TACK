#include "Subsystems/TackSubsystem.h"
#include "Engine/GameInstance.h"
#include "TackManager.h"


UWorld* UTackSubsystem::GetWorld() const
{
    return GetGameInstance()->GetWorld();
}

UGameInstance* UTackSubsystem::GetGameInstance() const
{
    return TackManager->GetGameInstance();
}

void UTackSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    TackManager = CastChecked<UTackManager>(GetOuter());
}