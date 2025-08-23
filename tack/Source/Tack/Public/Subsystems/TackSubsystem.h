
#pragma once
#include "Tack.h"
#include "Subsystems/Subsystem.h"
#include "Interfaces/TackReceivesStateChangeInterface.h"
#include "TackSubsystem.generated.h"

class UGameInstance;
class ULocalPlayer;
class APlayerController;
class UWorld;

UCLASS(Abstract)
class TACK_API UTackSubsystem : public USubsystem, public ITackReceivesStateChangeInterface
{
    GENERATED_BODY()

public:

    //USubsystem overrides
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void OnTackStart();
    virtual void OnTackStart_Implementation() override {};

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void OnTackEnd();
    virtual void OnTackEnd_Implementation() override {};

    UTackManager* GetTackManager() const { return TackManager; }

    //ovveride UObject get world
    virtual UWorld* GetWorld() const override;
    UGameInstance* GetGameInstance() const;

private:

    class UTackManager* TackManager;
};