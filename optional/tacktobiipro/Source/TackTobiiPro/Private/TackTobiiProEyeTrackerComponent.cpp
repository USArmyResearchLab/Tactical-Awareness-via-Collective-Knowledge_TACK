#include "TackTobiiProEyeTrackerComponent.h"
#include "TobiiProTypes.h"
#include "GameFramework/GameStateBase.h"
#include "EyeTrackerFunctionLibrary.h"
#include "Serialization/TackJsonDomBuilder.h"
#include "MessageEndpointBuilder.h"
#include "TackManager.h"
#include "TackPublisher.h"


UTackTobiiProEyeTrackerComponent::UTackTobiiProEyeTrackerComponent() : UTackEyeTrackerComponent()
{

}

void UTackTobiiProEyeTrackerComponent::BeginPlay()
{
    Super::BeginPlay();

    //UEyeTrackerFunctionLibrary::SetEyeTrackedPlayer(PlayerController);

    MessageEndpoint_Inbox = FMessageEndpoint::Builder(TEXT("TackTobiiProEyeTrackerComponentEndpoint"))
        .Handling<FTobiiProGazeData>(this, &UTackTobiiProEyeTrackerComponent::OnGazeDataEvent)
        .Handling<FTobiiProUserPositionGuide>(this, &UTackTobiiProEyeTrackerComponent::OnUserPositionGuideEvent)
        .Handling<FTobiiProExternalSignalData>(this, &UTackTobiiProEyeTrackerComponent::OnExternalsignalDataEvent)
        .Handling<FTobiiProTimeSynchronizationData>(this, &UTackTobiiProEyeTrackerComponent::OnTimeSynchronizationDataEvent)
        .Handling<FTobiiProStreamErrorData>(this, &UTackTobiiProEyeTrackerComponent::OnStreamErrorDataEvent)
        .Handling<FTobiiProNotification>(this, &UTackTobiiProEyeTrackerComponent::OnNotificationEvent)
        .Handling<FTobiiProEyeOpennessData>(this, &UTackTobiiProEyeTrackerComponent::OnEyeOpennessEvent)
        .ReceivingOnAnyThread();

    MessageEndpoint_Inbox->EnableInbox();
    MessageEndpoint_Inbox->Subscribe<FTobiiProGazeData>(FMessageScopeRange::AtMost(EMessageScope::Process));
    MessageEndpoint_Inbox->Subscribe<FTobiiProUserPositionGuide>(FMessageScopeRange::AtMost(EMessageScope::Process));
    MessageEndpoint_Inbox->Subscribe<FTobiiProExternalSignalData>(FMessageScopeRange::AtMost(EMessageScope::Process));
    MessageEndpoint_Inbox->Subscribe<FTobiiProTimeSynchronizationData>(FMessageScopeRange::AtMost(EMessageScope::Process));
    MessageEndpoint_Inbox->Subscribe<FTobiiProStreamErrorData>(FMessageScopeRange::AtMost(EMessageScope::Process));
    MessageEndpoint_Inbox->Subscribe<FTobiiProNotification>(FMessageScopeRange::AtMost(EMessageScope::Process));
    MessageEndpoint_Inbox->Subscribe<FTobiiProEyeOpennessData>(FMessageScopeRange::AtMost(EMessageScope::Process));

}

void UTackTobiiProEyeTrackerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    ProcessMessages();

    if(MessageEndpoint_Inbox)
    {
        MessageEndpoint_Inbox->Unsubscribe();
        FMessageEndpoint::SafeRelease(MessageEndpoint_Inbox);
        MessageEndpoint_Inbox.Reset();
    }
}

void UTackTobiiProEyeTrackerComponent::OnTackStart_Implementation()
{
    Super::OnTackStart_Implementation();

}

void UTackTobiiProEyeTrackerComponent::OnTackEnd_Implementation()
{
    Super::OnTackEnd_Implementation();
}


void UTackTobiiProEyeTrackerComponent::ProcessMessages()
{
    if(MessageEndpoint_Inbox)
        MessageEndpoint_Inbox->ProcessInbox();
}

void UTackTobiiProEyeTrackerComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    ProcessMessages();
}

void UTackTobiiProEyeTrackerComponent::OnGazeDataEvent(const FTobiiProGazeData& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
{
    const FTobiiProGazePoint& LeftGazePoint = Message.LeftEye.GazePoint;
    const FTobiiProGazePoint& RightGazePoint = Message.RightEye.GazePoint;

    FTackEyeTrackerTraceAggregateQueryParams QueryParams; /*Add in source utc*/

    if(LeftGazePoint.Valid && RightGazePoint.Valid)
    {
        //if both Gazes are valid we do a combined trace and then a Left and Right trace

        QueryParams.Main = BuildParamsFromDisplayArea({
            (RightGazePoint.PositionOnDisplayArea.X + LeftGazePoint.PositionOnDisplayArea.X) / 2.0f,
            (RightGazePoint.PositionOnDisplayArea.Y + LeftGazePoint.PositionOnDisplayArea.Y) / 2.0f
            });

        QueryParams.Right = BuildParamsFromDisplayArea({
            RightGazePoint.PositionOnDisplayArea.X,
            RightGazePoint.PositionOnDisplayArea.Y
            });

        QueryParams.Left = BuildParamsFromDisplayArea({
            LeftGazePoint.PositionOnDisplayArea.X,
            LeftGazePoint.PositionOnDisplayArea.Y
            });

    }
    else if(RightGazePoint.Valid)
    {
        //When Right is only valid we do a right trace then copy it into main 
        QueryParams.Right = BuildParamsFromDisplayArea({
            RightGazePoint.PositionOnDisplayArea.X,
            RightGazePoint.PositionOnDisplayArea.Y
            });

        QueryParams.Main = QueryParams.Right;

    }
    else if(LeftGazePoint.Valid)
    {
        //When Left is only valid we do a Left trace then copy it into main 
        QueryParams.Left = BuildParamsFromDisplayArea({
            LeftGazePoint.PositionOnDisplayArea.X,
            LeftGazePoint.PositionOnDisplayArea.Y
            });

        QueryParams.Main = QueryParams.Left;

    }
    else
    {
        return;
    }

    QueryParams.Raw = ParseTobiiProGazeData(Message);

    PublishTrace(QueryParams);
}

FTackJsonDomBuilder::FTackObject UTackTobiiProEyeTrackerComponent::ParseTobiiProGazeData(const FTobiiProGazeData& GazeMsg) const
{
    FTackJsonDomBuilder::FTackObject Json;
    Json.Set("LeftEye", ParseTobiiProEyeData(GazeMsg.LeftEye))
        .Set("RightEye", ParseTobiiProEyeData(GazeMsg.RightEye))
        .Set("DeviceTimetstamp", GazeMsg.DeviceTimestamp)
        .Set("SystemTimestamp", GazeMsg.SystemTimestamp);
    return Json;
}

FTackJsonDomBuilder::FTackObject UTackTobiiProEyeTrackerComponent::ParseTobiiProEyeData(const FTobiiProEyeData& Eye) const
{
    FTackJsonDomBuilder::FTackObject Json;
    Json.Set("GazePoint", ParseTobiiProGazePointData(Eye.GazePoint))
        .Set("PupilData", ParseTobiiProPupilData(Eye.PupilData))
        .Set("GazeOrigin", ParseTobiiProGazeOrigin(Eye.GazeOrigin));
    return Json;
}

FTackJsonDomBuilder::FTackObject UTackTobiiProEyeTrackerComponent::ParseTobiiProGazePointData(const FTobiiProGazePoint& GazePoint) const
{
    FTackJsonDomBuilder::FTackObject Json;
    Json.Set("Valid", GazePoint.Valid)
        .Set("PositionOnDisplayArea", FTackJsonDomBuilder::Serialize(GazePoint.PositionOnDisplayArea))
        .Set("PositionInUserCoordinates", FTackJsonDomBuilder::Serialize(GazePoint.PositionInUserCoordinates));
    return Json;
}

FTackJsonDomBuilder::FTackObject UTackTobiiProEyeTrackerComponent::ParseTobiiProPupilData(const FTobiiProPupilData& Pupil) const
{
    FTackJsonDomBuilder::FTackObject Json;
    Json.Set("Valid", Pupil.Valid)
        .Set("Diameter", Pupil.Diameter);
    return Json;
}

FTackJsonDomBuilder::FTackObject UTackTobiiProEyeTrackerComponent::ParseTobiiProGazeOrigin(const FTobiiProGazeOrigin& GazeOrigin) const
{
    FTackJsonDomBuilder::FTackObject Json;
    Json.Set("Valid", GazeOrigin.Valid)
        .Set("PositionInUserCoordinates", FTackJsonDomBuilder::Serialize(GazeOrigin.PositionInUserCoordinates))
        .Set("PositionInTrackBoxCoordinates", FTackJsonDomBuilder::Serialize(GazeOrigin.PositionInTrackBoxCoordinates));
    return Json;

}

void UTackTobiiProEyeTrackerComponent::OnUserPositionGuideEvent(const FTobiiProUserPositionGuide& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
{
    FTackJsonDomBuilder::FTackObject Json;
    Json.Set("LeftEye", ParseTobiiEyeUserPositionGuide(Message.LeftEye))
        .Set("RightEye", ParseTobiiEyeUserPositionGuide(Message.RightEye));
    GetTackManager()->GetPublisher()->Publish_Json(TEXT("unreal.client.device.tobiipro.userpositionguide"), Json.AsJsonObject());
}

void UTackTobiiProEyeTrackerComponent::OnExternalsignalDataEvent(const FTobiiProExternalSignalData& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
{
    FTackJsonDomBuilder::FTackObject Json;
    Json.Set("DeviceTimestmap", Message.DeviceTimestamp)
        .Set("SystemTimestamp", Message.SystemTimestamp)
        .Set("Value", Message.Value)
        .Set("ChangeType", Message.ChangeType);
    GetTackManager()->GetPublisher()->Publish_Json(TEXT("unreal.client.device.tobiipro.externalsignal"), Json.AsJsonObject());
}

void UTackTobiiProEyeTrackerComponent::OnTimeSynchronizationDataEvent(const FTobiiProTimeSynchronizationData& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
{
    FTackJsonDomBuilder::FTackObject Json;
    Json.Set("SystemRequestTimestamp", Message.SystemRequestTimestamp)
        .Set("DeviceTimestamp", Message.DeviceTimestamp)
        .Set("SystemResponseTimestamp", Message.SystemResponseTimestamp);
    GetTackManager()->GetPublisher()->Publish_Json(TEXT("unreal.client.device.tobiipro.timesynchronization"), Json.AsJsonObject());
}

void UTackTobiiProEyeTrackerComponent::OnStreamErrorDataEvent(const FTobiiProStreamErrorData& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
{
    FTackJsonDomBuilder::FTackObject Json;
    Json.Set("SystemTimestamp", Message.SystemTimestamp)
        .Set("Error", Message.Error)
        .Set("Source", Message.Source)
        .Set("Message", Message.Message);
    GetTackManager()->GetPublisher()->Publish_Json(TEXT("unreal.client.device.tobiipro.streamerror"), Json.AsJsonObject());
}

void UTackTobiiProEyeTrackerComponent::OnNotificationEvent(const FTobiiProNotification& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
{
    FTackJsonDomBuilder::FTackObject Json;
    Json.Set("SystemTimestamp", Message.SystemTimestamp)
        .Set("NotificationType", Message.NotificationType);
    switch(Message.NotificationType)
    {
    case ETobiiProNotificationType::DisplayAreaChanged:
        //const FTobiiProDisplayArea& DisplayArea = static_cast<const FTobiiProDisplayAreaChangedNotification&>(Message).DisplayArea;
        Json.Set(
            "DisplayArea",
            ParseTobiiProDisplayArea(static_cast<const FTobiiProDisplayAreaChangedNotification&>(Message).DisplayArea)
        );
        break;
    case ETobiiProNotificationType::GazeOutputFrequencyChanged:
        //float OutputFrequency = static_cast<const FTobiiProGazeOutputFrequencyChangedNotification&>(Message).OutputFrequency;
        Json.Set(
            "OutputFrequency",
            static_cast<const FTobiiProGazeOutputFrequencyChangedNotification&>(Message).OutputFrequency
        );
        break;
    case ETobiiProNotificationType::DeviceFaults:
        Json.Set(
            "Faults",
            static_cast<const FTobiiProDeviceFaultsNotification&>(Message).Faults
        );
        break;
    case ETobiiProNotificationType::DeviceWarnings:
        Json.Set(
            "Warnings",
            static_cast<const FTobiiProDeviceWarningsNotification&>(Message).Warnings
        );
        break;
    }

    GetTackManager()->GetPublisher()->Publish_Json(TEXT("unreal.client.device.tobiipro.notification"), Json.AsJsonObject());
}

void UTackTobiiProEyeTrackerComponent::OnEyeOpennessEvent(const FTobiiProEyeOpennessData& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
{
    FTackJsonDomBuilder::FTackObject Json;
    Json.Set("DeviceTimestamp", Message.DeviceTimestamp)
        .Set("SystemTimestamp", Message.SystemTimestamp)
        .Set("LeftEyeValid", Message.LeftEyeValid)
        .Set("LeftEyeOpennessValue", Message.LeftEyeOpennessValue)
        .Set("RightEyeValid", Message.RightEyeValid)
        .Set("RightEyeOpennessValue", Message.RightEyeOpennesValue);
    GetTackManager()->GetPublisher()->Publish_Json(TEXT("unreal.client.device.tobiipro.eyeopenness"), Json.AsJsonObject());
}

FTackJsonDomBuilder::FTackObject UTackTobiiProEyeTrackerComponent::ParseTobiiProDisplayArea(const FTobiiProDisplayArea& DisplayArea) const
{
    FTackJsonDomBuilder::FTackObject Json;
    Json.Set("BottomLeft", FTackJsonDomBuilder::Serialize(DisplayArea.BottomLeft))
        .Set("BottomRight", FTackJsonDomBuilder::Serialize(DisplayArea.BottomRight))
        .Set("Height", DisplayArea.Height)
        .Set("TopLeft", FTackJsonDomBuilder::Serialize(DisplayArea.TopLeft))
        .Set("TopRight", FTackJsonDomBuilder::Serialize(DisplayArea.TopRight))
        .Set("Width", DisplayArea.Width);
    return Json;
}

FTackJsonDomBuilder::FTackObject UTackTobiiProEyeTrackerComponent::ParseTobiiEyeUserPositionGuide(const FTobiiProEyeUserPositionGuide& EyeUserPositionGuide) const
{
    FTackJsonDomBuilder::FTackObject Json;
    Json.Set("UserPosition", FTackJsonDomBuilder::Serialize(EyeUserPositionGuide.UserPosition))
        .Set("Valid", EyeUserPositionGuide.Valid);
    return Json;
}