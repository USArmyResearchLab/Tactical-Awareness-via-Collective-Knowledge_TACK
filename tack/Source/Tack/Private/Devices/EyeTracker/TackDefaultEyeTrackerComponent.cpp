#include "Devices/EyeTracker/TackDefaultEyeTrackerComponent.h"
#include "EyeTrackerFunctionLibrary.h"
#include "IEyeTrackerModule.h"

UTackDefaultEyeTrackerComponent::UTackDefaultEyeTrackerComponent() : UTackEyeTrackerComponent()
{

}

void UTackDefaultEyeTrackerComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UTackDefaultEyeTrackerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void UTackDefaultEyeTrackerComponent::OnTackStart_Implementation()
{
    Super::OnTackStart_Implementation();
}

void UTackDefaultEyeTrackerComponent::OnTackEnd_Implementation()
{
    Super::OnTackEnd_Implementation();
}

void UTackDefaultEyeTrackerComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    //TODO Take into account confidence values
    if(UEyeTrackerFunctionLibrary::IsEyeTrackerConnected())
    {
        FEyeTrackerGazeData GazeData;
        FTackEyeTrackerTraceAggregateQueryParams QueryParams;

        if(UEyeTrackerFunctionLibrary::GetGazeData(GazeData))
        {
            QueryParams.Main = FTackEyeTrackerTraceQueryParams(GazeData.GazeOrigin, GazeData.GazeDirection);
        }
        else
        {
            return;
        }

        if(UEyeTrackerFunctionLibrary::IsStereoGazeDataAvailable())
        {
            FEyeTrackerStereoGazeData StereoGazeData;
            if(UEyeTrackerFunctionLibrary::GetStereoGazeData(StereoGazeData))
            {
                QueryParams.Right = FTackEyeTrackerTraceQueryParams(StereoGazeData.RightEyeOrigin, StereoGazeData.RightEyeDirection);
                QueryParams.Left = FTackEyeTrackerTraceQueryParams(StereoGazeData.LeftEyeOrigin, StereoGazeData.LeftEyeDirection);
            }
        }
        PublishTrace(QueryParams);
    }
}