#include "TackManager.h"
#include "TackPublisher.h"
#include "TackSettings.h"
#include "TackStatics.h"
#include "Subsystems/TackWorldSubsystem.h"
#include "Components/TackGameStateBaseComponent.h"
#include "Components/TackControllerComponent.h"
#include "Components/TackPlayerStateComponent.h"
#include "Interfaces/TackReceivesStateChangeInterface.h"

#include "Engine/World.h"
#include "UObject/UObjectIterator.h"
#include "GameFramework/Actor.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"


UTackManager* UTackManager::GetInstance(const UObject* WorldContextObject)
{
    if(UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
    {
        if(UGameInstance* GameInstance = World->GetGameInstance())
        {
            return GameInstance->GetSubsystem<UTackManager>();
        }
    }
    return nullptr;
}

UTackManager::UTackManager() : UGameInstanceSubsystem()
{

}

FString UTackManager::GetTackLogPrefix(UWorld* World, bool bIncludeConnectionString) const
{
    check(World != nullptr);

    FString NetModeString = ToString(World->GetNetMode());
    //  switch(World->GetNetMode()){
    //      case NM_Standalone:         NetModeString = TEXT("Standalone"); break;
    //      case NM_DedicatedServer:    NetModeString = TEXT("DedicatedServer"); break;
    //      case NM_ListenServer:       NetModeString = TEXT("ListenServer"); break;
    //      case NM_Client:             NetModeString = TEXT("Client"); break;
    //      default:                    NetModeString = TEXT("Max"); break;
    //  }

    if(bIncludeConnectionString)
    {
        return FString::Printf(TEXT("%s - Instance (%s) Session (%s) Connection (%s)"), *NetModeString, *GetLocalInstanceId().ToString(), *GetCurrentSessionId().ToString(), *GetKafkaConnectionString());
    }
    else
    {
        return FString::Printf(TEXT("%s - Instance (%s) Session (%s)"), *NetModeString, *GetLocalInstanceId().ToString(), *GetCurrentSessionId().ToString());
    }
}

/** Implement this for initialization of instances of the system */
void UTackManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Publisher = Collection.InitializeDependency<UTackPublisher>();
    LocalInstanceId = FGuid::NewGuid();

    if(GetGameInstance()->GetWorld() == nullptr || GetGameInstance()->GetWorld()->GetNetMode() < NM_Client)
    {
        //GameMode Events
        FGameModeEvents::OnGameModeInitializedEvent().AddUObject(this, &UTackManager::OnGameModeInitialized);
        FGameModeEvents::OnGameModePostLoginEvent().AddUObject(this, &UTackManager::OnGameModePostLoginEvent);

        //GameInstance Events
        GetGameInstance()->GetOnPawnControllerChanged().AddDynamic(this, &UTackManager::OnPawnControllerChanged);
    }

    GetGameInstance()->OnNotifyPreClientTravel().AddUObject(this, &UTackManager::OnNotifyPreClientTravel);
    GetGameInstance()->GetEngine()->OnTravelFailure().AddUObject(this, &UTackManager::OnTravelFailure);
    //World Events
    FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &UTackManager::OnWorldBeginTearingDown);
}

void UTackManager::Deinitialize()
{
    Super::Deinitialize();
    //GameMode Events
    FGameModeEvents::OnGameModeInitializedEvent().RemoveAll(this);
    FGameModeEvents::OnGameModePostLoginEvent().RemoveAll(this);

    //World Events
    FWorldDelegates::OnWorldBeginTearDown.RemoveAll(this);

    //GameInstance Events
    GetGameInstance()->OnNotifyPreClientTravel().RemoveAll(this);
    GetGameInstance()->GetOnPawnControllerChanged().RemoveAll(this);
    GetGameInstance()->GetEngine()->OnTravelFailure().RemoveAll(this);
    Publisher->DisablePublisher();
}

void UTackManager::OnGameModePostLoginEvent(AGameModeBase* GameMode, APlayerController* NewPlayer)
{
    if(IsValid(NewPlayer))
    {
        auto TackControllerComponent = NewPlayer->FindComponentByClass<UTackControllerComponent>();
        if(TackControllerComponent == nullptr)
        {
            TackControllerComponent = NewObject<UTackControllerComponent>(NewPlayer, NAME_None);
            TackControllerComponent->ShouldStartTackOnBeginPlay(IsTackRunning());
            TackControllerComponent->RegisterComponent();
        }
    }
}

void UTackManager::OnGameModeInitialized(AGameModeBase* GameModeBase)
{
    auto GameState = GameModeBase->GameState;
    if(GameState != nullptr && GameState->IsActorInitialized())
    {
        if(UTackGameStateBaseComponent* TackGameStateBaseComponent = NewObject<UTackGameStateBaseComponent>(GameState, NAME_None))
        {
            TackGameStateBaseComponent->ClientKafkaConnectionString = GetDefault<UTackSettings>()->ClientConnectionString;
            TackGameStateBaseComponent->RegisterComponent();
        }
        else
        {
            UE_LOG(LogTack, Fatal, TEXT("Failed to spawn UTackGameStateBaseComponent"));
        }
    }
    else
    {
        GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, GameModeBase]() {
            OnGameModeInitialized(GameModeBase);
            }));
    }
}

void UTackManager::OnWorldBeginTearingDown(UWorld* World)
{
    if(IsTackRunning() && World == GetGameInstance()->GetWorld())
    {
        UE_LOG(LogTack, Log, TEXT("%s - world begin tear down %s stopping tack"), *GetTackLogPrefix(World), *World->URL.Map);
        StopTackLocally();
    }
}

void UTackManager::OnNotifyPreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel)
{

}

void UTackManager::OnTravelFailure(UWorld* InWorld, ETravelFailure::Type FailureType, const FString& ErrorString)
{

}

void UTackManager::OnPawnControllerChanged(APawn* Pawn, AController* Controller)
{
    if(
        IsValid(Pawn) &&
        Pawn->GetWorld() != nullptr &&
        Pawn->GetWorld()->GetGameState() != nullptr &&
        GetGameInstance()->GetWorld()->GetNetMode() < ENetMode::NM_Client
        )
    {
        if(Controller != nullptr && Controller->IsPlayerController() && Controller->PlayerState == nullptr)
        {
            UE_LOG(LogTack, Warning, TEXT("%s - Delaying Pawn Controller Publish until PlayerState/Controller have been tackified"), *GetTackLogPrefix(Pawn->GetWorld()));
            GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, Pawn, Controller]() {
                this->OnPawnControllerChanged(Pawn, Controller);
                }));
        }
        else
        {
            Publisher->PublishPawnControllerChanged(Pawn, Controller);

            UE_LOG(LogTemp, Log, TEXT("%s - Controller (%s) - Pawn (%s)"), *GetTackLogPrefix(Pawn->GetWorld()),
                *UTackStatics::GetTackIdFromActor(Controller).ToString(),
                *UTackStatics::GetTackIdFromActor(Pawn).ToString()
            );
        }
    }
}

FString UTackManager::GetKafkaConnectionString() const
{
    auto Settings = GetDefault<UTackSettings>();

    UWorld* World = GetGameInstance()->GetWorld();

    if(World->GetNetMode() < ENetMode::NM_Client || Settings->bForceClientToUseLocalConnectionString)
    {
        return Settings->LocalConnectionString;
    }
    else
    {
        auto GameStateBaseComponent = UTackStatics::GetTackGameStateBaseComponent(World);
        check(GameStateBaseComponent);
        FString ConnectionString = GameStateBaseComponent->ClientKafkaConnectionString;
        check(!ConnectionString.IsEmpty())
            return ConnectionString;
    }
}

const FGuid& UTackManager::GetCurrentSessionId() const
{
    return CurrentSessionId;
}

void UTackManager::StartTack()
{
    UWorld* World = GetGameInstance()->GetWorld();

    if(IsTackRunning())
    {
        FString LogMessage = FString::Printf(TEXT("%s - Tack is already running"), *GetTackLogPrefix(World));
        UE_LOG(LogTack, Warning, TEXT("%s"), *LogMessage);
        if(GEngine)
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, *LogMessage);
        return;
    }

    if(GetGameInstance()->GetWorld()->GetNetMode() < ENetMode::NM_Client)
    {
        //Start Tack with new Session Guid
        StartTackLocally(FGuid::NewGuid());
    }
    else
    {
        if(auto TackControllerComponent = World->GetFirstPlayerController()->FindComponentByClass<UTackControllerComponent>())
        {
            TackControllerComponent->ServerStopTack();
        }
        else
        {
            UE_LOG(LogTack, Error, TEXT("%s - Failed to start Tack. Could not find TackControllerComponent on LocalPlayerController."), *GetTackLogPrefix(World));
        }
    }
}

void UTackManager::StopTack()
{
    UWorld* World = GetGameInstance()->GetWorld();

    if(!IsTackRunning())
    {
        FString LogMessage = FString::Printf(TEXT("%s - Tack is already NOT running"), *GetTackLogPrefix(World));
        UE_LOG(LogTack, Warning, TEXT("%s"), *LogMessage);
        if(GEngine)
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, *LogMessage);
        return;
    }

    if(GetGameInstance()->GetWorld()->GetNetMode() < ENetMode::NM_Client)
    {
        check(IsTackRunning()) // Tack must not be running to call this
            check(CurrentSessionId.IsValid()) // session id should be valid at this point
            check(GetGameInstance()->GetWorld()->GetNetMode() < ENetMode::NM_Client); //Only call this on server

        StopTackLocally();
    }
    else
    {
        if(auto TackControllerComponent = World->GetFirstPlayerController()->FindComponentByClass<UTackControllerComponent>())
        {
            TackControllerComponent->ServerStopTack();
        }
        else
        {
            UE_LOG(LogTack, Error, TEXT("%s - Failed to stop Tack. Could not find TackControllerComponent on LocalPlayerController."), *GetTackLogPrefix(World));
        }
    }
}

void UTackManager::StartTackLocally(const FGuid& SessionId)
{
    Internal_StartTack(SessionId);
}

void UTackManager::StopTackLocally()
{
    Internal_StopTack();
}

void UTackManager::Internal_StartTack(const FGuid& SessionId)
{
    check(SessionId.IsValid());

    UWorld* World = GetGameInstance()->GetWorld();
    bool bIsServer = World->GetNetMode() < ENetMode::NM_Client;

    if(IsTackRunning())
    {
        FString LogMessage = FString::Printf(TEXT("%s - Tack is already running"), *GetTackLogPrefix(World));
        UE_LOG(LogTack, Warning, TEXT("%s"), *LogMessage);
        if(GEngine)
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, *LogMessage);
        return;
    }

    CurrentSessionId = SessionId;

    FString LogPrefix = GetTackLogPrefix(World);
    FString LogPrefixWithConnection = GetTackLogPrefix(World, true);

    UE_LOG(LogTack, Log, TEXT("%s - Tack Starting"), *LogPrefixWithConnection);

    bIsTackRunning = true;

    Publisher->EnablePublisher(GetCurrentSessionId(), GetLocalInstanceId(), GetKafkaConnectionString());

    if(bIsServer)
    {
        Publisher->PublishTackServerStart(World);

        //Send start command to all connected clients
        for(FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
        {
            APlayerController* PlayerController = Cast<APlayerController>(*Iterator);
            //The local server player controller does not need to make a request to start Tack
            if(!PlayerController->IsLocalPlayerController())
            {
                if(auto TackControllerComponent = PlayerController->FindComponentByClass<UTackControllerComponent>())
                {
                    TackControllerComponent->ClientStartLocalTack(GetCurrentSessionId());
                }
            }
        }
    }

    Publisher->PublishTackClientStart(World);

    SubsystemCollection.Initialize(this);

    static const EObjectFlags ExcludeFlags = RF_ClassDefaultObject | RF_ArchetypeObject;
    for(TObjectIterator<UObject> It(ExcludeFlags, true, EInternalObjectFlags::Garbage); It; ++It)
    {
        if(It->GetWorld() == World && It->Implements<UTackReceivesStateChangeInterface>())
        {
            ITackReceivesStateChangeInterface::Execute_OnTackStart(*It);
        }

        // if(ITackRecievesStateChangeInterface* TackInterface = Cast<ITackRecievesStateChangeInterface>(*It))
        //     TackInterface->OnTackStart();
    }

    OnTackStart.Broadcast();

    FString LogMessage = FString::Printf(TEXT("%s - Tack Started"), *LogPrefixWithConnection);
    UE_LOG(LogTack, Log, TEXT("%s"), *LogMessage);
    if(GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, *LogMessage);
}

void UTackManager::Internal_StopTack()
{
    UWorld* World = GetGameInstance()->GetWorld();
    bool bIsServer = World->GetNetMode() < ENetMode::NM_Client;
    FString LogPrefix = GetTackLogPrefix(World);

    if(!IsTackRunning())
    {
        FString LogMessage = FString::Printf(TEXT("%s - Tack is already NOT running"), *LogPrefix);
        UE_LOG(LogTack, Warning, TEXT("%s"), *LogMessage);
        if(GEngine)
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, *LogMessage);
        return;
    }

    UE_LOG(LogTack, Log, TEXT("%s - Tack Stopping"), *LogPrefix);

    //Current sessionId should always be valid at this point
    check(CurrentSessionId.IsValid())

        Publisher->PublishTackClientEnd(World);

    OnTackEnd.Broadcast();

    static const EObjectFlags ExcludeFlags = RF_ClassDefaultObject | RF_ArchetypeObject;
    for(TObjectIterator<UObject> It(ExcludeFlags, true, EInternalObjectFlags::Garbage); It; ++It)
    {
        if(It->GetWorld() == World && It->Implements<UTackReceivesStateChangeInterface>())
        {
            ITackReceivesStateChangeInterface::Execute_OnTackEnd(*It);
        }
    }

    SubsystemCollection.Deinitialize();

    if(bIsServer)
    {
        Publisher->PublishTackServerEnd(World);

        //send request to stop all clients Tack
        for(FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
        {
            APlayerController* PlayerController = Cast<APlayerController>(*Iterator);
            //The local server player controller does not need to make a request to stop Tack
            if(!PlayerController->IsLocalPlayerController())
            {
                if(auto TackControllerComponent = PlayerController->FindComponentByClass<UTackControllerComponent>())
                {
                    TackControllerComponent->ClientStopLocalTack();
                }
            }
        }
    }

    Publisher->DisablePublisher();

    bIsTackRunning = false;

    FString LogMessage = FString::Printf(TEXT("%s - Tack has stopped"), *LogPrefix);
    UE_LOG(LogTack, Log, TEXT("%s"), *LogMessage);
    if(GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, *LogMessage);

    //Invalidate the CurrentSessionId;
    CurrentSessionId.Invalidate();
}