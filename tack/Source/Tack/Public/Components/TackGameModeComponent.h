#pragma once
#include "GameFramework/GameMode.h"
#include "TackGameModeComponent.generated.h"

UCLASS(BlueprintType, Blueprintable, Transient, Within = GameMode)
class TACK_API UTackGameModeComponent : public UTackGameModeBaseComponent
{
    GENERATED_BODY()
};