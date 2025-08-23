#pragma once
#include "Components/ActorComponent.h"
#include "Tack.h"
#include "Interfaces/TackReceivesStateChangeInterface.h"
#include "TackGameStateBaseComponent.generated.h"

class UTackManager;
class UTackControllerComponent;

UCLASS(BlueprintType, Blueprintable, Transient, Within = GameStateBase)
class TACK_API UTackGameStateBaseComponent : public UActorComponent, public ITackReceivesStateChangeInterface
{
    GENERATED_BODY()

public:

    UTackGameStateBaseComponent();
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(Replicated)
    FString ClientKafkaConnectionString;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void OnTackStart();
    virtual void OnTackStart_Implementation() override;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void OnTackEnd();
    virtual void OnTackEnd_Implementation() override;

public:

    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    UFUNCTION()
    void OnGameModeMatchStateSetEvent(FName MatchState);

    UTackManager* TackManager;
};