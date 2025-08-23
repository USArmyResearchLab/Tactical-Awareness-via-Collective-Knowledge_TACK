#include "TackSRanipalEyeTrackerComponent.h"
#include "GameFramework/GameStateBase.h"
#include "IEyeTrackerModule.h"
#include "EyeTrackerFunctionLibrary.h"

UTackSRanipalEyeTrackerComponent::UTackSRanipalEyeTrackerComponent() : UTackEyeTrackerComponent()
{

}

std::atomic<float> UTackSRanipalEyeTrackerComponent::CurrentServerWorldTime = 0.0f;
TQueue<FSRanipalEyeDataTuple, EQueueMode::Spsc> UTackSRanipalEyeTrackerComponent::EyeDataQueue;

void UTackSRanipalEyeTrackerComponent::EyeDataCallback_v2(ViveSR::anipal::Eye::EyeData_v2 const& eye_data)
{
    UTackSRanipalEyeTrackerComponent::EyeDataQueue.Enqueue(
        FSRanipalEyeDataTuple(eye_data, GFrameCounter)
    );
}

void UTackSRanipalEyeTrackerComponent::CleanUp()
{
    ViveSR::anipal::Eye::UnregisterEyeDataCallback_v2(UTackSRanipalEyeTrackerComponent::EyeDataCallback_v2);
    EyeDataQueue.Empty();
    SRanipalConnected = false;
}

void UTackSRanipalEyeTrackerComponent::OnTackStart_Implementation()
{
    Super::OnTackStart_Implementation();
}

void UTackSRanipalEyeTrackerComponent::OnTackEnd_Implementation()
{
    Super::OnTackEnd_Implementation();
    CleanUp();
}

void UTackSRanipalEyeTrackerComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    CurrentServerWorldTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();

    if(
        !SRanipalConnected &&
        UEyeTrackerFunctionLibrary::IsEyeTrackerConnected() &&
        IEyeTrackerModule::Get().GetModuleKeyName() == TEXT("SRanipalEyeTrackerModule")
        )
    {
        ViveSR::anipal::Eye::RegisterEyeDataCallback_v2(UTackSRanipalEyeTrackerComponent::EyeDataCallback_v2);
        SRanipalConnected = true;
    }

    FSRanipalEyeDataTuple EyeDataRef;
    while(EyeDataQueue.Dequeue(EyeDataRef))
    {
        ProcessEyeData(EyeDataRef);
    }
}

void UTackSRanipalEyeTrackerComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UTackSRanipalEyeTrackerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    CleanUp();
}

void UTackSRanipalEyeTrackerComponent::ProcessEyeData(FSRanipalEyeDataTuple const& EyeDataTuple)
{
    static const FVector INVALID_COMBINED_GAZE_VECTOR = FVector(-1.0f, -1.0f, -1.0f);

    auto const& EyeData = EyeDataTuple.Get<0>();

    FTackEyeTrackerTraceAggregateQueryParams AggregateTraceResults(EyeData.timestamp);

    const FVector PlayerCameraLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
    const FRotator PlayerCameraRotation = PlayerController->PlayerCameraManager->GetCameraRotation();

    //We may only need to check for main. Seems like non main is always true if main is true?

    if(EyeData.verbose_data.combined.eye_data.gaze_direction_normalized != INVALID_COMBINED_GAZE_VECTOR)
    {
        {
            FVector direction = EyeData.verbose_data.combined.eye_data.gaze_direction_normalized;
            direction.X *= -1;
            CovertToUnrealLocation(direction);

            const FVector RayCastDirection = (PlayerCameraRotation.RotateVector(direction) * 100000) + PlayerCameraLocation;

            AggregateTraceResults.Main = FTackEyeTrackerTraceQueryParams(PlayerCameraLocation, RayCastDirection);
        }

        if(!EyeData.verbose_data.right.gaze_direction_normalized.IsZero())
        {
            FVector direction = EyeData.verbose_data.right.gaze_direction_normalized;
            direction.X *= -1;
            CovertToUnrealLocation(direction);

            const FVector RayCastDirection = (PlayerCameraRotation.RotateVector(direction) * 100000) + PlayerCameraLocation;

            AggregateTraceResults.Right = FTackEyeTrackerTraceQueryParams(PlayerCameraLocation, RayCastDirection);
        }

        if(!EyeData.verbose_data.left.gaze_direction_normalized.IsZero())
        {
            FVector direction = EyeData.verbose_data.left.gaze_direction_normalized;
            direction.X *= -1;
            CovertToUnrealLocation(direction);

            const FVector RayCastDirection = (PlayerCameraRotation.RotateVector(direction) * 100000) + PlayerCameraLocation;

            AggregateTraceResults.Left = FTackEyeTrackerTraceQueryParams(PlayerCameraLocation, RayCastDirection);
        }
    }
    else if(!EyeData.verbose_data.right.gaze_direction_normalized.IsZero())
    {

        FVector direction = EyeData.verbose_data.right.gaze_direction_normalized;
        direction.X *= -1;
        CovertToUnrealLocation(direction);

        const FVector RayCastDirection = (PlayerCameraRotation.RotateVector(direction) * 100000) + PlayerCameraLocation;

        AggregateTraceResults.Right = FTackEyeTrackerTraceQueryParams(PlayerCameraLocation, RayCastDirection);

        //When Right is only valid we copy it into main
        AggregateTraceResults.Main = AggregateTraceResults.Right;
    }
    else if(!EyeData.verbose_data.left.gaze_direction_normalized.IsZero())
    {
        FVector direction = EyeData.verbose_data.left.gaze_direction_normalized;
        direction.X *= -1;
        CovertToUnrealLocation(direction);

        const FVector RayCastDirection = (PlayerCameraRotation.RotateVector(direction) * 100000) + PlayerCameraLocation;

        //When Left is only valid we copy it into main
        AggregateTraceResults.Left = FTackEyeTrackerTraceQueryParams(PlayerCameraLocation, RayCastDirection);

        AggregateTraceResults.Main = AggregateTraceResults.Left;
    }

    AggregateTraceResults.Raw = ParseSRanipalData(EyeData);
    PublishTrace(AggregateTraceResults);
}

FTackJsonDomBuilder::FTackObject  UTackSRanipalEyeTrackerComponent::ParseSRanipalData(ViveSR::anipal::Eye::EyeData_v2 const& EyeData) const
{
    FTackJsonDomBuilder::FTackObject Json;
    Json.Set("no_user", EyeData.no_user)
        .Set("frame_sequence", EyeData.frame_sequence)
        .Set("timestamp", EyeData.timestamp);

    FTackJsonDomBuilder::FTackObject VerboseDataJson;
    VerboseDataJson.Set("left", ParseSingleEyeData(EyeData.verbose_data.left));
    VerboseDataJson.Set("right", ParseSingleEyeData(EyeData.verbose_data.right));
    VerboseDataJson.Set("combined", ParseCombinedEyeData(EyeData.verbose_data.combined));

    Json.Set("verbose_data", VerboseDataJson);

    return Json;
}

FTackJsonDomBuilder::FTackObject  UTackSRanipalEyeTrackerComponent::ParseSingleEyeData(ViveSR::anipal::Eye::SingleEyeData const& SingleEyeData) const
{
    FTackJsonDomBuilder::FTackObject Json;

    //Gaze Origin Json
    {
        FTackJsonDomBuilder::FTackObject GazeOriginJson;
        GazeOriginJson.Set("X", SingleEyeData.gaze_origin_mm.X);
        GazeOriginJson.Set("Y", SingleEyeData.gaze_origin_mm.Y);
        GazeOriginJson.Set("Z", SingleEyeData.gaze_origin_mm.Z);

        Json.Set("gaze_origin_mm", GazeOriginJson);
        Json.Set("gaze_origin_validity", const_cast<ViveSR::anipal::Eye::SingleEyeData&>(SingleEyeData).GetValidity(ViveSR::anipal::Eye::SINGLE_EYE_DATA_GAZE_ORIGIN_VALIDITY));
    }

    //Gaze Direction Json
    {
        FTackJsonDomBuilder::FTackObject GazeDirectionJson;
        GazeDirectionJson.Set("X", SingleEyeData.gaze_direction_normalized.X);
        GazeDirectionJson.Set("Y", SingleEyeData.gaze_direction_normalized.Y);
        GazeDirectionJson.Set("Z", SingleEyeData.gaze_direction_normalized.Z);

        Json.Set("gaze_direction_normalized", GazeDirectionJson);
        Json.Set("gaze_direction_validity", const_cast<ViveSR::anipal::Eye::SingleEyeData&>(SingleEyeData).GetValidity(ViveSR::anipal::Eye::SINGLE_EYE_DATA_GAZE_DIRECTION_VALIDITY));
    }

    //Pupil Json
    {
        Json.Set("pupil_diameter_mm", SingleEyeData.pupil_diameter_mm);
        Json.Set("pupil_diameter_validity", const_cast<ViveSR::anipal::Eye::SingleEyeData&>(SingleEyeData).GetValidity(ViveSR::anipal::Eye::SINGLE_EYE_DATA_PUPIL_DIAMETER_VALIDITY));

        FTackJsonDomBuilder::FTackObject PupilPositionInSensorArea;
        PupilPositionInSensorArea.Set("X", SingleEyeData.pupil_position_in_sensor_area.X);
        PupilPositionInSensorArea.Set("Y", SingleEyeData.pupil_position_in_sensor_area.Y);
        Json.Set("pupil_position_in_sensor_area", PupilPositionInSensorArea);
        Json.Set("pupil_position_validity", const_cast<ViveSR::anipal::Eye::SingleEyeData&>(SingleEyeData).GetValidity(ViveSR::anipal::Eye::SINGLE_EYE_DATA_PUPIL_POSITION_IN_SENSOR_AREA_VALIDITY));
    }

    //eye_openness
    {
        Json.Set("eye_openness", SingleEyeData.eye_openness);
        Json.Set("eye_openness_validity", const_cast<ViveSR::anipal::Eye::SingleEyeData&>(SingleEyeData).GetValidity(ViveSR::anipal::Eye::SINGLE_EYE_DATA_EYE_OPENNESS_VALIDITY));
    }

    return Json;
}

FTackJsonDomBuilder::FTackObject  UTackSRanipalEyeTrackerComponent::ParseCombinedEyeData(ViveSR::anipal::Eye::CombinedEyeData const& CombinedEyeData) const
{
    FTackJsonDomBuilder::FTackObject Json;
    Json.Set("eye_data", ParseSingleEyeData(CombinedEyeData.eye_data));
    Json.Set("convergence_distance", CombinedEyeData.convergence_distance_mm);
    Json.Set("convergence_distance_validity", CombinedEyeData.convergence_distance_validity);
    return Json;
}

void UTackSRanipalEyeTrackerComponent::CovertToUnrealLocation(FVector& vector) const
{
    FVector temp = vector;
    vector.X = temp.Z;
    vector.Y = temp.X;
    vector.Z = temp.Y;
}

void UTackSRanipalEyeTrackerComponent::ApplyUnrealWorldToMeterScale(FVector& vector) const
{
    float scale = GetWorld()->GetWorldSettings()->WorldToMeters;
    vector *= scale;
}

void UTackSRanipalEyeTrackerComponent::CovertToUnrealQuaternion(FQuat& quat) const
{
    FQuat temp = quat;
    quat.X = temp.Z;
    quat.Y = temp.X;
    quat.Z = temp.Y;
    quat.W = temp.W;
}