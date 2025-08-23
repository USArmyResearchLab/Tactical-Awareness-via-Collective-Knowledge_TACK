// Copyright Epic Games, Inc. All Rights Reserved.

#include "Fmc.h"

#include "Fmo.h"
#include "ImagePixelData.h"
#include "Modules/ModuleManager.h"
#include "IImageWrapperModule.h"
#include "ImageWriteQueue.h"
#include "KImageWriteTask.h"
#include "GameFramework/GameStateBase.h"


UTackSnapshotSubsystem* UFmc::GetSnapshotSubsystem() const
{
    return CastChecked<UFmo>(GetOuter())->GetSnapshotSubsystem();
}

void UFmc::OnFrameCaptured_RenderingThread(const FCaptureBaseData& InBaseData, TSharedPtr<FMediaCaptureUserData, ESPMode::ThreadSafe> InUserData, void* InBuffer, int32 Width, int32 Height)
{
    //this is limited to 24fps by the InBaseData, higher frame rates are possible but this calculation needs to be replaced
    FFrameNumber CurrentFrame = FFrameRate::TransformTime(
        FFrameTime(InBaseData.SourceFrameTimecode.ToFrameNumber(InBaseData.SourceFrameTimecodeFramerate)),
        InBaseData.SourceFrameTimecodeFramerate,
        CaptureFrameRate
    ).FloorToFrame();

    if(CurrentFrame != LastCapturedFrame)
    {
        IImageWriteQueueModule* ImageWriteQueueModule = FModuleManager::Get().GetModulePtr<IImageWriteQueueModule>("ImageWriteQueue");
        if(ImageWriteQueueModule == nullptr)
        {
            SetState(EMediaCaptureState::Error);
            return;
        }

        TUniquePtr<FKImageWriteTask> ImageTask = MakeUnique<FKImageWriteTask>();
        auto CustomCaptureData = StaticCastSharedPtr<FCustomCapData>(InUserData);

        ImageTask->SnapshotSubsystem = GetSnapshotSubsystem();
        ImageTask->Format = ImageFormat;
        ImageTask->Filename = FString::Printf(TEXT("%s%s"), *BaseFilePathName, *CustomCaptureData->ImgId);
        ImageTask->ImgId = CustomCaptureData->ImgId;
        ImageTask->ServerWorldTime = CustomCaptureData->ServerWorldTime;
        ImageTask->ClientID = LocalTackID;
        ImageTask->bSaveToFile = bSaveToFile;
        ImageTask->bSendKafka = bSendKafka;
        ImageTask->bOverwriteFile = bOverwriteFile;
        ImageTask->CompressionQuality = CompressionQuality;
        ImageTask->OnCompleted = OnCompleteWrapper;

        EPixelFormat PixelFormat = GetDesiredPixelFormat();
        if(PixelFormat == PF_B8G8R8A8)
        {
            TUniquePtr<TImagePixelData<FColor>> PixelData = MakeUnique<TImagePixelData<FColor>>(FIntPoint(Width, Height), TArray<FColor, FDefaultAllocator64>(reinterpret_cast<FColor*>(InBuffer), Width * Height));
            ImageTask->PixelData = MoveTemp(PixelData);
        }
        else if(PixelFormat == PF_FloatRGBA)
        {
            TUniquePtr<TImagePixelData<FColor>> PixelData = MakeUnique<TImagePixelData<FColor>>(FIntPoint(Width, Height), TArray<FColor, FDefaultAllocator64>(reinterpret_cast<FColor*>(InBuffer), Width * Height));
            ImageTask->PixelData = MoveTemp(PixelData);
        }
        else
        {
            check(false);
        }

        TFuture<bool> DispatchedTask = ImageWriteQueueModule->GetWriteQueue().Enqueue(MoveTemp(ImageTask));

        if(!bAsync)
        {
            // If not async, wait for the dispatched task to complete.
            if(DispatchedTask.IsValid())
            {
                DispatchedTask.Wait();
            }
        }
        LastCapturedFrame = CurrentFrame;
    }
}

bool UFmc::CaptureSceneViewportImpl(TSharedPtr<FSceneViewport>& InSceneViewport)
{
    UFmo* FileMediaOutput = CastChecked<UFmo>(MediaOutput);
    CaptureFrameRate = FileMediaOutput->CaptureFrameRate;

    FModuleManager::Get().LoadModuleChecked<IImageWriteQueueModule>("ImageWriteQueue");
    CacheMediaOutputValues();

    SetState(EMediaCaptureState::Capturing);
    return true;
}

bool UFmc::CaptureRenderTargetImpl(UTextureRenderTarget2D* InRenderTarget)
{
    UFmo* FileMediaOutput = CastChecked<UFmo>(MediaOutput);
    CaptureFrameRate = FileMediaOutput->CaptureFrameRate;

    FModuleManager::Get().LoadModuleChecked<IImageWriteQueueModule>("ImageWriteQueue");
    CacheMediaOutputValues();

    SetState(EMediaCaptureState::Capturing);
    return true;
}

TSharedPtr<FMediaCaptureUserData, ESPMode::ThreadSafe> UFmc::GetCaptureFrameUserData_GameThread()
{
    return MakeShareable(
        new FCustomCapData(
            GetWorld()->GetGameState()->GetServerWorldTimeSeconds(),
            FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens)
        )
    );
}

void UFmc::CacheMediaOutputValues()
{
    UFileMediaOutput* FileMediaOutput = CastChecked<UFileMediaOutput>(MediaOutput);
    BaseFilePathName = FPaths::Combine(FileMediaOutput->FilePath.Path, FileMediaOutput->BaseFileName);
    ImageFormat = ImageFormatFromDesired(FileMediaOutput->WriteOptions.Format);
    CompressionQuality = FileMediaOutput->WriteOptions.CompressionQuality;
    bOverwriteFile = FileMediaOutput->WriteOptions.bOverwriteFile;
    bAsync = FileMediaOutput->WriteOptions.bAsync;

    OnCompleteWrapper = [NativeCB = FileMediaOutput->WriteOptions.NativeOnComplete, DynamicCB = FileMediaOutput->WriteOptions.OnComplete](bool bSuccess)
        {
            if(NativeCB)
            {
                NativeCB(bSuccess);
            }
            DynamicCB.ExecuteIfBound(bSuccess);
        };
}