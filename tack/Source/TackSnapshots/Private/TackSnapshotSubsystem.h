#pragma once
#include "Subsystems/TackSubsystem.h"
#include "TackSnapshotSubsystem.generated.h"

UCLASS()
class UTackSnapshotSubsystem : public UTackSubsystem
{
    GENERATED_BODY()
public:

    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    virtual void OnTackStart_Implementation() override;
    virtual void OnTackEnd_Implementation() override;
private:

    UPROPERTY()
    UMediaCapture* MediaCapture;

    void StopMediaCapture();
};