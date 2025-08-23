#pragma once
#include "Components/ActorComponent.h"
#include "TackBaseComponent.h"
#include "TackClientCameraPublisherComponent.generated.h"

//This class only exists on Local PlayerControllers
UCLASS(Within = PlayerController)
class UTackClientCameraPublisherComponent : public UTackBaseComponent
{
    GENERATED_BODY()
public:
    UTackClientCameraPublisherComponent();

    virtual void InitializeComponent() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);

    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction);
private:
    class APlayerController* PlayerController;

    float Tolerance;
    FVector PreviousCameraLocation;
    FRotator PreviousCameraRotation;
};