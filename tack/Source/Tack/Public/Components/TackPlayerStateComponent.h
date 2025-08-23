#pragma once
#include "Components/ActorComponent.h"
#include "TackPlayerStateComponent.generated.h"


UCLASS(Within = PlayerState, BlueprintType, hidecategories(Tags, Collision, AssetUserData, Activation, Cooking))
class UTackPlayerStateComponent : public UTackIdComponent
{
    GENERATED_BODY()
public:

    UTackPlayerStateComponent();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    //Verify the Client has recieved the Guid
    virtual void PostNetReceive() override;
    //Verifys we Server has Set the PlayerStateGuid 
    virtual void BeginPlay() override;

    //Copy Guid from TackControllerComponent
    void SetTackId(const FGuid& PlayerControllerTackGuid);

    virtual const FGuid& GetTackId() const override { return Guid; }
    virtual bool HasValidTackId() const override { return Guid.IsValid(); }

private:

    UPROPERTY(Replicated, NonPIEDuplicateTransient, VisibleInstanceOnly, SaveGame)
    FGuid Guid;
};