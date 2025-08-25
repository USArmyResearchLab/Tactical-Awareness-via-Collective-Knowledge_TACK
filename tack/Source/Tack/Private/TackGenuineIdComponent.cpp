#include "TackGenuineIdComponent.h"
#include "TackManager.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

UTackGenuineIdComponent::UTackGenuineIdComponent() : UTackIdComponent()
{

}

void UTackGenuineIdComponent::OnComponentCreated()
{
    Super::OnComponentCreated();

    if(IsTemplate())
    {
        //Invalidate Template Guid
        Guid.Invalidate();
    }
    else
    {
        //If We are a NetStartupComponent in a GameWorld are Guid should be valid at this point
        //This is true on the client and server as NetStartupComponents are loaded from the map
        check(!(GetWorld()->IsGameWorld() && IsNetStartupComponent() && !Guid.IsValid()));

        if(GetOwner()->HasAuthority() && !Guid.IsValid())
        {
            //Initialize Guid On Authority Only.
            Guid = FGuid::NewGuid();
            //Mark for Replication only when initializing Guid
            //Actors Saved in the map shouldn't need to replicate the Guid as its already set on map save
            //This should be the only place we ever have to mark the Guid for replication
            MARK_PROPERTY_DIRTY_FROM_NAME(UTackGenuineIdComponent, Guid, this);
        }
    }
}

void UTackGenuineIdComponent::PostNetReceive()
{
    Super::PostNetReceive();
    //Guids On Client Should Always be valid at this point
    //Validate here and not on begin play as BeginPlay can get called before Propertys are replicated
    check(Guid.IsValid());
}



void UTackGenuineIdComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    FDoRepLifetimeParams LifetimeParams;
    LifetimeParams.bIsPushBased = true;
    DOREPLIFETIME_WITH_PARAMS_FAST(UTackGenuineIdComponent, Guid, LifetimeParams);
}

TStructOnScope<FActorComponentInstanceData> UTackGenuineIdComponent::GetComponentInstanceData() const
{
    return MakeStructOnScope<FActorComponentInstanceData, FTackGenuineIdComponentInstanceData>(this);
}

//TACK COMPONENT INSTANCE DATA STRUCT

FTackGenuineIdComponentInstanceData::FTackGenuineIdComponentInstanceData(const UTackGenuineIdComponent* SourceComponent) : FActorComponentInstanceData(SourceComponent)
{
    Guid = SourceComponent->Guid;
}

bool FTackGenuineIdComponentInstanceData::ContainsData() const
{
    return Guid.IsValid() || Super::ContainsData();
}

void FTackGenuineIdComponentInstanceData::ApplyToComponent(UActorComponent* Component, const ECacheApplyPhase CacheApplyPhase)
{
    Super::ApplyToComponent(Component, CacheApplyPhase);

    UTackGenuineIdComponent* TackIdComponent = CastChecked<UTackGenuineIdComponent>(Component);
    TackIdComponent->Guid = Guid;
}