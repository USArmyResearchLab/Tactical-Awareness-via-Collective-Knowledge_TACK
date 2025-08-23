#include "Subsystems/TackWorldSubsystem.h"
#include "EngineUtils.h"
#include "TackStatics.h"
#include "Tack.h"
#include "TackSettings.h"
#include "ActorEditorUtils.h"

#include "GameFramework/PlayerController.h"
#include "AIController.h"

#include "TackManager.h"
#include "TackGenuineIdComponent.h"

#include "Interfaces/TackReceivesStateChangeInterface.h"
#include "Components/TackAuthorityPublisherComponent.h"
#include "Components/TackClientCameraPublisherComponent.h"
#include "Components/TackControllerInputComponent.h"
#include "Components/TackPlayerStateComponent.h"
#include "Components/TackControllerComponent.h"
#include "Devices/EyeTracker/TackEyeTrackerComponent.h"

DECLARE_CYCLE_STAT(TEXT("TackWorldSubsystem Tick"), STAT_TackWorldSubsystemTick, STATGROUP_Tack);

UTackWorldSubsystem::UTackWorldSubsystem()
{
    bWorldTackified = false;
}
UTackWorldSubsystem::~UTackWorldSubsystem() {}

TStatId UTackWorldSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UTackWorldSubsystem, STATGROUP_Tickables);
}

void UTackWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Settings = GetMutableDefault<UTackSettings>();

    ActorWorldSpawnedHandle = GetWorld()->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &UTackWorldSubsystem::OnActorSpawnedEvent));
    GetWorld()->OnActorsInitialized.AddLambda([this](const UWorld::FActorsInitializedParams& Params)
        {
            check(Params.World == GetWorld());
            TackManager = UTackManager::GetInstance(GetWorld());
            TackifyWorld();
        });
}

void UTackWorldSubsystem::Deinitialize()
{
    Super::Deinitialize();
    GetWorld()->RemoveOnActorSpawnedHandler(ActorWorldSpawnedHandle);
}

bool UTackWorldSubsystem::IsTackRunning() const
{
    return TackManager != nullptr && TackManager->IsTackRunning();
}

void UTackWorldSubsystem::OnActorSpawnedEvent(AActor* Actor)
{
    TWeakObjectPtr<AActor> ActorPtr(Actor);
    if(!ActorPtr.IsValid())
        return;

    if(ShouldDelayActorTackification(Actor))
    {
        DelayedTackificationActors.Add(ActorPtr);
    }
    else
    {
        TackifyActor(Actor);
    }
}

bool UTackWorldSubsystem::ShouldDelayActorTackification(AActor* Actor) const
{
#if WITH_EDITOR
    return Actor->GetWorld()->WorldType != EWorldType::Editor && (!Actor->IsActorInitialized() || (Actor->IsChildActor() && !Actor->GetParentActor()->IsActorInitialized()));
#else
    return !Actor->IsActorInitialized() || (Actor->IsChildActor() && !Actor->GetParentActor()->IsActorInitialized());
#endif
}

void UTackWorldSubsystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

#if WITH_EDITOR
    if(!bWorldTackified && GetWorld()->WorldType == EWorldType::Editor && GetWorld()->bIsWorldInitialized)
    {
        TackifyWorld();
    }
#endif

    // Filter out All invalid actors or actors that are ready to be tackified
    TArray<TWeakObjectPtr<AActor>> ActorsToBeTackified;
    DelayedTackificationActors.RemoveAll(
        [this, &ActorsToBeTackified](const TWeakObjectPtr<AActor>& Actor)
        {
            if(!Actor.IsValid())
                return true;

            if(!ShouldDelayActorTackification(Actor.Get()))
            {
                ActorsToBeTackified.Add(Actor);
                return true;
            }
            return false;
        }
    );

    // Tackify filtered actors
    for(TWeakObjectPtr<AActor> Actor : ActorsToBeTackified)
    {
        if(Actor.IsValid())
            TackifyActor(Actor.Get());
    }
}

void UTackWorldSubsystem::TackifyWorld()
{
    const EActorIteratorFlags Flags = EActorIteratorFlags::SkipPendingKill;
    for(TActorIterator<AActor> Itr(GetWorld(), AActor::StaticClass(), Flags); Itr; ++Itr)
    {
        TackifyActor(*Itr);
    }
    bWorldTackified = true;
}

void UTackWorldSubsystem::TackifyActor(AActor* Actor)
{
    //Check if Actor is valid for TackComponent
    if(!UTackStatics::IsActorValidForTackComponent(Actor))
    {
#if WITH_EDITOR
        //If we are in the editor we should detackify the actor
        if(GetWorld()->WorldType == EWorldType::Editor)
            DeTackifyActor(Actor);
#endif
        return;
    }

    //Can only tackify Authority
    if(!Actor->HasAuthority())
        return;

    if(ShouldDelayActorTackification(Actor))
    {
        UE_LOG(LogTack, Verbose, TEXT("Actor (%s) is not fully initialized delaying until next frame"), *Actor->GetName());
        DelayedTackificationActors.Add(Actor);
        return;
    }

    if(UTackIdComponent* TackIdComponent = AddOrGetTackIdComponent(Actor))
    {
        //Components that should only spawn on GameWorlds
        if(GetWorld()->IsGameWorld())
        {
            AddAuthorityPublisherComponent(Actor, TackIdComponent);
        }
    }
    else
    {
        if(TackManager)
        {
            UE_LOG(LogTack, Warning, TEXT("%s - Could not tackify actor %s delaying until next frame"), *TackManager->GetTackLogPrefix(GetWorld()), *Actor->GetName());
        }
        else
        {
            UE_LOG(LogTack, Warning, TEXT("Editor - Could not tackify actor %s delaying until next frame"), *Actor->GetName());
        }
        DelayedTackificationActors.Add(Actor);
    }
    return;
}

void UTackWorldSubsystem::DeTackifyActor(AActor* Actor)
{
    //remove All components of UTackComponentInterface
    for(UActorComponent* Component : Actor->GetComponentsByInterface(UTackComponentInterface::StaticClass()))
    {
        Component->DestroyComponent(false);
    }
}

void UTackWorldSubsystem::RegisterBaseComponent(AActor* Actor, UTackBaseComponent* Component, UTackIdComponent* IdComponent)
{
    if(Component != nullptr)
    {

#if WITH_EDITOR
        //If we are in an editor world We will add an instance of said component
        if(Actor->GetWorld()->WorldType == EWorldType::Editor)
            Actor->AddInstanceComponent(Component);
#endif

        if(!Component->HasValidTackId())
            Component->SetTackIdComponent(IdComponent);

        if(!Component->IsRegistered())
        {
            Component->RegisterComponent();

            UE_LOG(
                LogTack, Verbose, TEXT("%s - Registered TackComponent [Actor Class: %s] [Actor Name: %s] [Tack Class: %s] [Tack Id: %s]"),
                TackManager != nullptr ? *TackManager->GetTackLogPrefix(GetWorld()) : TEXT("Editor"),
                *Actor->GetClass()->GetName(),
                *Actor->GetName(),
                *Component->GetClass()->GetName(),
                *Component->GetTackId().ToString(EGuidFormats::DigitsWithHyphens)
            );
        }
    }
}

UTackIdComponent* UTackWorldSubsystem::AddOrGetTackIdComponent(AActor* Actor)
{
    //Actor Should be initialized by this point
    check(Actor->GetWorld()->WorldType == EWorldType::Editor || Actor->IsActorInitialized());
    //Only Authority Actors can be tackified
    check(Actor->HasAuthority());


    //Attempt to first find an existing TackIdComponent
#if UE_BUILD_SHIPPING
    //In shipping builds We do a slight optimization and don't loop through every
    //component. Assuming there is only one
    UTackIdComponent* TackIdComponent = Actor->FindComponentByClass<UTackIdComponent>();
#else
    //If we are not in a shipping build we make sure there is only one TackIdComponent per Actor.
    UTackIdComponent* TackIdComponent = nullptr;
    Actor->ForEachComponent<UTackIdComponent>(false, [&TackIdComponent](UTackIdComponent* Component) {
        check(TackIdComponent == nullptr);
        TackIdComponent = Component;
        });
#endif

    //If we failed to find a TackComponent on an Actor in a GameWorld that is marked NetStartup tackification has failed.
    //However there is an edge case where an actor is Transient but marked as a NetStartup (ADefaultPhysicsVolume, ALevelScriptActor, etc). 
    //These actors also have bEditable=false marked as well but I don't belive thats the issue.
    //We do an extra check to make sure its a NetStartupActor && its not transient to handle the above edge case
   // checkf(TackIdComponent != nullptr || !Actor->HasAnyFlags(RF_WasLoaded) || Actor->HasAnyFlags(RF_Transient) || Actor->GetWorld()->WorldType == EWorldType::Editor || !Actor->IsNetStartupActor(), TEXT("Actor: %s"), *AActor::GetDebugName(Actor));
    //checkf(Actor->GetWorld()->WorldType == EWorldType::Editor || !(TackIdComponent == nullptr && Actor->HasAnyFlags(RF_WasLoaded)) || Actor->HasAnyFlags(RF_Transient), TEXT("Actor: %s"), *AActor::GetDebugName(Actor));
    //Above check causing more headace then worth

    //If we did not find A TackIdComponent we create one
    if(TackIdComponent == nullptr)
    {
        //If this list start to expand we can shift this into a factory pattern
        //It probably will stay fairly small though
        UClass* TackIdClass = nullptr;
        if(Actor->IsA<AAIController>())
        {
            TackIdClass = UTackControllerComponent::StaticClass();
        }
        else
        {
            TackIdClass = UTackGenuineIdComponent::StaticClass();
        }

        TackIdComponent = NewObject<UTackIdComponent>(Actor, TackIdClass, TEXT("TackIdComponent"));

        //TackIdComponent should be valid at this point
        check(TackIdComponent)

#if WITH_EDITOR
            //If we are in an editor world We will add an instance of the TackIdComponent
            if(Actor->GetWorld()->WorldType == EWorldType::Editor)
            {
                Actor->AddInstanceComponent(TackIdComponent);
            }
#endif

        //If we arn't registerd We will register the component and Log
        if(!TackIdComponent->IsRegistered())
        {
            TackIdComponent->RegisterComponent();

            UE_LOG(
                LogTack, Log, TEXT("%s - Registered TackId [Actor Class: %s] [Actor Name: %s] [Tack Class: %s] [Tack Id: %s]"),
                TackManager != nullptr ? *TackManager->GetTackLogPrefix(GetWorld()) : TEXT("Editor"),
                *Actor->GetClass()->GetName(),
                *Actor->GetName(),
                *TackIdComponent->GetClass()->GetName(),
                *TackIdComponent->GetTackId().ToString(EGuidFormats::DigitsWithHyphens)
            );
        }
    }
    return TackIdComponent;
}

void UTackWorldSubsystem::AddAuthorityPublisherComponent(AActor* Actor, UTackIdComponent* TackIdComponent)
{
    UTackAuthorityPublisherComponent* Component = Actor->FindComponentByClass<UTackAuthorityPublisherComponent>();

    if(Actor->HasAuthority())
    {
        if(Component == nullptr)
        {
            Component = NewObject<UTackAuthorityPublisherComponent>(Actor, TEXT("TackAuthorityPublisher"));
        }

        if(Component != nullptr)
        {
            RegisterBaseComponent(Actor, Component, TackIdComponent);
        }
        else
        {
            UE_LOG(LogTack, Error, TEXT("Failed to Spawn class(TackAuthorityPublisher) on Actor(%s)"), *Actor->GetName());
        }
    }
    else if(Component != nullptr && Actor->GetWorld()->IsGameWorld())
    {
        Component->DestroyComponent();
    }
}