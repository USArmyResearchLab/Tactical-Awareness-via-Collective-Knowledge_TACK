#pragma once

#include "Subsystems/TackSubsystem.h"
#include "IIdentifiableXRDevice.h"
#include "HeadMountedDisplayTypes.h"
#include "TackXRSubsystem.generated.h"

class IMotionController;
class UMotionControllerComponent;

UCLASS()
class UTackXRSubsystem : public UTackSubsystem
{
    GENERATED_BODY()

public:

    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    virtual void OnTackStart_Implementation() override;
    virtual void OnTackEnd_Implementation() override;

private:

    void OnActorSpawnedEvent(AActor* Actor);

    TMap<FName, UMotionControllerComponent*> MotionControllerMap;

    bool Tick(float DeltaTime);
    FDelegateHandle ActorWorldSpawnedHandle;

    FDelegateHandle TickHandle;

    TArray<FName> FXRMotionControllerBaseSupport;
};