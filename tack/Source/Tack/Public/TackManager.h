#pragma once
#include "Subsystems/Subsystem.h"
#include "Subsystems/TackSubsystem.h"
#include "Subsystems/SubsystemCollection.h"
#include "TackManager.generated.h"

class AGameModeBase;
class AController;
class APawn;
class UTackPublisher;

DECLARE_MULTICAST_DELEGATE(FTackStateChangeEvent);

UCLASS(BlueprintType)
class TACK_API UTackManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:

    static UTackManager* GetInstance(const UObject* WorldContextObject);

    UTackManager();

    /** Implement this for initialization of instances of the system */
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable)
    UTackPublisher* GetPublisher() const { return Publisher; }

    UFUNCTION()
    FString GetTackLogPrefix(UWorld* World, bool bIncludeConnectionString = false) const;

    UFUNCTION(BlueprintCallable)
    bool IsTackRunning() const { return bIsTackRunning; };

    UFUNCTION(BlueprintCallable)
    const FGuid& GetLocalInstanceId() const { return LocalInstanceId; }

    const FGuid& GetCurrentSessionId() const;

    UFUNCTION(BlueprintCallable)
    void StartTack();

    UFUNCTION(BlueprintCallable)
    void StopTack();

    //These two functions forward to Interal_StartTack and Interal_StopTack
    //Generally you should always call StartTack or StopTack and not call these two
    void StartTackLocally(const FGuid& SessionId);
    void StopTackLocally();

private:
    void Internal_StartTack(const FGuid& SessionId);
    void Internal_StopTack();

public: // Tack Events

    FTackStateChangeEvent OnTackStart;
    FTackStateChangeEvent OnTackEnd;

    //client constants
    UPROPERTY(BlueprintReadOnly)
    TMap<FString, FString> AdditionalClientInfo;

private:

    FString GetKafkaConnectionString() const;

    UPROPERTY()
    FGuid CurrentSessionId;

    UPROPERTY() bool bIsTackRunning;
    UPROPERTY() FGuid LocalInstanceId;
    UPROPERTY() UTackPublisher* Publisher;

    void OnGameModeInitialized(AGameModeBase* GameModeBase);
    void OnGameModePostLoginEvent(AGameModeBase* GameMode, APlayerController* NewPlayer);

    UFUNCTION()
    void OnWorldBeginTearingDown(UWorld* World);

    UFUNCTION()
    void OnTravelFailure(UWorld* InWorld, ETravelFailure::Type FailureType, const FString& ErrorString);

    UFUNCTION()
    void OnNotifyPreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel);

    UFUNCTION()
    void OnPawnControllerChanged(APawn* Pawn, AController* Controller);

    FSubsystemCollection<UTackSubsystem> SubsystemCollection;
};