#pragma once
#include "Subsystems/LocalPlayerSubsystem.h"
#include "TackEyeTrackerSubsystem.generated.h"

UCLASS(Abstract)
class TACK_API UTackEyeTrackerSubsystem : public ULocalPlayerSubsystem
{
    GENERATED_BODY()
public:

    UTackEyeTrackerSubsystem();

    virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return false; }
    virtual bool IsEyeTrackerConnected() const { return false; }
    virtual TSubclassOf<UTackEyeTrackerComponent> GetTackEyeTrackerComponentClass() const { return nullptr; }

    static const UTackEyeTrackerSubsystem* GetFirstConnectedEyeTrackerSubsystem(const UObject* WorldContextObject);
    static const TArray<UTackEyeTrackerSubsystem*>& GetEyeTrackerSubsystems(const UObject* WorldContextObject);
};