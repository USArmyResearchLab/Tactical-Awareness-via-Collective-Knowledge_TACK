#include "TackTobiiProSubsystem.h"
#include "TackTobiiProEyeTrackerComponent.h"

bool UTackTobiiProSubsystem::ShouldCreateSubsystem(UObject* Outer) const { return true; }
bool UTackTobiiProSubsystem::IsEyeTrackerConnected() const { return true; }

TSubclassOf<UTackEyeTrackerComponent> UTackTobiiProSubsystem::GetTackEyeTrackerComponentClass() const
{
    return UTackTobiiProEyeTrackerComponent::StaticClass();
}