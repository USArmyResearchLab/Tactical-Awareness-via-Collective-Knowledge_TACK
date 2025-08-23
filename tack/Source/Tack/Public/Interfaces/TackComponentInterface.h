#pragma once
#include "CoreMinimal.h"
#include "TackComponentInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType, meta = (CannotImplementInterfaceInBlueprint))
class UTackComponentInterface : public UInterface
{
    GENERATED_BODY()
};

class TACK_API ITackComponentInterface
{
    GENERATED_BODY()
public:
    virtual const FGuid& GetTackId() const
    {
        static const FGuid INVALID_GUID = FGuid();
        return INVALID_GUID;
    };

    virtual bool HasValidTackId() const { return GetTackId().IsValid(); }
};