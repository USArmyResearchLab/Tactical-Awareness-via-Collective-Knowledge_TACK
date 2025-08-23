
#pragma once
#include "Components/ActorComponent.h"
#include "TackIdComponent.h"
#include "TackControllerComponent.generated.h"

class AController;
class UTackManager;
class UTackPlayerStateComponent;

UCLASS(Within = Controller, BlueprintType, hidecategories(Tags, Collision, AssetUserData, Activation, Cooking))
class UTackControllerComponent : public UTackIdComponent
{
    GENERATED_BODY()
public:

    UTackControllerComponent();
    void TackControllerInit();

    virtual void PostNetReceive() override;
    virtual void OnComponentCreated() override;
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable)
    UTackManager* GetTackManager() const { return TackManager; }

    UFUNCTION(Server, Reliable)
    void ServerStartTack();

    UFUNCTION(Server, Reliable)
    void ServerStopTack();

    UFUNCTION(Client, Reliable)
    void ClientStartLocalTack(const FGuid& SessionId);

    UFUNCTION(Client, Reliable)
    void ClientStopLocalTack();

    virtual const FGuid& GetTackId() const override { return Guid; }
    virtual bool HasValidTackId() const override { return Guid.IsValid(); }

    void ShouldStartTackOnBeginPlay(bool bNewStartTackOnBeginPlay);

private:

    bool bLocallyInitialized;

    void RequestServerToStartLocalClientTack();

    UFUNCTION(Server, Reliable)
    void ServerStartLocalTack();

    UPROPERTY(Transient)
    AController* OwningController;

    UPROPERTY(Transient)
    UTackManager* TackManager;

    UPROPERTY(Replicated)
    UTackPlayerStateComponent* TackPlayerStateComponent;

    UPROPERTY(Replicated)
    bool bStartTackOnBeginPlay;

    UPROPERTY(Replicated, NonPIEDuplicateTransient, VisibleInstanceOnly, SaveGame)
    FGuid Guid;

    void AddExtraLocalControllerComponents();

    void AttemptToAddEyeTrackerComponentToLocalPlayer();
};