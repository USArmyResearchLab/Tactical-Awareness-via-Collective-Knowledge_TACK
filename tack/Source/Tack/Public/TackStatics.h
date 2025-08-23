#pragma once
#include "TackIdComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include <chrono>
#include "TackStatics.generated.h"

class AActor;
class UTackSettings;
class UTackAuthorityPublisherComponent;
class UTackManager;
class AController;
class UTackGameStateBaseComponent;


UCLASS()
class TACK_API UTackStatics : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public: //General Methods

    UFUNCTION(BlueprintCallable)
    static FORCEINLINE int64 GetCurrentTimestamp()
    {
        using namespace std::chrono;
        return duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
    }

    //Tack component Methods

    static UTackGameStateBaseComponent* GetTackGameStateBaseComponent(const UObject* WorldContextObject);

    static bool IsActorValidForTackComponent(const AActor* Actor);

    UFUNCTION(BlueprintCallable)
    static FORCEINLINE UTackIdComponent* GetTackIdComponentFromActor(const AActor* Actor)
    {
        return Actor != nullptr ? Actor->FindComponentByClass<UTackIdComponent>() : nullptr;
    }

    UFUNCTION(BlueprintCallable)
    static FORCEINLINE bool IsActorTackified(const AActor* Actor)
    {
        return GetTackIdFromActor(Actor).IsValid();
    }

    UFUNCTION(BlueprintCallable)
    static const FGuid& GetTackIdFromActor(const AActor* Actor);
};