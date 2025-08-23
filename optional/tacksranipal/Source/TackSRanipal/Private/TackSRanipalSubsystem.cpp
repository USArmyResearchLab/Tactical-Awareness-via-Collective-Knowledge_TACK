#include "TackSRanipalSubsystem.h"
#include "TackSRanipalEyeTrackerComponent.h"

bool UTackSRanipalSubsystem::ShouldCreateSubsystem(UObject* Outer) const { return true; }
bool UTackSRanipalSubsystem::IsEyeTrackerConnected() const { return true; }


TSubclassOf<UTackEyeTrackerComponent> UTackSRanipalSubsystem::GetTackEyeTrackerComponentClass() const
{
    return UTackSRanipalEyeTrackerComponent::StaticClass();
}