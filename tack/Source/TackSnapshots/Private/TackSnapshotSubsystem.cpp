#include "TackSnapshotSubsystem.h"
#include "Fmo.h"
#include "Fmc.h"
#include "TackManager.h"
#include "TackSettings.h"
#include "Misc/Paths.h"

bool UTackSnapshotSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    auto TackGameSubsystem = CastChecked<UTackManager>(Outer);
    bool bIsLocalPlayer = (TackGameSubsystem->GetGameInstance()->GetFirstGamePlayer() != nullptr);

    const UTackSettings* Settings = GetDefault<UTackSettings>();
    return Settings->bEnableSnapshotPublish && bIsLocalPlayer;
}

void UTackSnapshotSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UTackSnapshotSubsystem::Deinitialize()
{
    Super::Deinitialize();
    StopMediaCapture();
}

void UTackSnapshotSubsystem::OnTackStart_Implementation()
{
    Super::OnTackStart_Implementation();
    const UTackSettings* Settings = GetDefault<UTackSettings>();

    int32 CaptureFrameRate = Settings->GetSnapshotFrameRate();
    if(CaptureFrameRate <= 0)
    {
        UE_LOG(LogTackSnapshots, Error, TEXT("CaptureFrameRate is <= 0 not starting frame capture."));
        return;
    }

    UFmo* Output = NewObject<UFmo>(this);
    Output->LocalTackID = GetTackManager()->GetLocalInstanceId().ToString();
    Output->CaptureFrameRate = FFrameRate(CaptureFrameRate, 1);
    Output->WriteOptions.CompressionQuality = Settings->CompressionQuality;
    Output->bSendKafka = Settings->bEnableSnapshotPublish;
    Output->bSaveToFile = false; //Settings->bEnableSnapshotLocalSaveFile;
    Output->WriteOptions.bAsync = true;
    Output->WriteOptions.bOverwriteFile = true;
    Output->WriteOptions.Format = EDesiredImageFormat::JPG;
    Output->bOverrideDesiredSize = Settings->bCropDesiredSize;
    Output->DesiredSize = Settings->DesiredSize;
    if(((int)Settings->imageType) == 1)
    {
        Output->WriteOptions.Format = EDesiredImageFormat::PNG;
    }
    Output->FilePath = FDirectoryPath{ FPaths::VideoCaptureDir() };// SnapshotDirectory;
    Output->BaseFileName = FString(TEXT("snap_"));
    MediaCapture = Output->CreateMediaCapture();

    FMediaCaptureOptions CaptureOptions;
    if(Settings->bCropDesiredSize)
    {
        CaptureOptions.Crop = EMediaCaptureCroppingType::Center;
    }
    //this potentially costs performance but is good to have to avoid repackage
    CaptureOptions.bConvertToDesiredPixelFormat = true;

    if(!MediaCapture->CaptureActiveSceneViewport(CaptureOptions))
    {
        UE_LOG(LogTackSnapshots, Error, TEXT("Failed to start Snapshot Capture"));
    }
}
void UTackSnapshotSubsystem::OnTackEnd_Implementation()
{
    Super::OnTackEnd_Implementation();
    StopMediaCapture();
}

void UTackSnapshotSubsystem::StopMediaCapture()
{
    if(MediaCapture != nullptr)
    {
        MediaCapture->StopCapture(true);
        MediaCapture = nullptr;
    }
}