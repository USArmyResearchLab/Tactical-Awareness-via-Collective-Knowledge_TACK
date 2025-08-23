#pragma once
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Components/ActorComponent.h"
#include "Interfaces/TackReceivesStateChangeInterface.h"
#include "TackGameModeBaseComponent.generated.h"

class UTackGameStateBaseComponent;

UCLASS(BlueprintType, Blueprintable, Transient, Within = GameModeBase)
class TACK_API UTackGameModeBaseComponent : public UActorComponent, public ITackReceivesStateChangeInterface
{
    GENERATED_BODY()
public:

    UTackGameModeBaseComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    //Helpers
public:

    AGameModeBase* GetGameMode() const { return GameModeBase; }
    AGameStateBase* GetGameState() const { return GetGameMode()->GameState; }

    template<typename TGameMode>
    TGameMode* GetGameMode() const { return Cast<TGameMode>(GameModeBase); }

    template<typename TGameState>
    TGameState* GetGameState() const { return GetGameMode()->GetGameState<TGameState>(); }


    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void OnTackStart();
    virtual void OnTackStart_Implementation() override;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void OnTackEnd();
    virtual void OnTackEnd_Implementation() override;

protected:

    virtual void OnGameModePostLoginEvent(AGameModeBase* GameMode, APlayerController* NewPlayer);
    virtual void OnGameModeLogoutEvent(AGameModeBase* GameMode, AController* NewPlayer);
    virtual void OnGameModeMatchStateSetEvent(FName MatchState);

    TSubclassOf<UTackGameStateBaseComponent> TackGameStateClassToSpawn;

private:

    AGameModeBase* GameModeBase;

    FDelegateHandle GameModePostLoginHandle;
    FDelegateHandle GameModeLogoutHandle;
    FDelegateHandle GameModeMatchStateSetEvent;

    void OnActorSpawnedEvent(AActor* Actor);
    TArray<AActor*> DelayedTackificationActors;
    FDelegateHandle ActorWorldSpawnedHandle;
};