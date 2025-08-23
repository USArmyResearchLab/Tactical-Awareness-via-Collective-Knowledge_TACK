#include "Subsystems/TackEyeTrackerSubsystem.h"

UTackEyeTrackerSubsystem::UTackEyeTrackerSubsystem() : ULocalPlayerSubsystem()
{

}

const TArray<UTackEyeTrackerSubsystem*>& UTackEyeTrackerSubsystem::GetEyeTrackerSubsystems(const UObject* WorldContextObject)
{
    static const TArray<UTackEyeTrackerSubsystem*> EmptyReturnArray = TArray<UTackEyeTrackerSubsystem*>();

    if(GEngine != nullptr)
    {
        if(UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
        {
            if(ULocalPlayer* LocalPlayer = World->GetFirstLocalPlayerFromController())
            {
                return LocalPlayer->GetSubsystemArray<UTackEyeTrackerSubsystem>();
            }
        }
    }

    return EmptyReturnArray;
}

const UTackEyeTrackerSubsystem* UTackEyeTrackerSubsystem::GetFirstConnectedEyeTrackerSubsystem(const UObject* WorldContextObject)
{
    for(const UTackEyeTrackerSubsystem* EyeTrackerSubsystem : UTackEyeTrackerSubsystem::GetEyeTrackerSubsystems(WorldContextObject))
    {
        if(EyeTrackerSubsystem->IsEyeTrackerConnected())
        {
            return EyeTrackerSubsystem;
        }
    }
    return nullptr;
}