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

void UFmc::OnFrameCaptured_RenderingThread(const FCaptureBaseData& InBaseData, TSharedPtr<FMediaCaptureUserData, ESPMode::ThreadSafe> InUserData, void* InBuffer, int32 Width, int32 Height, int32 BytesPerRow)
{
    FFrameNumber CurrentFrame = FFrameRate::TransformTime(
        FFrameTime(InBaseData.SourceFrameTimecode.ToFrameNumber(InBaseData.SourceFrameTimecodeFramerate)),
        InBaseData.SourceFrameTimecodeFramerate,
        CaptureFrameRate
    ).FloorToFrame();

    if(CurrentFrame != LastCaputredFrame)
    {
        IImageWriteQueueModule* ImageWriteQueueModule = FModuleManager::Get().GetModulePtr<IImageWriteQueueModule>("ImageWriteQueue");
        if(ImageWriteQueueModule == nullptr)
        {
            SetState(EMediaCaptureState::Error);
            return;
        }

        TUniquePtr<FKImageWriteTask> ImageTask = MakeUnique<FKImageWriteTask>();
        auto CustomCaptureData = StaticCastSharedPtr<FCustomCapData>(InUserData);

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
            // We only support tightly packed rows without padding
            if((BytesPerRow != 0) && (BytesPerRow != (Width * 4)))
            {
                UE_LOG(LogTemp, Error, TEXT("File media capture only supports tightly packed rows. Expected stride: %d. Received stride: %d. It might also mean that output resolution is too small."), Width * 4, BytesPerRow);
                SetState(EMediaCaptureState::Error);
                return;
            }
            TUniquePtr<TImagePixelData<FColor>> PixelData = MakeUnique<TImagePixelData<FColor>>(FIntPoint(Width, Height), TArray<FColor, FDefaultAllocator64>(reinterpret_cast<FColor*>(InBuffer), Width * Height));
            ImageTask->PixelData = MoveTemp(PixelData);
        }
        else if(PixelFormat == PF_FloatRGBA)
        {
            // We only support tightly packed rows without padding
            if((BytesPerRow != 0) && (BytesPerRow != (Width * 8)))
            {
                UE_LOG(LogTemp, Error, TEXT("File media capture only supports tightly packed rows. Expected stride: %d. Received stride: %d. It might also mean that output resolution is too small."), Width * 8, BytesPerRow);
                SetState(EMediaCaptureState::Error);
                return;
            }
            TUniquePtr<TImagePixelData<FFloat16Color>> PixelData = MakeUnique<TImagePixelData<FFloat16Color>>(FIntPoint(Width, Height), TArray<FFloat16Color, FDefaultAllocator64>(reinterpret_cast<FFloat16Color*>(InBuffer), Width * Height));
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

        LastCaputredFrame = CurrentFrame;
    }
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

bool UFmc::InitializeCapture()
{
    FModuleManager::Get().LoadModuleChecked<IImageWriteQueueModule>("ImageWriteQueue");
    CacheMediaOutputValues();

    SetState(EMediaCaptureState::Capturing);

    return true;
}

void UFmc::CacheMediaOutputValues()
{
    UFmo* FileMediaOutput = CastChecked<UFmo>(MediaOutput);
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