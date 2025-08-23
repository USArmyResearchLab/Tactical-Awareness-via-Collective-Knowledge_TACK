#pragma once

#include "Subsystems/TackEyeTrackerSubsystem.h"
#include "TackEyeTrackerDefaultSubsystem.generated.h"

UCLASS()
class UTackEyeTrackerDefaultSubsystem : public UTackEyeTrackerSubsystem
{
    GENERATED_BODY()
public:

    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

    virtual bool IsEyeTrackerConnected() const override;

    virtual TSubclassOf<UTackEyeTrackerComponent> GetTackEyeTrackerComponentClass() const override;
};