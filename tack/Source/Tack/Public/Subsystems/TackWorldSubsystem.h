#pragma once
#include "Subsystems/WorldSubsystem.h"
#include "TackWorldSubsystem.generated.h"

class AController;
class AActor;
class UTackIdComponent;
class ITackRecievesStateChangeInterface;


UCLASS()
class TACK_API UTackWorldSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:

    UTackWorldSubsystem();
    virtual ~UTackWorldSubsystem();

    void Initialize(FSubsystemCollectionBase& Collection) override;
    void Deinitialize() override;

    virtual bool IsTickableInEditor() const override { return true; }
    virtual void Tick(float DeltaTime) override;

    virtual TStatId GetStatId() const override;

    void OnActorSpawnedEvent(AActor* Actor);

    void TackifyWorld();

private:

    bool IsTackRunning() const;

    UTackManager* TackManager;

    bool bWorldTackified;

    bool ShouldDelayActorTackification(AActor* Actor) const;
    FDelegateHandle ActorWorldSpawnedHandle;
    TArray<TWeakObjectPtr<AActor>> DelayedTackificationActors;

    void TackifyActor(AActor* Actor);

    void DeTackifyActor(AActor* Actor);

    UTackSettings* Settings;

    UTackIdComponent* AddOrGetTackIdComponent(AActor* Actor);

    void AddAuthorityPublisherComponent(AActor* Actor, UTackIdComponent* TackIdComponent);

    void RegisterBaseComponent(AActor* Actor, UTackBaseComponent* Component, UTackIdComponent* IdComponent);
};