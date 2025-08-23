#include "Devices/EyeTracker/TackEyeTrackerComponent.h"
#include "Components/TackGameModeBaseComponent.h"
#include "Serialization/TackJsonDomBuilder.h"
#include "TackStatics.h"

#include "TackSettings.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Slate/SceneViewport.h"
#include "DrawDebugHelpers.h"
#include "TackManager.h"
#include "TackPublisher.h"

DECLARE_CYCLE_STAT(TEXT("Eyetracker Trace Publish"), STAT_Eyetracker_TracePublish, STATGROUP_Tack);


static TAutoConsoleVariable<int32> CVarTackEyeTrackerDebug(
    TEXT("Tack.EyeTracker.Debug"),
    0,
    TEXT("Enable visual Debugging | 1 - Enabled | 0 - Disabled")
);

static TAutoConsoleVariable<int32> CVarTackEyeTrackerDebugMain(
    TEXT("Tack.EyeTracker.DebugMain"),
    1,
    TEXT("Debug main Impact Point | 1 - Enabled | 0 - Disabled ")
);

static TAutoConsoleVariable<int32> CVarTackEyeTrackerDebugLeftEye(
    TEXT("Tack.EyeTracker.DebugLeftEye"),
    1,
    TEXT("Debug Left Eye Impact Point | 1 - Enabled | 0 - Disabled ")
);

static TAutoConsoleVariable<int32> CVarTackEyeTrackerDebugRightEye(
    TEXT("Tack.EyeTracker.DebugRightEye"),
    1,
    TEXT("Debug Right Eye Impact Point | 0 - Enabled | 1 - Disabled ")
);


UTackEyeTrackerComponent::UTackEyeTrackerComponent() : UTackBaseComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(false);
    bAutoActivate = true;
    bAutoRegister = false;
}

void UTackEyeTrackerComponent::BeginPlay()
{
    Super::BeginPlay();
    PlayerController = Cast<APlayerController>(GetOwner());
    MaximumTraceDistance = GetDefault<UTackSettings>()->MaximumTraceDistance;
    FrameTraceCount = 0;
}

void UTackEyeTrackerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void UTackEyeTrackerComponent::OnTackStart_Implementation()
{
    Super::OnTackStart_Implementation();
}

void UTackEyeTrackerComponent::OnTackEnd_Implementation()
{
    Super::OnTackEnd_Implementation();
}

void UTackEyeTrackerComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    CurrentFrameDebuged = false;
}

FIntPoint UTackEyeTrackerComponent::GetDesktopResolution()
{
    if(!CachedDesktopResolution.IsSet())
        CachedDesktopResolution = GEngine->GetGameUserSettings()->GetDesktopResolution();

    return CachedDesktopResolution.GetValue();
}

float UTackEyeTrackerComponent::GetServerWorldTimeSeconds()
{
    return GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
}

TOptional<FTackEyeTrackerTraceQueryParams> UTackEyeTrackerComponent::BuildParamsFromDisplayArea(const FVector2D& PosOnDisplayArea)
{
    ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();

    if(PlayerController->GetPawn() == nullptr)
    {
        return {};
    }

    FIntPoint DesktopResolution = GetDesktopResolution();
    FIntPoint ScreenGazePointNormalized(PosOnDisplayArea.X * DesktopResolution.X, PosOnDisplayArea.Y * DesktopResolution.Y);

    FVector2D ViewportSize;
    LocalPlayer->ViewportClient->GetViewportSize(ViewportSize);

    FVector2D ViewportPixelCoordUNorm = LocalPlayer->ViewportClient->GetGameViewport()->VirtualDesktopPixelToViewport(ScreenGazePointNormalized);
    FVector2D ScreenGazePoint(ViewportPixelCoordUNorm.X * ViewportSize.X, ViewportPixelCoordUNorm.Y * ViewportSize.Y);

    FVector WorldGazeOrigin;
    FVector WorldGazeDirection;

    FTackEyeTrackerTraceQueryParams QueryParams;

    //is deproject succesful
    if(!UGameplayStatics::DeprojectScreenToWorld(PlayerController, ScreenGazePoint, QueryParams.GazeOrigin, QueryParams.GazeDirection))
    {
        return {};
    }
    else
    {
        return QueryParams;
    }
}

TOptional<FHitResult> UTackEyeTrackerComponent::PerformDefaultTrace(const TOptional<FTackEyeTrackerTraceQueryParams>& TraceParams)
{
    if(!TraceParams.IsSet())
        return {};

    const FTackEyeTrackerTraceQueryParams& TraceParamsValue = TraceParams.GetValue();

    FCollisionQueryParams CollisionQueryParams;
    CollisionQueryParams.AddIgnoredActor(PlayerController);
    CollisionQueryParams.AddIgnoredActor(PlayerController->GetPawn());

    FHitResult HitResult;

    GetWorld()->LineTraceSingleByChannel(
        HitResult,
        TraceParamsValue.GazeOrigin,
        TraceParamsValue.GazeOrigin + (TraceParamsValue.GazeDirection * MaximumTraceDistance),
        ECC_Visibility,
        CollisionQueryParams
    );

    return HitResult;
}

void  UTackEyeTrackerComponent::PublishTrace(const FTackEyeTrackerTraceAggregateQueryParams& QueryParams)
{
    SCOPE_CYCLE_COUNTER(STAT_Eyetracker_TracePublish);

    //PlayerController must be valid and have a Pawn
    //Main must be set or no Publish is performed
    if(PlayerController->GetPawn() == nullptr || !QueryParams.Main.IsSet())
    {
        return;
    }

    FTackJsonDomBuilder::FTackObject Json;
    //Json.Set("Client_id", TackManager->GetLocalInstanceId()) Dont need this anymore as its in kafka headers
    Json.Set("ServerWorldTime", GetServerWorldTimeSeconds())
        .OptionalSet("SourceUtc", QueryParams.SourceUtc)
        .Set("GameFrame", (int64)GFrameCounter)
        .Set("Raw", QueryParams.Raw);

    //Perform and set Main Trace
    TOptional<FHitResult> Main = PerformDefaultTrace(QueryParams.Main);
    Json.OptionalSet("Main", Main);

    TOptional<FHitResult> Right = PerformDefaultTrace(QueryParams.Right);
    Json.OptionalSet("Right", Right);

    TOptional<FHitResult> Left = PerformDefaultTrace(QueryParams.Left);
    Json.OptionalSet("Left", Left);

    GetTackManager()->GetPublisher()->Publish_Json(TEXT("unreal.client.eyetracker.trace"), Json.AsJsonObject());

    Debug(Main, Right, Left);
}

void UTackEyeTrackerComponent::Debug(const TOptional<FHitResult>& Main, const TOptional<FHitResult>& Right, const TOptional<FHitResult>& Left)
{
    if(!CurrentFrameDebuged && CVarTackEyeTrackerDebug.GetValueOnGameThread())
    {

        if(Main.IsSet() && CVarTackEyeTrackerDebugMain.GetValueOnGameThread())
        {
            DebugDrawHitResult(Main.GetValue());
            CurrentFrameDebuged = true;
        }

        if(Right.IsSet() && CVarTackEyeTrackerDebugRightEye.GetValueOnGameThread())
        {
            DebugDrawHitResult(Right.GetValue());
            CurrentFrameDebuged = true;
        }

        if(Left.IsSet() && CVarTackEyeTrackerDebugLeftEye.GetValueOnGameThread())
        {
            DebugDrawHitResult(Left.GetValue());
            CurrentFrameDebuged = true;
        }
    }
}

void UTackEyeTrackerComponent::DebugDrawHitResult(const FHitResult& HitResult)
{
    if(UTackStatics::IsActorTackified(HitResult.GetActor()))
    {
        DrawDebugPoint(GetWorld(), HitResult.ImpactPoint, 10.0f, FColor::Cyan, false, -1.0f);
    }
    else
    {
        DrawDebugPoint(GetWorld(), HitResult.ImpactPoint, 10.0f, FColor::Red, false, -1.0f);
    }
}