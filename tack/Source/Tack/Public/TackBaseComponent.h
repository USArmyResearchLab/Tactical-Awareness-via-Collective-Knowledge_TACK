#pragma once
#include "Components/ActorComponent.h"
#include "Interfaces/TackComponentInterface.h"
#include "Interfaces/TackReceivesStateChangeInterface.h"
#include "TackBaseComponent.generated.h"

class UTackManager;
class UTackIdComponent;

UCLASS(Abstract, Blueprintable)
class TACK_API UTackBaseComponent : public UActorComponent, public ITackComponentInterface, public ITackReceivesStateChangeInterface
{
    GENERATED_BODY()

public:

    UTackBaseComponent();

    virtual void InitializeComponent() override;
    virtual void BeginPlay() override;
    virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, BlueprintPure)
    UTackManager* GetTackManager() const { return TackManager; }

    UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DisplayName = "Get Tack Id of Owning Actor"))
    const FGuid& BP_GetTackId() { return GetTackId(); }

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void OnTackStart();
    virtual void OnTackStart_Implementation() override {};

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void OnTackEnd();
    virtual void OnTackEnd_Implementation() override {};

    virtual const FGuid& GetTackId() const override;
    virtual bool HasValidTackId() const override;

    // Setter functions. Must use before BeginPlay
    void SetTackManager(UTackManager* NewTackManager);
    void SetTackIdComponent(UTackIdComponent* NewTackIdComponent);

private:

    UPROPERTY()
    bool bAutoCallTackStart;

    UPROPERTY(Transient)
    UTackManager* TackManager;

    UPROPERTY(Replicated, VisibleInstanceOnly, SaveGame)
    UTackIdComponent* TackIdComponent;
};