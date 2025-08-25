// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MediaCapture.h"
#include "ImageWriteTypes.h"

#include "Fmc.generated.h"


//basically the same as UFileMediaCapture but has some changes to it
UCLASS()
class UFmc : public UMediaCapture
{
    GENERATED_BODY()

protected:
    virtual void OnFrameCaptured_RenderingThread(const FCaptureBaseData& InBaseData, TSharedPtr<FMediaCaptureUserData, ESPMode::ThreadSafe> InUserData, void* InBuffer, int32 Width, int32 Height, int32 BytesPerRow) override;
    virtual bool InitializeCapture() override;

    virtual TSharedPtr<FMediaCaptureUserData, ESPMode::ThreadSafe> GetCaptureFrameUserData_GameThread();

private:
    void CacheMediaOutputValues();

public:
    int JpegQuality;
    FFrameRate CaptureFrameRate;
    FFrameNumber LastCaputredFrame;
    FString LocalTackID;
    bool bSendKafka;
    bool bSaveToFile;

    class UTackSnapshotSubsystem* GetSnapshotSubsystem() const;

protected:
    FString BaseFilePathName;
    EImageFormat ImageFormat;
    TFunction<void(bool)> OnCompleteWrapper;
    bool bOverwriteFile;
    int32 CompressionQuality;
    bool bAsync;
};


class FCustomCapData : public FMediaCaptureUserData
{
public:
    FCustomCapData(float InServerWorldTime, FString InImgId) :
        ServerWorldTime(InServerWorldTime),
        ImgId(InImgId)
    {
    }

    float ServerWorldTime;
    FString ImgId;
};