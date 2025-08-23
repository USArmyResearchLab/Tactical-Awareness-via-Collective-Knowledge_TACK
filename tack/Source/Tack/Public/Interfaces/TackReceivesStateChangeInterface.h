#pragma once
#include "CoreMinimal.h"
#include "TackReceivesStateChangeInterface.generated.h"


UINTERFACE(MinimalAPI, BlueprintType)
class UTackReceivesStateChangeInterface : public UInterface
{
    GENERATED_BODY()
};

class TACK_API ITackReceivesStateChangeInterface
{
    GENERATED_BODY()
public:

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void OnTackStart();
    virtual void OnTackStart_Implementation() {}

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void OnTackEnd();
    virtual void OnTackEnd_Implementation() {}
};