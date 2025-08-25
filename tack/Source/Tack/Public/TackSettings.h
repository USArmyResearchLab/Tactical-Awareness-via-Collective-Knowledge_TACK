#pragma once
#include "Engine/DeveloperSettings.h"
#include "TackSettings.generated.h"


UENUM()
enum class EImgTypeEnum
{
    JPEG,
    PNG
};

UCLASS(config = Tack)
class TACK_API UTackSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public: // --- General Settings ---
    UPROPERTY(EditAnywhere, noclear, config, Category = "General")
    FString ExperimentName;

    UPROPERTY(EditAnywhere, config, Category = "General")
    TMap<FString, FString> AdditionalClientInfo;

    //--- Actor Publisher Settings ---

    UPROPERTY(EditAnywhere, noclear, config, Category = "Actor Publisher Settings")
    bool bPublishLandscapeSplineMeshComponents = true;

    UPROPERTY(EditAnywhere, noclear, config, Category = "Actor Publisher Settings")
    bool bEnableActorDamagePublisher = true;

    UPROPERTY(EditAnywhere, noclear, config, Category = "Actor Publisher Settings")
    bool bEnableActorCollisionPublisher = true;

    UPROPERTY(EditAnywhere, noclear, config, Category = "Actor Publisher Settings")
    bool bEnableActorTransformPublisher = true;

    UPROPERTY(EditAnywhere, noclear, config, Category = "Actor Publisher Settings", meta = (EditCondition = bEnableActorTransformPublisher))
    bool bOnlyPublishRootComponentTransform = true;

    UPROPERTY(EditAnywhere, noclear, config, Category = "AI Publisher Settings")
    bool bEnableAIPerceptionPublish = true;

    // --- Camera Transform Publisher Settings ---

    UPROPERTY(EditAnywhere, noclear, config, Category = "Camera Transfrom Publisher")
    bool bEnableCameraTransformPublisher = true;

    //meta=(DisplayName="Tick Interval (secs)")
    /** The frequency in seconds at which this tick function will be executed.  If less than or equal to 0 then it will tick every frame */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "Camera Transfrom Publisher", meta = (EditCondition = bEnableCameraTransformPublisher))
    float CameraTransformPublisherTickInterval = 0.0f;

    //Default to using KINDA_SMALL_NUMBER its what .Equals Defaults to
    UPROPERTY(EditAnywhere, noclear, config, Category = "Camera Transfrom Publisher", meta = (EditCondition = bEnableCameraTransformPublisher))
    float CameraTransformPublisherTolerance = KINDA_SMALL_NUMBER;

    // --- Input Publisher Settings ---

    UPROPERTY(EditAnywhere, noclear, config, Category = "Input Publisher Settings")
    bool bEnableMappedInputPublisher = true;

    UPROPERTY(EditAnywhere, noclear, config, Category = "Input Publisher Settings")
    bool bEnableRawInputPublisher = true;

    bool PublishesAnyInput() const
    {
        return bEnableMappedInputPublisher || bEnableRawInputPublisher;
    }

    // --- kafka Settings --- Get Current Connection String from UTackStatics

    UPROPERTY(EditAnywhere, noclear, config, Category = "Kafka Settings")
    bool bForceClientToUseLocalConnectionString;

    UPROPERTY(EditAnywhere, noclear, config, Category = "Kafka Settings")
    FString LocalConnectionString;

    UPROPERTY(EditAnywhere, noclear, config, Category = "Kafka Settings")
    FString ClientConnectionString;

    // --- EyeTracker Settings ---

    UPROPERTY(EditAnywhere, noclear, config, Category = "EyeTracker Settings")
    bool bEnableEyetrackerPublisher = true;

    UPROPERTY(EditAnywhere, noclear, config, Category = "EyeTracker Settings")
    float MaximumTraceDistance = 100000.f;

    // --- Snapshot Settings ---

    UPROPERTY(EditAnywhere, noclear, config, Category = "Snapshot Settings")
    bool bEnableSnapshotPublish;

    UPROPERTY(EditAnywhere, noclear, config, Category = "Snapshot Settings", meta = (EditCondition = bEnableSnapshotPublish))
    int CompressionQuality;

    UPROPERTY(EditAnywhere, noclear, config, Category = "Snapshot Settings", meta = (EditCondition = bEnableSnapshotPublish, UIMin = "0", MinValue = "0", UIMax = "24"))
    int32 CaptureFrameRate = 10;

    UPROPERTY(EditAnywhere, noclear, config, Category = "Snapshot Settings", meta = (EditCondition = bEnableSnapshotPublish))
    EImgTypeEnum imageType = EImgTypeEnum::JPEG;

    UPROPERTY(EditAnywhere, noclear, config, Category = "Snapshot Settings")
    bool bCropDesiredSize;

    UPROPERTY(EditAnywhere, noclear, config, Category = "Snapshot Settings", meta = (EditCondition = bCropDesiredSize))
    FIntPoint DesiredSize;

    int32 GetSnapshotFrameRate() const
    {
        return FMath::Clamp(CaptureFrameRate, 0, 60);
    }

    // TackHttpServer
    UPROPERTY(EditAnywhere, noclear, config, Category = "Http Server")
    bool bEnableHttpServer = false;

    UPROPERTY(EditAnywhere, noclear, config, Category = "Http Server", meta = (EditCondition = bEnableHttpServer))
    uint32 HttpServerPort = 49152;

    // --- UDeveloperSettings Interface ---

    virtual FName GetContainerName() const override
    {
        return FName(TEXT("Project"));
    }

    virtual FName GetCategoryName() const override
    {
        return FName(TEXT("Tack"));
    }

    virtual FName GetSectionName() const override
    {
        return FName(TEXT("Tack"));
    }
};