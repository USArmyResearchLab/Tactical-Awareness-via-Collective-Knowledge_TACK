#pragma once
#include "TackEyeTrackerTypes.h"
#include "TackBaseComponent.h"
#include "TackEyeTrackerComponent.generated.h"

class UTackId;

UCLASS(Abstract, Within = PlayerController)
class TACK_API UTackEyeTrackerComponent : public UTackBaseComponent
{
    GENERATED_BODY()
public:

    UTackEyeTrackerComponent();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    virtual void OnTackStart_Implementation() override;
    virtual void OnTackEnd_Implementation() override;

    float MaximumTraceDistance = 100000;

    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    void PublishTrace(const FTackEyeTrackerTraceAggregateQueryParams& QueryParams);
    TOptional<FTackEyeTrackerTraceQueryParams> BuildParamsFromDisplayArea(const FVector2D& PosOnDisplayArea);
    TOptional<FHitResult> PerformDefaultTrace(const TOptional<FTackEyeTrackerTraceQueryParams>& TraceParams);

    FIntPoint GetDesktopResolution();
    float GetServerWorldTimeSeconds();

protected:

    APlayerController* PlayerController;
    TOptional<FIntPoint> CachedDesktopResolution;

private:
    int64 CurrentFrame;
    int64 FrameTraceCount;

    bool CurrentFrameDebuged = false;
    void Debug(const TOptional<FHitResult>& Main, const TOptional<FHitResult>& Left, const TOptional<FHitResult>& Right);
    void DebugDrawHitResult(const FHitResult& HitResult);
};