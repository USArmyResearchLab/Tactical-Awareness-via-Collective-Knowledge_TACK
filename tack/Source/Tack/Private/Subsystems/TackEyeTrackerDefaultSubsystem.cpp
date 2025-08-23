#include "Subsystems/TackEyeTrackerDefaultSubsystem.h"
#include "Devices/EyeTracker/TackDefaultEyeTrackerComponent.h"
#include "TackSettings.h"

bool UTackEyeTrackerDefaultSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    const UTackSettings* Settings = GetDefault<UTackSettings>();
    return Settings->bForceDefaultEyetrackerPublisher;
}
bool UTackEyeTrackerDefaultSubsystem::IsEyeTrackerConnected() const { return true; }

TSubclassOf<UTackEyeTrackerComponent> UTackEyeTrackerDefaultSubsystem::GetTackEyeTrackerComponentClass() const
{
    return UTackDefaultEyeTrackerComponent::StaticClass();
}