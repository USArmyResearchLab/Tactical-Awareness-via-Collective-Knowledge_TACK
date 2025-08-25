#include "TackStatics.h"
#include "TackSettings.h"
#include "Tack.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/ChildActorComponent.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/Volume.h"
#include "Camera/CameraComponent.h"

#include "ActorEditorUtils.h"
#include "Components/TackGameStateBaseComponent.h"
#include "Components/TackAuthorityPublisherComponent.h"
#include "TackIdComponent.h"
#include "TackManager.h"

//Ignore actors include
#include "Landscape.h"
#include "InstancedFoliageActor.h"
#include "Engine/LevelScriptActor.h"


const FGuid& UTackStatics::GetTackIdFromActor(const AActor* Actor)
{
    static const FGuid INVALID_GUID = FGuid();

    if(auto TackComponent = GetTackIdComponentFromActor(Actor))
    {
        return TackComponent->GetTackId();
    }
    else
    {
        return INVALID_GUID;
    }
}

bool UTackStatics::IsActorValidForTackComponent(const AActor* Actor)
{

    if(Actor == nullptr)
        return false;

    UWorld* World = Actor->GetWorld();
    //Contant defulats that are never valid
    if(
        IsValid(Actor) == false || // don't add pending kill
        Actor->IsTemplate() ||  // don't add templates
        Actor->IsEditorOnly() || // don't add ediotr only actors
        // FActorEditorUtils::IsABuilderBrush(Actor) || // don't add brush actors
        // Actor->IsA(AWorldSettings::StaticClass()) || //|| // don't add WorldSettings
         //Actor->IsA(ALandscape::StaticClass()) || // don't tackify landscapes
       //  Actor->IsA(AInstancedFoliageActor::StaticClass()) ||
         //Actor->IsA(ALevelScriptActor::StaticClass()) || // don't tackify script actors
        Actor->IsA(APlayerState::StaticClass()) || //Handled in TackManager
        Actor->IsA(APlayerController::StaticClass()) || //Handled in TackManager
        //Actor->IsA(AController::StaticClass()) || // Don't Tackify Controllers.
        (!World->IsGameWorld() && World->WorldType != EWorldType::Editor) // only allow GameWorlds and EditorWorlds
#if WITH_EDITOR // Specific things to check for when we are in Editor
        //|| Actor->IsHiddenEd()
        // || (World->WorldType == EWorldType::Editor && (
        //     Actor->HasAnyFlags(RF_Transient) // || !Actor->IsEditable() || !Actor->IsListedInSceneOutliner()
        // ))
#endif
        )
    {
        return false;
    }

    //Passes all above conditions return true;
    return true;
}

UTackGameStateBaseComponent* UTackStatics::GetTackGameStateBaseComponent(const UObject* WorldContextObject)
{
    if(UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
    {
        return World->GetGameState()->FindComponentByClass<UTackGameStateBaseComponent>();
    }
    return nullptr;
}