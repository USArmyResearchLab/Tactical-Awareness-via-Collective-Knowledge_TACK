#include "Components/TackGameModeBaseComponent.h"
#include "Components/TackGameStateBaseComponent.h"
#include "Components/TackPlayerStateComponent.h"
#include "TackStatics.h"


#include "Components/ChildActorComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameState.h"
#include "TackManager.h"
#include "TackPublisher.h"

DECLARE_CYCLE_STAT(TEXT("Actor Tackification"), STAT_ActorTackification, STATGROUP_Tack);


UTackGameModeBaseComponent::UTackGameModeBaseComponent()
{
    PrimaryComponentTick.bStartWithTickEnabled = true;
    PrimaryComponentTick.bCanEverTick = true;
}

void UTackGameModeBaseComponent::BeginPlay()
{
    Super::BeginPlay();

    GameModeBase = Cast<AGameModeBase>(GetOwner());

    GameModePostLoginHandle = FGameModeEvents::OnGameModePostLoginEvent().AddUObject(this, &UTackGameModeBaseComponent::OnGameModePostLoginEvent);
    GameModeLogoutHandle = FGameModeEvents::OnGameModeLogoutEvent().AddUObject(this, &UTackGameModeBaseComponent::OnGameModeLogoutEvent);
    GameModeMatchStateSetEvent = FGameModeEvents::OnGameModeMatchStateSetEvent().AddUObject(this, &UTackGameModeBaseComponent::OnGameModeMatchStateSetEvent);


    if(UTackGameStateBaseComponent* GameStateComponent = NewObject<UTackGameStateBaseComponent>(GetGameState(), TackGameStateClassToSpawn))
        GameStateComponent->RegisterComponent();
    else
        UE_LOG(LogTack, Fatal, TEXT("Failed to spawn TackGameStateComponent"));
}

void UTackGameModeBaseComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    FGameModeEvents::OnGameModePostLoginEvent().Remove(GameModePostLoginHandle);
    FGameModeEvents::OnGameModeLogoutEvent().Remove(GameModeLogoutHandle);
    FGameModeEvents::OnGameModeMatchStateSetEvent().Remove(GameModeMatchStateSetEvent);
}

void UTackGameModeBaseComponent::OnTackStart_Implementation()
{
    //Publish Gamestate
    if(AGameState* GameState = GetGameState<AGameState>())
        OnGameModeMatchStateSetEvent(GameState->GetMatchState());
    else
        UE_LOG(LogTack, Warning, TEXT("MatchState publishing only supports GameStates deriving from AGameState"));
}

void UTackGameModeBaseComponent::OnTackEnd_Implementation()
{

}

void UTackGameModeBaseComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{

}

void UTackGameModeBaseComponent::OnActorSpawnedEvent(AActor* Actor)
{

}

void UTackGameModeBaseComponent::OnGameModeMatchStateSetEvent(FName MatchState)
{
    auto TackManager = UTackManager::GetInstance(this);

    FTackJsonDomBuilder::FTackObject Object;
    Object.Set("MatchState", MatchState);
    Object.Set("bHasMatchStarted", GetGameState()->HasMatchStarted());
    Object.Set("ServerWorldTime", GetWorld()->GetGameState()->GetServerWorldTimeSeconds());
    TackManager->GetPublisher()->Publish_Json(TEXT("unreal.match_state"), Object.AsJsonObject());
}

void UTackGameModeBaseComponent::OnGameModePostLoginEvent(AGameModeBase* GameMode, APlayerController* NewPlayer)
{

}

void UTackGameModeBaseComponent::OnGameModeLogoutEvent(AGameModeBase* GameMode, AController* Player)
{
    if(Player == nullptr || GameMode == nullptr)
        return;

    UE_LOG(LogTack, Log, TEXT("Client %s Logout"), *UTackStatics::GetTackIdFromActor(Player).ToString());
}