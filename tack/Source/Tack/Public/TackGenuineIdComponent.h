#pragma once
#include "TackIdComponent.h"
#include "Components/ActorComponent.h"
#include "TackGenuineIdComponent.generated.h"


USTRUCT()
struct TACK_API FTackGenuineIdComponentInstanceData : public FActorComponentInstanceData
{
    GENERATED_BODY()

    FTackGenuineIdComponentInstanceData() = default;
    FTackGenuineIdComponentInstanceData(const class UTackGenuineIdComponent* SourceComponent);

    virtual ~FTackGenuineIdComponentInstanceData() = default;

    virtual bool ContainsData() const override;
    virtual void ApplyToComponent(UActorComponent* Component, const ECacheApplyPhase CacheApplyPhase) override;

    UPROPERTY()
    FGuid Guid;
};


UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent), hidecategories("Variable", "ComponentTick", ComponentReplication, Tags, Collision, AssetUserData, Activation, Cooking))
class TACK_API UTackGenuineIdComponent : public UTackIdComponent
{
    GENERATED_BODY()
public:
    UTackGenuineIdComponent();

    //Verifys Clients have a TackId when Components replicate from server 
    virtual void PostNetReceive() override;

    //Initializes TackId
    virtual void OnComponentCreated() override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual TStructOnScope<FActorComponentInstanceData> GetComponentInstanceData() const override;

    // TackComponentInterface
    virtual const FGuid& GetTackId() const override { return Guid; }
    virtual bool HasValidTackId() const override { return Guid.IsValid(); }

private:
    friend FTackGenuineIdComponentInstanceData;
    UPROPERTY(Replicated, NonPIEDuplicateTransient, VisibleInstanceOnly, SaveGame)
    FGuid Guid;
};