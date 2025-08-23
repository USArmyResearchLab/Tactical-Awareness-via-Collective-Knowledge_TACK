
#pragma once
#include "TackEyeTrackerComponent.h"
#include "TackDefaultEyeTrackerComponent.generated.h"

UCLASS()
class UTackDefaultEyeTrackerComponent : public UTackEyeTrackerComponent {
    GENERATED_BODY()
public:

    UTackDefaultEyeTrackerComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    virtual void OnTackStart_Implementation() override;
    virtual void OnTackEnd_Implementation() override;

    void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};