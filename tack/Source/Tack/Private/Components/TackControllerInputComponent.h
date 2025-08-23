#pragma once
#include "Components/InputComponent.h"
#include "Interfaces/TackComponentInterface.h"
#include "Interfaces/TackReceivesStateChangeInterface.h"
#include "TackBaseComponent.h"
#include "TackControllerInputComponent.generated.h"

class UTackIdComponent;

UCLASS()
class UTackControllerInputComponent : public UInputComponent, public ITackComponentInterface
{
    GENERATED_BODY()
public:
    UTackControllerInputComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    virtual void OnTackStart();
    virtual void OnTackEnd();

    virtual const FGuid& GetTackId() const override;
    virtual bool HasValidTackId() const override;

    void SetTackManager(UTackManager* NewTackManager);

private:
    class UTackManager* TackManager;
    FString CachedClientTackId;

    friend class UTackWorldSubsystem;
    UPROPERTY(VisibleInstanceOnly, SaveGame)
    UTackIdComponent* TackIdComponent;

    //Mappped Inputs
    void EnableMappedInputs();
    void PublishActionMappings();
    void PublishAxisMappings();

    void AddMappedActionBinding(FName& ActionName, EInputEvent InputEvent);
    void PublishMappedAction(const FString& ActionName, EInputEvent InputEvent, const FKey& Key);

    void AddMappedAxisBinding(FName& ActionName);
    void PublishMappedAxis(const FString& ActionName, float Value);

    //Raw Inputs
    void EnableRawInputs();

    void AddRawKeyBinding(const FKey& Key, EInputEvent InputEvent);
    void PublishRawKey(const FKey& Key, EInputEvent InputEvent);

    void AddRawAxisBinding(const FKey& Key);
    void PublishRawAxis(const FKey& Key, const FVector& Value);
};