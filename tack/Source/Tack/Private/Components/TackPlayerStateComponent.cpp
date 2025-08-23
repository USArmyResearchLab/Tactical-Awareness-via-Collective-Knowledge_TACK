#include "Components/TackPlayerStateComponent.h"
#include "Components/TackControllerComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"
#include "Tack.h"
#include "TackManager.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

UTackPlayerStateComponent::UTackPlayerStateComponent() : UTackIdComponent()
{

}

void UTackPlayerStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    FDoRepLifetimeParams SharedParams;
    SharedParams.bIsPushBased = true;
    DOREPLIFETIME_WITH_PARAMS_FAST(UTackPlayerStateComponent, Guid, SharedParams);
}

void UTackPlayerStateComponent::SetTackId(const FGuid& PlayerControllerTackGuid)
{
    check(GetOwner()->HasAuthority());
    check(!HasBegunPlay());
    check(!Guid.IsValid());

    //Set Guid on Server Only
    Guid = PlayerControllerTackGuid;
    //This should be the only place we ever have to mark the Guid for replication
    MARK_PROPERTY_DIRTY_FROM_NAME(UTackPlayerStateComponent, Guid, this);
}

void UTackPlayerStateComponent::PostNetReceive()
{
    Super::PostNetReceive();
    //Guids On Client Should Always be valid at this point
    //Validate here and not on begin play as BeginPlay can get called before Propertys are replicated
    check(Guid.IsValid());
}

void UTackPlayerStateComponent::BeginPlay()
{
    Super::BeginPlay();
    //Server Should always have a valid Guid at this point
    check(!(GetOwner()->HasAuthority() && !Guid.IsValid()));
}