#include "TackXRSubsystem.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "TackManager.h"
#include "TackPublisher.h"
#include "Serialization/TackJsonDomBuilder.h"
#include "Serialization/TackJsonStructSerializer.h"
#include "IXRTrackingSystem.h"
#include "IMotionController.h"
#include "XRMotionControllerBase.h"
#include "IHandTracker.h"
#include "MotionControllerComponent.h"
#include "UObject/UObjectIterator.h"
#include "IXRSystemAssets.h"


bool UTackXRSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    return !IsRunningDedicatedServer();
}

void UTackXRSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    //Below features were added in UE4.27 if this module is failing or not publishing correct data they may need updated
    //Engine\Plugins\Lumin\MagicLeap\Source\MagicLeapController\Private\MagicLeapController.h
    FXRMotionControllerBaseSupport.Add(TEXT("MagicLeapController"));
    //Engine\Plugins\Lumin\MagicLeap\Source\MagicLeapHandTracking\Private\MagicLeapHandTracking.h
    FXRMotionControllerBaseSupport.Add(TEXT("MagicLeapHandTracking"));
    //Engine\Plugins\Runtime\GoogleVR\GoogleVRController\Source\GoogleVRController\Private\GoogleVRController.h
    FXRMotionControllerBaseSupport.Add(TEXT("GoogleVRController"));
    //Engine\Plugins\Runtime\Oculus\OculusVR\Source\OculusInput\Private\OculusInput.h
    FXRMotionControllerBaseSupport.Add(TEXT("OculusInputDevice"));
    //Engine\Plugins\Runtime\OpenXR\Source\OpenXRInput\Private\OpenXRInput.h
    FXRMotionControllerBaseSupport.Add(TEXT("OpenXR"));
    //Engine\Plugins\Runtime\OpenXRHandTracking\Source\OpenXRHandTracking\Private\OpenXRHandTracking.h
    FXRMotionControllerBaseSupport.Add(TEXT("OpenXRHandTracking"));
    //Engine\Plugins\Runtime\Steam\SteamVR\Source\SteamVRInputDevice\Private\SteamVRInputDevice.h
    FXRMotionControllerBaseSupport.Add(TEXT("SteamVRInputDevice"));
    //Engine\Plugins\Runtime\WindowsMixedReality\Source\WindowsMixedRealityHandTracking\Private\WindowsMixedRealityHandTracking.h
    FXRMotionControllerBaseSupport.Add(TEXT("WindowsMixedRealityHandTracking"));
    //Engine\Plugins\Runtime\WindowsMixedReality\Source\WindowsMixedRealitySpatialInput\Private\WindowsMixedRealitySpatialInput.h
    FXRMotionControllerBaseSupport.Add(TEXT("WindowsMixedRealitySpatialInput"));

    ActorWorldSpawnedHandle = GetWorld()->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &UTackXRSubsystem::OnActorSpawnedEvent));

    for(TObjectIterator<UMotionControllerComponent> Itt; Itt; ++Itt)
    {
        UMotionControllerComponent* Component = *Itt;
        if(IsValid(Component) && !Component->IsTemplate() && !Component->GetClass()->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated))
        {
            MotionControllerMap.Add(Component->MotionSource, Component);
        }
    }
}

void UTackXRSubsystem::Deinitialize()
{
    Super::Deinitialize();
    GetWorld()->RemoveOnActorSpawnedHandler(ActorWorldSpawnedHandle);
    FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
    MotionControllerMap.Empty();
}

void UTackXRSubsystem::OnTackStart_Implementation()
{
    Super::OnTackStart_Implementation();
    TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UTackXRSubsystem::Tick));
}

void UTackXRSubsystem::OnTackEnd_Implementation()
{
    Super::OnTackEnd_Implementation();
    FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
}

void UTackXRSubsystem::OnActorSpawnedEvent(AActor* Actor)
{
    if(UMotionControllerComponent* Component = Actor->FindComponentByClass<UMotionControllerComponent>())
    {
        MotionControllerMap.Add(Component->MotionSource, Component);
    }
}

bool UTackXRSubsystem::Tick(float dtime)
{
    if(GEngine == nullptr || !GEngine->XRSystem.IsValid())
        return true;

    //TArray<IXRSystemAssets*> XRAssetSystems = IModularFeatures::Get().GetModularFeatureImplementations<IXRSystemAssets>(IXRSystemAssets::GetModularFeatureName());
    TArray<IMotionController*> MotionControllers = IModularFeatures::Get().GetModularFeatureImplementations<IMotionController>(IMotionController::GetModularFeatureName());
    for(auto MotionController : MotionControllers)
    {
        if(MotionController == nullptr || !FXRMotionControllerBaseSupport.Contains(MotionController->GetMotionControllerDeviceTypeName()))
            continue;

        //This is garunteed by FXRMotionControllerBaseSupport array
        FXRMotionControllerBase* MotionControllerBase = static_cast<FXRMotionControllerBase*>(MotionController);

        TArray<FMotionControllerSource> Sources;
        MotionControllerBase->EnumerateSources(Sources);
        for(auto Source : Sources)
        {
            auto TrackingStatus = MotionControllerBase->GetControllerTrackingStatus(0, Source.SourceName);
            if(TrackingStatus == ETrackingStatus::NotTracked)
                continue;

            FTackJsonDomBuilder::FTackObject DeviceJson;
            DeviceJson.Set("MotionController", MotionControllerBase->GetMotionControllerDeviceTypeName());
            DeviceJson.Set("MotionSource", Source.SourceName);
            DeviceJson.Set("TrackingStatus", TrackingStatus);

            FRotator OutOrientation;
            FVector OutPosition;
            if(MotionControllerBase->GetControllerOrientationAndPosition(0, Source.SourceName, OutOrientation, OutPosition, GEngine->XRSystem->GetWorldToMetersScale()))
            {
                DeviceJson.Set("Orientation_Quat", OutOrientation.Quaternion());
                DeviceJson.Set("Orientation", OutOrientation.Euler());
                DeviceJson.Set("Position", OutPosition);
            }
            else
            {
                DeviceJson.Set("Orientation_Quat", nullptr);
                DeviceJson.Set("Orientation", nullptr);
                DeviceJson.Set("Position", nullptr);
            }

            EControllerHand OutControllerHand;
            if(FXRMotionControllerBase::GetHandEnumForSourceName(Source.SourceName, OutControllerHand))
            {
                DeviceJson.Set("HandEnumForSourceName", OutControllerHand);
            }
            else
            {
                DeviceJson.Set("HandEnumForSourceName", nullptr);
            }

            if(UMotionControllerComponent** MotionControllerComponentExists = MotionControllerMap.Find(Source.SourceName))
            {
                auto MotionControllerComponent = *MotionControllerComponentExists;
                DeviceJson.Set("Actor_id", MotionControllerComponent->GetOwner());
                DeviceJson.Set("Component", MotionControllerComponent);
            }
            else
            {
                DeviceJson.Set("Actor_id", nullptr);
                DeviceJson.Set("Component", nullptr);
            }

            GetTackManager()->GetPublisher()->Publish_Json(TEXT("unreal.client.xr.motioncontroller"), DeviceJson.AsJsonObject());
        }
    }
    return true;
}