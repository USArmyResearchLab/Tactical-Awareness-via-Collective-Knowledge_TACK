#pragma once
#include "Devices/EyeTracker/TackEyeTrackerComponent.h"
#include "TobiiProTypes.h"
#include "MessageEndpoint.h"
#include "TackTobiiProEyeTrackerComponent.generated.h"

UCLASS()
class UTackTobiiProEyeTrackerComponent : public UTackEyeTrackerComponent
{
    GENERATED_BODY()
public:

    UTackTobiiProEyeTrackerComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void OnTackStart_Implementation() override;
    virtual void OnTackEnd_Implementation() override;

    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    TSharedPtr<FMessageEndpoint, ESPMode::ThreadSafe> MessageEndpoint_Inbox;
    void ProcessMessages();

    void OnGazeDataEvent(const FTobiiProGazeData& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context);

    FTackJsonDomBuilder::FTackObject ParseTobiiProGazeData(const FTobiiProGazeData& GazeMsg) const;
    FTackJsonDomBuilder::FTackObject ParseTobiiProEyeData(const FTobiiProEyeData& Eye) const;
    FTackJsonDomBuilder::FTackObject ParseTobiiProGazePointData(const FTobiiProGazePoint& GazePoint) const;
    FTackJsonDomBuilder::FTackObject ParseTobiiProPupilData(const FTobiiProPupilData& Pupil) const;
    FTackJsonDomBuilder::FTackObject ParseTobiiProGazeOrigin(const FTobiiProGazeOrigin& GazeOrigin) const;

    FTackJsonDomBuilder::FTackObject ParseTobiiProDisplayArea(const FTobiiProDisplayArea& DisplayArea) const;
    FTackJsonDomBuilder::FTackObject ParseTobiiEyeUserPositionGuide(const FTobiiProEyeUserPositionGuide& EyeUserPositionGuide) const;

    void OnUserPositionGuideEvent(const FTobiiProUserPositionGuide& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context);
    void OnExternalsignalDataEvent(const FTobiiProExternalSignalData& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context);
    void OnTimeSynchronizationDataEvent(const FTobiiProTimeSynchronizationData& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context);
    void OnStreamErrorDataEvent(const FTobiiProStreamErrorData& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context);
    void OnNotificationEvent(const FTobiiProNotification& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context);
    void OnEyeOpennessEvent(const FTobiiProEyeOpennessData& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context);
};