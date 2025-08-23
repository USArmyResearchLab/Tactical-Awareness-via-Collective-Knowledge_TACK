// Copyright Epic Games, Inc. All Rights Reserved.

#include "Fmo.h"
#include "Fmc.h"
#include "Misc/Paths.h"
#include "UnrealEngine.h"
#include "TackSnapshotSubsystem.h"


UFmo::UFmo()
    : Super()
{
}

UTackSnapshotSubsystem* UFmo::GetSnapshotSubsystem() const
{
    return CastChecked<UTackSnapshotSubsystem>(GetOuter());
}

bool UFmo::Validate(FString& OutFailureReason) const
{
    if(!Super::Validate(OutFailureReason))
    {
        return false;
    }

    if(GetRequestedPixelFormat() == PF_A2B10G10R10)
    {
        OutFailureReason = FString::Printf(TEXT("Can't validate MediaOutput '%s'. File media output doesn't support 10bits format."), *GetName());
        return false;
    }

    return true;
}

EPixelFormat UFmo::GetRequestedPixelFormat() const
{
    static const auto CVarDefaultBackBufferPixelFormat = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.DefaultBackBufferPixelFormat"));
    EPixelFormat SceneTargetFormat = EDefaultBackBufferPixelFormat::Convert2PixelFormat(EDefaultBackBufferPixelFormat::FromInt(CVarDefaultBackBufferPixelFormat->GetValueOnAnyThread()));
    return SceneTargetFormat == EPixelFormat::PF_A2B10G10R10 ? PF_B8G8R8A8 : SceneTargetFormat;
}

EMediaCaptureConversionOperation UFmo::GetConversionOperation(EMediaCaptureSourceType InSourceType) const
{
    return EMediaCaptureConversionOperation::SET_ALPHA_ONE;
}

UMediaCapture* UFmo::CreateMediaCaptureImpl()
{
    UFmc* Result = NewObject<UFmc>(this);
    if(Result)
    {
        Result->SetMediaOutput(this);
        Result->LocalTackID = LocalTackID;
        Result->bSaveToFile = bSaveToFile;
        Result->bSendKafka = bSendKafka;
    }
    return Result;
}