#pragma once
#include "Interfaces/TackComponentInterface.h"
#include "Components/ActorComponent.h"
#include "TackIdComponent.generated.h"

UCLASS(Abstract, BlueprintType, hidecategories("Variable", "ComponentTick", ComponentReplication, Tags, Collision, AssetUserData, Activation, Cooking))
class TACK_API UTackIdComponent : public UActorComponent, public ITackComponentInterface
{
    GENERATED_BODY()
public:
    UTackIdComponent();
    virtual bool IsProxy() const { return false; }
};