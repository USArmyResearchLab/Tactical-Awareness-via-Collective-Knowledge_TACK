#include "TackIdComponent.h"

UTackIdComponent::UTackIdComponent() : UActorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.SetTickFunctionEnable(false);

    SetIsReplicatedByDefault(true);
    bAutoActivate = true;
    bAutoRegister = true;
    bAllowReregistration = true;
    bNeverNeedsRenderUpdate = true;
    bNavigationRelevant = false;
    bCanEverAffectNavigation = false;
}