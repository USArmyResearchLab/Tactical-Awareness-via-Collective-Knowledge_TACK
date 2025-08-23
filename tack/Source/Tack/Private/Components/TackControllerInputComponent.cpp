#include "TackControllerInputComponent.h"
#include "Tack.h"
#include "TackStatics.h"
#include "Serialization/TackJsonDomBuilder.h"
#include "TackSettings.h"
#include "InputCoreTypes.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/PlayerInput.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "TackIdComponent.h"
#include "TackManager.h"
#include "TackPublisher.h"

DECLARE_CYCLE_STAT(TEXT("Input Mapped Action Publish"), STAT_Input_MappedActionPublish, STATGROUP_Tack);
DECLARE_CYCLE_STAT(TEXT("Input Mapped Axis Publish"), STAT_Input_MappedAxisPublish, STATGROUP_Tack);
DECLARE_CYCLE_STAT(TEXT("Input Raw Key Publish"), STAT_Input_RawKeyPublish, STATGROUP_Tack);
DECLARE_CYCLE_STAT(TEXT("Input Raw Axis Publish"), STAT_Input_RawAxisPublish, STATGROUP_Tack);

UTackControllerInputComponent::UTackControllerInputComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    //Only want to Record data on server
    SetIsReplicatedByDefault(false);

    bAutoActivate = true;
    bAllowReregistration = true;

    Priority = 1;
    bBlockInput = 0;
}

const FGuid& UTackControllerInputComponent::GetTackId() const
{
    return TackIdComponent->GetTackId();
}

bool UTackControllerInputComponent::HasValidTackId() const
{
    return TackIdComponent != nullptr && TackIdComponent->HasValidTackId();
}

void UTackControllerInputComponent::SetTackManager(UTackManager* NewTackManager)
{
    check(!HasBegunPlay())
        TackManager = NewTackManager;
}

void UTackControllerInputComponent::BeginPlay()
{
    check(TackManager);

    const UTackSettings* Settings = GetDefault<UTackSettings>();
    if(Settings->bEnableMappedInputPublisher)
        EnableMappedInputs();

    if(Settings->bEnableRawInputPublisher)
        EnableRawInputs();

    TackManager->OnTackStart.AddUObject(this, &UTackControllerInputComponent::OnTackStart);
    TackManager->OnTackEnd.AddUObject(this, &UTackControllerInputComponent::OnTackEnd);

    if(TackManager->IsTackRunning())
        OnTackStart();

    Super::BeginPlay();
}

void UTackControllerInputComponent::OnTackStart()
{
    const UTackSettings* Settings = GetDefault<UTackSettings>();
    if(Settings->bEnableMappedInputPublisher)
    {
        PublishActionMappings();
        PublishAxisMappings();
    }

    Cast<APlayerController>(GetOwner())->PushInputComponent(this);
}

void UTackControllerInputComponent::OnTackEnd()
{
    Cast<APlayerController>(GetOwner())->PopInputComponent(this);
}

/////////////////////////////// MAPPED INPUT
void UTackControllerInputComponent::EnableMappedInputs()
{
    UInputSettings* InputSettings = UInputSettings::GetInputSettings();

    TArray<FName> ActionNames;
    InputSettings->GetActionNames(ActionNames);
    for(FName Action : ActionNames)
    {
        AddMappedActionBinding(Action, IE_Pressed);
        AddMappedActionBinding(Action, IE_Released);
        AddMappedActionBinding(Action, IE_Repeat);
        AddMappedActionBinding(Action, IE_DoubleClick);
        AddMappedActionBinding(Action, IE_Axis);
    }

    TArray<FName> AxisNames;
    InputSettings->GetAxisNames(AxisNames);
    for(FName Axis : AxisNames)
    {
        AddMappedAxisBinding(Axis);
    }
}

void UTackControllerInputComponent::PublishActionMappings()
{
    UInputSettings* InputSettings = UInputSettings::GetInputSettings();

    TArray<FName> ActionNames;
    InputSettings->GetActionNames(ActionNames);
    for(FName Action : ActionNames)
    {
        FTackJsonDomBuilder::FTackObject Json;
        Json.Set("Action", Action);

        TArray<FInputActionKeyMapping> MappedKeys;
        InputSettings->GetActionMappingByName(Action, MappedKeys);
        FTackJsonDomBuilder::FTackArray JsonMappingArray;
        for(FInputActionKeyMapping& ActionMapping : MappedKeys)
        {
            FTackJsonDomBuilder::FTackObject JsonMapping;
            JsonMapping.Set("Key", ActionMapping.Key.ToString())
                .Set("bShift", (bool)ActionMapping.bShift)
                .Set("bCtrl", (bool)ActionMapping.bCtrl)
                .Set("bAlt", (bool)ActionMapping.bAlt)
                .Set("bCmd", (bool)ActionMapping.bCmd);
            JsonMappingArray.Add(JsonMapping);
        }
        Json.Set("Mappings", JsonMappingArray);

        TackManager->GetPublisher()->Publish_Json(TEXT("unreal.client.input.action.mapping"), Json.AsJsonObject());
    }
}

void UTackControllerInputComponent::PublishAxisMappings()
{
    UInputSettings* InputSettings = UInputSettings::GetInputSettings();

    TArray<FName> AxisNames;
    InputSettings->GetAxisNames(AxisNames);
    for(FName Axis : AxisNames)
    {
        FTackJsonDomBuilder::FTackObject Json;
        Json.Set("Axis", Axis);

        TArray<FInputAxisKeyMapping> MappedKeys;
        InputSettings->GetAxisMappingByName(Axis, MappedKeys);
        FTackJsonDomBuilder::FTackArray JsonMappingArray;
        for(FInputAxisKeyMapping& AxisMapping : MappedKeys)
        {
            FTackJsonDomBuilder::FTackObject JsonMapping;
            JsonMapping.Set("Key", AxisMapping.Key.ToString())
                .Set("Scale", AxisMapping.Scale);

            JsonMappingArray.Add(JsonMapping);
        }
        Json.Set("Mappings", JsonMappingArray);

        TackManager->GetPublisher()->Publish_Json(TEXT("unreal.client.input.axis.mapping"), Json.AsJsonObject());
    }
}

void UTackControllerInputComponent::AddMappedAxisBinding(FName& Axis)
{
    FInputAxisBinding AxisBinding(Axis);
    AxisBinding.bConsumeInput = false;
    AxisBinding.AxisDelegate.GetDelegateForManualSet().BindLambda(
        [PreviousValue = 0.0f, AxisName = Axis.ToString(), this](float Value) mutable
        {
            if(PreviousValue != Value)
            {
                PublishMappedAxis(AxisName, Value);
                PreviousValue = Value;
            }
        }
    );
    AxisBindings.Emplace(MoveTemp(AxisBinding));
}

void UTackControllerInputComponent::AddMappedActionBinding(FName& Action, EInputEvent InputEvent)
{
    FInputActionBinding ActionBinding(Action, InputEvent);
    ActionBinding.bConsumeInput = false;

    ActionBinding.ActionDelegate.GetDelegateWithKeyForManualSet().BindLambda(
        [InputEvent, ActionName = Action.ToString(), this](FKey key)
        {
            PublishMappedAction(ActionName, InputEvent, key);
        }
    );
    this->AddActionBinding(MoveTemp(ActionBinding));
}

void UTackControllerInputComponent::PublishMappedAction(const FString& ActionName, EInputEvent InputEvent, const FKey& Key)
{
    SCOPE_CYCLE_COUNTER(STAT_Input_MappedActionPublish);

    FTackJsonDomBuilder::FTackObject Json;
    Json.Set("Action", ActionName)
        .Set("Key", Key.ToString())
        .Set("InputEvent", InputEvent)
        .Set("ServerWorldTime", GetWorld()->GetGameState()->GetServerWorldTimeSeconds());

    TackManager->GetPublisher()->Publish_Json(TEXT("unreal.client.input.action"), Json.AsJsonObject());
}

void UTackControllerInputComponent::PublishMappedAxis(const FString& AxisName, float Value)
{
    SCOPE_CYCLE_COUNTER(STAT_Input_MappedAxisPublish);

    FTackJsonDomBuilder::FTackObject Json;
    Json.Set("AxisDim", 1)
        .Set("Axis", AxisName)
        .Set("Value", FVector(Value, 0, 0))
        .Set("ServerWorldTime", GetWorld()->GetGameState()->GetServerWorldTimeSeconds());

    TackManager->GetPublisher()->Publish_Json(TEXT("unreal.client.input.axis"), Json.AsJsonObject());
}

/////////////////////////////// RAW INPUT


void UTackControllerInputComponent::EnableRawInputs()
{
    UInputSettings* InputSettings = UInputSettings::GetInputSettings();
    TArray<FKey> AllKeys;
    EKeys::GetAllKeys(AllKeys);
    for(const FKey& Key : AllKeys)
    {
        if(
            !Key.IsValid() || //Skip invalid keys
            Key == FKey(TEXT("AnyKey")) ||  // skip the "AnyKey"
            Key.GetPairedAxis() != EPairedAxis::Unpaired // skip keys that are paired
            )
            continue;

        bool bIsKeyAxis = (Key.IsAxis1D() || Key.IsAxis2D() || Key.IsAxis3D());

        if(bIsKeyAxis && Key.GetPairedAxis() == EPairedAxis::Unpaired)
        {
            AddRawAxisBinding(Key);
        }
        else if(!bIsKeyAxis)
        {
            AddRawKeyBinding(Key, IE_Pressed);
            AddRawKeyBinding(Key, IE_Released);
            AddRawKeyBinding(Key, IE_Repeat);
            AddRawKeyBinding(Key, IE_DoubleClick);
            AddRawKeyBinding(Key, IE_Axis);
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("key (%s) was not bound to anything"), *Key.ToString());
        }
    }
}

void UTackControllerInputComponent::AddRawAxisBinding(const FKey& Key)
{
    if(Key.IsAxis1D())
    {
        FInputAxisKeyBinding FloatAxisKeyBinding(Key);
        FloatAxisKeyBinding.bConsumeInput = false;
        FloatAxisKeyBinding.AxisDelegate.GetDelegateForManualSet().BindLambda(
            [PreviousValue = 0.0f, Key, this](float Value) mutable
            {
                if(PreviousValue != Value)
                {
                    PublishRawAxis(Key, FVector(Value, 0, 0));
                    PreviousValue = Value;
                }
            }
        );
        AxisKeyBindings.Emplace(MoveTemp(FloatAxisKeyBinding));
    }
    else
    {
        FInputVectorAxisBinding VectorAxisBinding(Key);
        VectorAxisBinding.bConsumeInput = false;
        VectorAxisBinding.AxisDelegate.GetDelegateForManualSet().BindLambda(
            [PreviousValue = FVector(0.0f), Key, this](FVector Value) mutable
            {
                if(!Value.Equals(PreviousValue))
                {
                    PublishRawAxis(Key, Value);
                    PreviousValue = Value;
                }
            }
        );
        VectorAxisBindings.Emplace(MoveTemp(VectorAxisBinding));
    }
}

void UTackControllerInputComponent::AddRawKeyBinding(const FKey& Key, EInputEvent InputEvent)
{
    FInputKeyBinding KB(FInputChord(Key, false, false, false, false), InputEvent);
    KB.bConsumeInput = false;
    KB.KeyDelegate.GetDelegateWithKeyForManualSet().BindLambda(
        [InputEvent, this](FKey key)
        {
            PublishRawKey(key, InputEvent);
        }
    );
    KeyBindings.Emplace(MoveTemp(KB));
}

void UTackControllerInputComponent::PublishRawKey(const FKey& Key, EInputEvent InputEvent)
{
    SCOPE_CYCLE_COUNTER(STAT_Input_RawKeyPublish);

    FTackJsonDomBuilder::FTackObject Json;
    Json.Set("Key", Key.ToString())
        .Set("InputEvent", InputEvent)
        .Set("ServerWorldTime", GetWorld()->GetGameState()->GetServerWorldTimeSeconds());

    TackManager->GetPublisher()->Publish_Json(TEXT("unreal.client.input.raw.key"), Json.AsJsonObject());
}

void UTackControllerInputComponent::PublishRawAxis(const FKey& Key, const FVector& Value)
{
    SCOPE_CYCLE_COUNTER(STAT_Input_RawAxisPublish);

    FTackJsonDomBuilder::FTackObject Json;
    Json.Set("Key", Key.ToString());
    Json.Set("Value", Value);

    if(Key.IsAxis1D())
        Json.Set("AxisDim", 1);
    else if(Key.IsAxis2D())
        Json.Set("AxisDim", 2);
    else if(Key.IsAxis3D())
        Json.Set("AxisDim", 3);
    else//This should never be reached(i think?) but just incase lets set it to nullptr
        Json.Set("AxisDim", nullptr);

    Json.Set("ServerWorldTime", GetWorld()->GetGameState()->GetServerWorldTimeSeconds());

    TackManager->GetPublisher()->Publish_Json(TEXT("unreal.client.input.raw.axis"), Json.AsJsonObject());
}

void UTackControllerInputComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    TackManager->OnTackStart.RemoveAll(this);
    TackManager->OnTackEnd.RemoveAll(this);

    Cast<APlayerController>(GetOwner())->PopInputComponent(this);
}