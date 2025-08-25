#include "Components/TackAuthorityPublisherComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Actor.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/World.h"
#include "Serialization/TackJsonObjectSerializer.h"
#include "Serialization/TackJsonDomBuilder.h"
#include "TackSettings.h"
#include "TackStatics.h"
#include "TackManager.h"
#include "TackPublisher.h"

#include "Perception/AIPerceptionComponent.h"

DECLARE_CYCLE_STAT(TEXT("Component Transform Publish"), STAT_Component_TransformPublish, STATGROUP_Tack);
DECLARE_CYCLE_STAT(TEXT("Actor Damage Publish"), STAT_Actor_DamagePublish, STATGROUP_Tack);
DECLARE_CYCLE_STAT(TEXT("Component Hit Publish"), STAT_Component_HitPublish, STATGROUP_Tack);
DECLARE_CYCLE_STAT(TEXT("Component Overlap Publish"), STAT_Component_OverlapPublish, STATGROUP_Tack);


UTackAuthorityPublisherComponent::UTackAuthorityPublisherComponent() : UTackBaseComponent()
{
    bSubscribed = false;

    SetIsReplicatedByDefault(false);
    //Set defaults
    bAlreadyPublishedCurrentDamage = false;

    bEnableAIPreceptionPublishing = true;
    bEnableDamagePublishing = true;
    bEnableCollisionPublishing = true;
    bEnableTransformPublishing = true;
}

bool UTackAuthorityPublisherComponent::NeedsLoadForClient() const
{
    return false;
}

void UTackAuthorityPublisherComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    //fix for clients crashing on seamless server travel
    if(GetOwner()->HasAuthority())
    {
        if(GetWorld()->GetGameState() != nullptr)
        {
            PublishLifetimeEvent(TEXT("Destroyed"), GetWorld()->GetGameState()->GetServerWorldTimeSeconds());
        }
        UnsubscribeAllPublishers();
    }
}

void UTackAuthorityPublisherComponent::OnTackStart_Implementation()
{
    Super::OnTackStart_Implementation();
    if(GetOwner()->HasAuthority())
    {
        this->PublishActor();
        SubscribeAllPublishers();
    }
}

void UTackAuthorityPublisherComponent::OnTackEnd_Implementation()
{
    Super::OnTackEnd_Implementation();
    if(GetOwner()->HasAuthority())
    {
        UnsubscribeAllPublishers();
    }
}

void UTackAuthorityPublisherComponent::SubscribeAllPublishers()
{
    AActor* Owner = GetOwner();

    check(Owner->HasAuthority());

    if(bSubscribed)
        return;

    auto Settings = GetDefault<UTackSettings>();
    //Damage Publisher
    if(bEnableDamagePublishing && Settings->bEnableActorDamagePublisher)
    {
        Owner->OnTakePointDamage.AddUniqueDynamic(this, &UTackAuthorityPublisherComponent::OnActorTakePointDamage);
        Owner->OnTakeRadialDamage.AddUniqueDynamic(this, &UTackAuthorityPublisherComponent::OnActorTakeRadialDamage);
        Owner->OnTakeAnyDamage.AddUniqueDynamic(this, &UTackAuthorityPublisherComponent::OnActorTakeAnyDamage);
    }

    //AI Publisher
    if(bEnableAIPreceptionPublishing && Settings->bEnableAIPerceptionPublish)
    {
        if(UAIPerceptionComponent* aic = Owner->FindComponentByClass<UAIPerceptionComponent>())
        {
            aic->OnTargetPerceptionUpdated.AddDynamic(this, &UTackAuthorityPublisherComponent::OnAITargetPerceptionUpdated);
        }
    }

    auto OwnerRootComponent = Owner->GetRootComponent();
    Owner->ForEachComponent<USceneComponent>(false, [this, Settings, OwnerRootComponent](USceneComponent* SceneComponent)
        {
#if WITH_EDITORONLY_DATA
            if(SceneComponent->IsEditorOnly())
            {
                return;
            }
#endif

            if(SceneComponent->IsA<UChildActorComponent>())
            {
                return;
            }

            //Transform Publisher
            if(bEnableTransformPublishing && Settings->bEnableActorTransformPublisher)
            {
                //Only publish transform root if setting is set
                if(SceneComponent->Mobility != EComponentMobility::Static && (!Settings->bOnlyPublishRootComponentTransform || OwnerRootComponent == SceneComponent))
                {
                    SceneComponent->TransformUpdated.AddUObject(this, &UTackAuthorityPublisherComponent::OnComponentTransformUpdate);
                }
            }

            //Collision Publisher
            if(bEnableCollisionPublishing && Settings->bEnableActorCollisionPublisher)
            {
                if(UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(SceneComponent))
                {
                    PrimitiveComponent->OnComponentHit.AddUniqueDynamic(this, &UTackAuthorityPublisherComponent::OnComponentHit);

                    if(PrimitiveComponent->GetGenerateOverlapEvents())
                    {
                        PrimitiveComponent->OnComponentBeginOverlap.AddUniqueDynamic(this, &UTackAuthorityPublisherComponent::OnComponentBeginOverlap);
                        PrimitiveComponent->OnComponentEndOverlap.AddUniqueDynamic(this, &UTackAuthorityPublisherComponent::OnComponentEndOverlap);
                    }

                }
            }
        });

    bSubscribed = true;
}

void UTackAuthorityPublisherComponent::UnsubscribeAllPublishers()
{
    AActor* Owner = GetOwner();

    check(Owner->HasAuthority());

    if(!bSubscribed)
        return;

    Owner->OnTakePointDamage.RemoveDynamic(this, &UTackAuthorityPublisherComponent::OnActorTakePointDamage);
    Owner->OnTakeRadialDamage.RemoveDynamic(this, &UTackAuthorityPublisherComponent::OnActorTakeRadialDamage);
    Owner->OnTakeAnyDamage.RemoveDynamic(this, &UTackAuthorityPublisherComponent::OnActorTakeAnyDamage);

    if(UAIPerceptionComponent* aic = Owner->FindComponentByClass<UAIPerceptionComponent>())
    {
        aic->OnTargetPerceptionUpdated.RemoveDynamic(this, &UTackAuthorityPublisherComponent::OnAITargetPerceptionUpdated);
    }

    Owner->ForEachComponent<USceneComponent>(false, [this](USceneComponent* SceneComponent)
        {
            //Remove Transform
            SceneComponent->TransformUpdated.RemoveAll(this);

            //Remove collision
            if(UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(SceneComponent))
            {
                PrimitiveComponent->OnComponentHit.RemoveDynamic(this, &UTackAuthorityPublisherComponent::OnComponentHit);
                PrimitiveComponent->OnComponentBeginOverlap.RemoveDynamic(this, &UTackAuthorityPublisherComponent::OnComponentBeginOverlap);
                PrimitiveComponent->OnComponentEndOverlap.RemoveDynamic(this, &UTackAuthorityPublisherComponent::OnComponentEndOverlap);
            }
        }
    );

    bSubscribed = false;
}

void UTackAuthorityPublisherComponent::PublishActor() const
{
    AActor* Owner = GetOwner();
    check(Owner->HasAuthority());
    if(GetTackManager()->IsTackRunning())
    {
        GetTackManager()->GetPublisher()->Publish_Json(
            TEXT("unreal.actor"),
            FTackObjectSerializer::SerializeObject(Owner).AsJsonObject()
        );

        PublishLifetimeEvent(TEXT("Spawned"), Owner->CreationTime);
    }
}

void UTackAuthorityPublisherComponent::PublishLifetimeEvent(const FString& Event, float EventTime) const
{
    FTackJsonDomBuilder::FTackObject Json;
    Json.Set("Actor_id", GetTackId());
    Json.Set("Event", Event);
    Json.Set("ServerWorldTime", EventTime);

    GetTackManager()->GetPublisher()->Publish_Json(TEXT("unreal.actor.lifetime"), Json.AsJsonObject());
}

void UTackAuthorityPublisherComponent::OnActorTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
    if(!bAlreadyPublishedCurrentDamage)
    {

        SCOPE_CYCLE_COUNTER(STAT_Actor_DamagePublish);

        //On Take Any damage Json
        FTackJsonDomBuilder::FTackObject Json;
        Json.Set("DamagedActor", DamagedActor);
        Json.Set("Damage", Damage);
        Json.Set("InstigatedBy", InstigatedBy);
        Json.Set("DamageCauser", DamageCauser);
        Json.Set("DamageType", FTackObjectSerializer::SerializeObject(DamageType));
        Json.Set("ServerWorldTime", GetWorld()->GetGameState()->GetServerWorldTimeSeconds());

        GetTackManager()->GetPublisher()->Publish_Json(TEXT("unreal.actor.damage"), Json.AsJsonObject());
    }

    bAlreadyPublishedCurrentDamage = false;
}

void UTackAuthorityPublisherComponent::OnActorTakePointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation, UPrimitiveComponent* HitComponent, FName BoneName, FVector ShotFromDirection, const UDamageType* DamageType, AActor* DamageCauser)
{
    SCOPE_CYCLE_COUNTER(STAT_Actor_DamagePublish);

    //On Take Any damage Json
    FTackJsonDomBuilder::FTackObject Json;
    Json.Set("DamagedActor", DamagedActor);
    Json.Set("Damage", Damage);
    Json.Set("InstigatedBy", InstigatedBy);
    Json.Set("DamageCauser", DamageCauser);
    Json.Set("DamageType", FTackObjectSerializer::SerializeObject(DamageType));
    Json.Set("ServerWorldTime", GetWorld()->GetGameState()->GetServerWorldTimeSeconds());

    //On Take Point Damage Json
    Json.Set("HitLocation", HitLocation);
    Json.Set("HitComponent", HitComponent);
    Json.Set("BoneName", BoneName);
    Json.Set("ShotFromDirection", ShotFromDirection);

    GetTackManager()->GetPublisher()->Publish_Json(TEXT("unreal.actor.damage"), Json.AsJsonObject());

    bAlreadyPublishedCurrentDamage = true;
}

void UTackAuthorityPublisherComponent::OnActorTakeRadialDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, FVector Origin, const FHitResult& HitInfo, AController* InstigatedBy, AActor* DamageCauser)
{
    SCOPE_CYCLE_COUNTER(STAT_Actor_DamagePublish);

    //On Take Any damage Json
    FTackJsonDomBuilder::FTackObject Json;
    Json.Set("DamagedActor", DamagedActor);
    Json.Set("Damage", Damage);
    Json.Set("InstigatedBy", InstigatedBy);
    Json.Set("DamageCauser", DamageCauser);
    Json.Set("DamageType", FTackObjectSerializer::SerializeObject(DamageType));
    Json.Set("ServerWorldTime", GetWorld()->GetGameState()->GetServerWorldTimeSeconds());

    //On Take Radial Damage Json
    Json.Set("Origin", Origin);
    Json.Set("HitInfo", HitInfo);

    GetTackManager()->GetPublisher()->Publish_Json(TEXT("unreal.actor.damage"), Json.AsJsonObject());

    bAlreadyPublishedCurrentDamage = true;
}

void UTackAuthorityPublisherComponent::OnComponentHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    SCOPE_CYCLE_COUNTER(STAT_Component_HitPublish);
    // not sure why Other would ever be null but this is the examples that I saw
    if((OtherActor != NULL) && (OtherComp != NULL))
    {
        return;
    }

    const FGuid& OtherTackId = UTackStatics::GetTackIdFromActor(OtherActor);
    if(OtherTackId.IsValid())
    {
        FTackJsonDomBuilder::FTackObject Json;
        Json.Set("HitActor_id", GetTackId());
        Json.Set("HitComp", HitComp->GetName());
        Json.Set("OtherActor_id", OtherTackId);
        Json.Set("OtherComp", OtherComp->GetName());
        Json.Set("NormalImpulse", NormalImpulse);
        Json.Set("Hit", Hit);
        Json.Set("ServerWorldTime", GetWorld()->GetGameState()->GetServerWorldTimeSeconds());

        GetTackManager()->GetPublisher()->Publish_Json(TEXT("unreal.actor.component.hit"), Json.AsJsonObject());
    }
}

void UTackAuthorityPublisherComponent::OnComponentBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    SCOPE_CYCLE_COUNTER(STAT_Component_OverlapPublish);

    const FGuid& OtherTackId = UTackStatics::GetTackIdFromActor(Other);
    if(OtherTackId.IsValid())
    {
        FTackJsonDomBuilder::FTackObject Json;
        Json.Set("Type", "BeginOverlap");
        Json.Set("OverlappedActor_id", GetTackId());
        Json.Set("OverlappedComp", OverlappedComp->GetName());
        Json.Set("OtherActor_id", OtherTackId);
        Json.Set("OtherComp", OtherComp->GetName());
        Json.Set("OtherBodyIndex", OtherBodyIndex);
        Json.Set("ServerWorldTime", GetWorld()->GetGameState()->GetServerWorldTimeSeconds());

        GetTackManager()->GetPublisher()->Publish_Json(TEXT("unreal.actor.component.overlap"), Json.AsJsonObject());
    }
}

void UTackAuthorityPublisherComponent::OnComponentEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    SCOPE_CYCLE_COUNTER(STAT_Component_OverlapPublish);

    const FGuid& OtherTackId = UTackStatics::GetTackIdFromActor(Other);
    if(OtherTackId.IsValid())
    {
        FTackJsonDomBuilder::FTackObject Json;
        Json.Set("Type", "EndOverlap");
        Json.Set("OverlappedActor_id", GetTackId());
        Json.Set("OverlappedComp", OverlappedComp->GetName());
        Json.Set("OtherActor_id", OtherTackId);
        Json.Set("OtherComp", OtherComp->GetName());
        Json.Set("OtherBodyIndex", OtherBodyIndex);
        Json.Set("ServerWorldTime", GetWorld()->GetGameState()->GetServerWorldTimeSeconds());

        GetTackManager()->GetPublisher()->Publish_Json(TEXT("unreal.actor.component.overlap"), Json.AsJsonObject());
    }
}

void UTackAuthorityPublisherComponent::OnAITargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    //no expiration age for now since it's a protected variable with no getter
    FTackJsonDomBuilder::FTackObject Json;
    Json.Set("SensingActor_id", GetTackId());
    Json.Set("DetectedActor_id", UTackStatics::GetTackIdFromActor(Actor));
    Json.Set("Age", Stimulus.GetAge());
    Json.Set("Strength", Stimulus.Strength);
    Json.Set("StimulusLocation", Stimulus.StimulusLocation);
    Json.Set("ReceiverLocation", Stimulus.ReceiverLocation);
    Json.Set("Tag", Stimulus.Tag);
    Json.Set("SuccessfullySensed", Stimulus.WasSuccessfullySensed());

    GetTackManager()->GetPublisher()->Publish_Json(TEXT("unreal.aiperception"), Json.AsJsonObject());
}

void UTackAuthorityPublisherComponent::OnComponentTransformUpdate(USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
    SCOPE_CYCLE_COUNTER(STAT_Component_TransformPublish);

    const bool bFromParent = !!(UpdateTransformFlags & EUpdateTransformFlags::PropagateFromParent);

    if(bFromParent)
        return;

    FTackJsonDomBuilder::FTackObject Json;
    Json.Set("Actor_id", GetTackId());
    Json.Set("Component", UpdatedComponent->GetName());

    AActor* AttachmentRootActor = UpdatedComponent->GetAttachmentRootActor();
    Json.Set("AttachmentRootActor_id", UTackStatics::GetTackIdFromActor(AttachmentRootActor));

    if(APawn* Pawn = Cast<APawn>(AttachmentRootActor))
    {
        Json.Set("Controller_id", UTackStatics::GetTackIdFromActor(Pawn->GetController()));
    }
    else
    {
        Json.Set("Controller_id", nullptr);
    }

    if(USceneComponent* AttachmentParent = UpdatedComponent->GetAttachParent())
    {
        Json.Set("AttachmentParentComponent", AttachmentParent->GetName());
        Json.Set("bRoot", false);
    }
    else
    {
        Json.Set("AttachmentParentComponent", nullptr);
        Json.Set("bRoot", true);
    }

    Json.Set("Transform", UpdatedComponent->GetComponentTransform());
    Json.Set("ServerWorldTime", GetWorld()->GetGameState()->GetServerWorldTimeSeconds());

    switch(UpdateTransformFlags)
    {
    case EUpdateTransformFlags::None:                   Json.Set("UpdateTransformFlags", TEXT("None")); break;
    case EUpdateTransformFlags::SkipPhysicsUpdate:      Json.Set("UpdateTransformFlags", TEXT("SkipPhysicsUpdate")); break;
    case EUpdateTransformFlags::PropagateFromParent:    Json.Set("UpdateTransformFlags", TEXT("PropagateFromParent")); break;
    case EUpdateTransformFlags::OnlyUpdateIfUsingSocket:Json.Set("UpdateTransformFlags", TEXT("OnlyUpdateIfUsingSocket")); break;
    }

    Json.Set("TeleportType", Teleport);

    GetTackManager()->GetPublisher()->Publish_Json(TEXT("unreal.actor.component.transform"), Json.AsJsonObject());
}