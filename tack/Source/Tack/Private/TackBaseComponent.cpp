#include "TackBaseComponent.h"
#include "TackManager.h"
#include "TackIdComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "Net/Core/PushModel/PushModel.h"


UTackBaseComponent::UTackBaseComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.SetTickFunctionEnable(false);

    SetIsReplicatedByDefault(true);
    bWantsInitializeComponent = true;
    bAutoActivate = true;
    bAutoRegister = false;
    bAllowReregistration = true;
    bNeverNeedsRenderUpdate = true;
    bNavigationRelevant = false;
    bCanEverAffectNavigation = false;

    bAutoCallTackStart = true;
}

const FGuid& UTackBaseComponent::GetTackId() const
{
    return TackIdComponent->GetTackId();
}

bool UTackBaseComponent::HasValidTackId() const
{
    return TackIdComponent != nullptr && TackIdComponent->HasValidTackId();
}

void UTackBaseComponent::SetTackManager(UTackManager* NewTackManager)
{
    check(!HasBegunPlay());
    TackManager = NewTackManager;
}

void UTackBaseComponent::SetTackIdComponent(UTackIdComponent* NewTackIdComponent)
{
    check(NewTackIdComponent != nullptr && !HasBegunPlay());
    TackIdComponent = NewTackIdComponent;
    MARK_PROPERTY_DIRTY_FROM_NAME(UTackBaseComponent, TackIdComponent, this);
}

void UTackBaseComponent::InitializeComponent()
{
    Super::InitializeComponent();
    if(TackManager == nullptr)
        TackManager = UTackManager::GetInstance(this);
}

void UTackBaseComponent::BeginPlay()
{
    Super::BeginPlay();
    if(bAutoCallTackStart && TackManager->IsTackRunning())
        OnTackStart();
}

void UTackBaseComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    FDoRepLifetimeParams SharedParams;
    SharedParams.bIsPushBased = true;
    DOREPLIFETIME_WITH_PARAMS_FAST(UTackBaseComponent, TackIdComponent, SharedParams);
}

bool UTackBaseComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
    bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

    if(IsValid(TackIdComponent))
    {
        WroteSomething |= Channel->ReplicateSubobject(TackIdComponent, *Bunch, *RepFlags);
    }

    return WroteSomething;
}