#pragma once
#include "Subsystems/LocalPlayerSubsystem.h"
#include "TackLocalPlayerSubsystem.generated.h"

UCLASS()
class UTackLocalPlayerSubsystem : public ULocalPlayerSubsystem
{
    GENERATED_BODY()
public:

    /** Implement this for initialization of instances of the system */
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
    void InitializeCommands();
    TUniquePtr<FAutoConsoleCommand> StartCommand;
    TUniquePtr<FAutoConsoleCommand> EndCommand;
};