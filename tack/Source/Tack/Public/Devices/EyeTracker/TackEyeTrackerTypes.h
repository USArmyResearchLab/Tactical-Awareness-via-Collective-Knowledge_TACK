#pragma once
#include "Serialization/TackJsonDomBuilder.h"
#include "TackEyeTrackerTypes.generated.h"


USTRUCT()
struct TACK_API FTackEyeTrackerTraceQueryParams
{
    GENERATED_BODY()
public:

    FTackEyeTrackerTraceQueryParams() = default;

    FTackEyeTrackerTraceQueryParams(const FVector& GazeOrigin, const FVector& GazeDirection) :
    GazeOrigin(GazeOrigin),
    GazeDirection(GazeDirection)
    {}

    FVector GazeOrigin;
    FVector GazeDirection;

};

USTRUCT()
struct TACK_API FTackEyeTrackerTraceAggregateQueryParams
{
    GENERATED_BODY()
public:
    FTackEyeTrackerTraceAggregateQueryParams() = default;
    FTackEyeTrackerTraceAggregateQueryParams(TOptional<int64> SourceUtc) :
    SourceUtc(SourceUtc)
    {}

    TOptional<int64> SourceUtc;

    TOptional<FTackEyeTrackerTraceQueryParams> Main;
    TOptional<FTackEyeTrackerTraceQueryParams> Left;
    TOptional<FTackEyeTrackerTraceQueryParams> Right;

    FTackJsonObject Raw;
};