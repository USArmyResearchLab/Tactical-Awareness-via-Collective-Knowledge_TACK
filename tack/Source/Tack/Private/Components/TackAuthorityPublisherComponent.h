#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "TackBaseComponent.h"
#include "TackAuthorityPublisherComponent.generated.h"

class AController;
class AActor;
class UDamageType;
class UTackStatics;
class AController;
class UPrimitiveComponent;
class UTackSettings;
class UTackManager;

UCLASS(BlueprintType, hidecategories(Tags, Collision, AssetUserData, Activation, Cooking))
class UTackAuthorityPublisherComponent : public UTackBaseComponent
{
    GENERATED_BODY()
public:
    UTackAuthorityPublisherComponent();

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);
    virtual bool NeedsLoadForClient() const override;

    virtual void OnTackStart_Implementation() override;
    virtual void OnTackEnd_Implementation() override;

private:

    void PublishActor() const;
    void PublishLifetimeEvent(const FString& Event, float EventTime) const;

    friend class UTackStatics;

    void SubscribeAllPublishers();
    void UnsubscribeAllPublishers();

    UPROPERTY()
    bool bSubscribed;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = Tack, meta = (AllowPrivateAccess = "true"))
    uint8 bEnableAIPreceptionPublishing : 1;

    UFUNCTION()
    void OnAITargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = Tack, meta = (AllowPrivateAccess = "true"))
    uint8 bEnableDamagePublishing : 1;

    uint8 bAlreadyPublishedCurrentDamage : 1;

    UFUNCTION()
    void OnActorTakePointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation, UPrimitiveComponent* HitComponent, FName BoneName, FVector ShotFromDirection, const UDamageType* DamageType, AActor* DamageCauser);
    UFUNCTION()
    void OnActorTakeRadialDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, FVector Origin, const FHitResult& HitInfo, AController* InstigatedBy, AActor* DamageCauser);
    UFUNCTION()
    void OnActorTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = Tack, meta = (AllowPrivateAccess = "true"))
    uint8 bEnableCollisionPublishing : 1;

    UFUNCTION()
    void OnComponentHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
    UFUNCTION()
    void OnComponentBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
    UFUNCTION()
    void OnComponentEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = Tack, meta = (AllowPrivateAccess = "true"))
    uint8 bEnableTransformPublishing : 1;

    void OnComponentTransformUpdate(USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport);
};