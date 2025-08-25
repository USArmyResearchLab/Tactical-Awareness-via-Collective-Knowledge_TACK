#include "TackClientCameraPublisherComponent.h"
#include "TackSettings.h"
#include "Serialization/TackJsonDomBuilder.h"
#include "TackManager.h"
#include "TackPublisher.h"

DECLARE_CYCLE_STAT(TEXT("Client Camera Publish"), STAT_ClientCameraPublish, STATGROUP_Tack);


UTackClientCameraPublisherComponent::UTackClientCameraPublisherComponent() : UTackBaseComponent()
{
    //This is a Local component that should only spawn if client is standalone, client, or listenserver
    SetIsReplicatedByDefault(false);

    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    PrimaryComponentTick.SetTickFunctionEnable(true);
}

void UTackClientCameraPublisherComponent::InitializeComponent()
{
    Super::InitializeComponent();
    PlayerController = Cast<APlayerController>(GetOwner());
    checkf(PlayerController != nullptr, TEXT("UTackClientCameraPublisherComponent can only exist on PlayerController Actors"));
    checkf(PlayerController->IsLocalController(), TEXT("UTackClientCameraPublisherComponentcan only exist on LOCAL PlayerController Actors"));

    const UTackSettings* Settings = GetDefault<UTackSettings>();
    checkf(Settings->bEnableCameraTransformPublisher, TEXT("UTackClientCameraPublisherComponent created when bEnableCameraTransformPublisher was false"));

    SetComponentTickInterval(Settings->CameraTransformPublisherTickInterval);
    Tolerance = Settings->CameraTransformPublisherTolerance;
}

void UTackClientCameraPublisherComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void UTackClientCameraPublisherComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if(!GetTackManager()->IsTackRunning())
        return;

    SCOPE_CYCLE_COUNTER(STAT_ClientCameraPublish);

    APlayerCameraManager* PlayerCameraManager = PlayerController->PlayerCameraManager;

    if(PlayerCameraManager != nullptr)
    {

        const FMinimalViewInfo CurrentPOV = PlayerCameraManager->GetCameraCacheView();

        if(!CurrentPOV.Location.Equals(PreviousCameraLocation, Tolerance) || !CurrentPOV.Rotation.Equals(PreviousCameraRotation, Tolerance))
        {
            FTackJsonDomBuilder::FTackObject Json;
            Json.Set("Position", CurrentPOV.Location);
            Json.Set("Rotation", CurrentPOV.Rotation.Vector());
            Json.Set("Quat", CurrentPOV.Rotation.Quaternion());
            Json.Set("ServerWorldTime", PlayerCameraManager->GetCameraCacheTime());

            GetTackManager()->GetPublisher()->Publish_Json(TEXT("unreal.client.cameraposition"), Json.AsJsonObject());

            PreviousCameraLocation = CurrentPOV.Location;
            PreviousCameraRotation = CurrentPOV.Rotation;
        }
    }
}