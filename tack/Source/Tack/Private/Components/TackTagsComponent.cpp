#include "Components/TackTagsComponent.h"
#include "Net/UnrealNetwork.h"


UTackTagsComponent::UTackTagsComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
    bAutoActivate = true;
    bAutoRegister = true;
}

void UTackTagsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UTackTagsComponent, Tags);
}