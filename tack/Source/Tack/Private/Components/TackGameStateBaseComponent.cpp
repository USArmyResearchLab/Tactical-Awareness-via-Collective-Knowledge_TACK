#include "Components/TackGameStateBaseComponent.h"
#include "GameFramework/GameState.h"
#include "Tack.h"
#include "TackStatics.h"
#include "TackSettings.h"
#include "GameFramework/GameModeBase.h"
#include <chrono>
#include "JsonDomBuilder.h"
#include "TackManager.h"
#include "TackPublisher.h"
#include "Net/UnrealNetwork.h"

DECLARE_CYCLE_STAT(TEXT("Client Time Publish"), STAT_Client_TimePublish, STATGROUP_Tack);


UTackGameStateBaseComponent::UTackGameStateBaseComponent()
{
    PrimaryComponentTick.bStartWithTickEnabled = true;
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
}

void UTackGameStateBaseComponent::BeginPlay()
{
    Super::BeginPlay();
    TackManager = UTackManager::GetInstance(this);
}

void UTackGameStateBaseComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    FDoRepLifetimeParams SharedParams;
    DOREPLIFETIME_WITH_PARAMS_FAST(UTackGameStateBaseComponent, ClientKafkaConnectionString, SharedParams);
}

void UTackGameStateBaseComponent::OnTackStart_Implementation()
{
    if(GetOwner()->HasAuthority())
    {
        if(AGameState* GameState = Cast<AGameState>(GetOwner()))
        {
            OnGameModeMatchStateSetEvent(GameState->GetMatchState());
            FGameModeEvents::OnGameModeMatchStateSetEvent().AddUObject(this, &UTackGameStateBaseComponent::OnGameModeMatchStateSetEvent);
        }
    }
}

void UTackGameStateBaseComponent::OnTackEnd_Implementation()
{
    if(GetOwner()->HasAuthority() && Cast<AGameState>(GetOwner()) != nullptr)
    {
        FGameModeEvents::OnGameModeMatchStateSetEvent().RemoveAll(this);
    }
}

void UTackGameStateBaseComponent::OnGameModeMatchStateSetEvent(FName MatchState)
{
    if(AGameState* GameState = Cast<AGameState>(GetOwner()))
    {
        FTackJsonDomBuilder::FTackObject Object;
        Object.Set("MatchState", MatchState);
        Object.Set("bHasMatchStarted", GameState->HasMatchStarted());
        Object.Set("ServerWorldTime", GameState->GetServerWorldTimeSeconds());
        TackManager->GetPublisher()->Publish_Json(TEXT("unreal.match_state"), Object.AsJsonObject());
    }
}

void UTackGameStateBaseComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if(TackManager->IsTackRunning())
    {
        SCOPE_CYCLE_COUNTER(STAT_Client_TimePublish);

        extern ENGINE_API float GAverageFPS;
        extern ENGINE_API float GAverageMS;

        FTackJsonDomBuilder::FTackObject Object;
        Object.Set("SteadyClock", std::chrono::nanoseconds(std::chrono::steady_clock::now().time_since_epoch()).count());
        Object.Set("ServerWorldTime", GetWorld()->GetGameState()->GetServerWorldTimeSeconds());
        Object.Set("FrameCount", (int64)GFrameCounter);
        Object.Set("DeltaTime", DeltaTime);
        Object.Set("AverageFPS", GAverageFPS);
        Object.Set("AverageMS", GAverageMS);

        static FString TOPIC_NAME = FString(TEXT("unreal.client.time"));
        TackManager->GetPublisher()->Publish_Json(TOPIC_NAME, Object.AsJsonObject());
    }
}