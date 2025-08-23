#pragma once
#include "Components/ActorComponent.h"
#include "GameplayTags.h"
#include "TackTagsComponent.generated.h"

UCLASS(BlueprintType, Blueprintable, hidecategories(Tags, Collision, AssetUserData, Activation, Cooking), meta = (BlueprintSpawnableComponent))
class TACK_API UTackTagsComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UTackTagsComponent();

    UPROPERTY(EditAnywhere, Replicated)
    FGameplayTagContainer Tags;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};