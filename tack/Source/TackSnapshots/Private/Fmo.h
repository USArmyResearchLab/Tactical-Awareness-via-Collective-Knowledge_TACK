#pragma once

#include "Engine/RendererSettings.h"
#include "ImageWriteBlueprintLibrary.h"
#include "FileMediaOutput.h"

#include "Fmo.generated.h"

UCLASS(BlueprintType)
class UFmo : public UFileMediaOutput
{
    GENERATED_BODY()

public:
    UFmo();

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int CompressionQuality;

    //1 all frames 2 every other, an so on
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FFrameRate CaptureFrameRate;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString SnapshotDirectory;

    UPROPERTY()
    FString LocalTackID;

    UPROPERTY()
    bool bSendKafka;

    UPROPERTY()
    bool bSaveToFile;

    UTackSnapshotSubsystem* GetSnapshotSubsystem() const;

    //~ UMediaOutput interface
    virtual bool Validate(FString& FailureReason) const override;
    virtual EPixelFormat GetRequestedPixelFormat() const override;
    virtual EMediaCaptureConversionOperation GetConversionOperation(EMediaCaptureSourceType InSourceType) const override;

protected:
    virtual UMediaCapture* CreateMediaCaptureImpl() override;
    //~ End UMediaOutput interface
};