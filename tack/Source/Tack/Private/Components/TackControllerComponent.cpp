#include "Components/TackControllerComponent.h"

//UE4 Includes
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "AIController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

//Tack includes
#include "Components/TackControllerInputComponent.h"
#include "Components/TackPlayerStateComponent.h"
#include "TackManager.h"
#include "TackSettings.h"
#include "Subsystems/TackEyeTrackerSubsystem.h"

UTackControllerComponent::UTackControllerComponent() : UTackIdComponent()
{
    bStartTackOnBeginPlay = false;
    bLocallyInitialized = false;
}

void UTackControllerComponent::PostNetReceive()
{
    Super::PostNetReceive();
    //This also should always be true I belive
    check(OwningController->IsLocalPlayerController() && !GetOwner()->HasAuthority());
    //Only Client will call PostNetInit
    //Client Propertyies have a chance of not being fully replicated in BeginPlay So we call 
    //Wait until properties are fully initialized
    if(Guid.IsValid() && (TackPlayerStateComponent != nullptr || (!OwningController->IsPlayerController() && !Cast<AAIController>(OwningController)->bWantsPlayerState)))
        TackControllerInit();
}

void UTackControllerComponent::OnComponentCreated()
{
    Super::OnComponentCreated();
    if(!IsTemplate())
    {
        OwningController = CastChecked<AController>(GetOwner());
        TackManager = UTackManager::GetInstance(this);

        //I belive these should always be true but not 100% sure
        //This is more here for validation for me then anything
        check(!Guid.IsValid());
        check(TackPlayerStateComponent == nullptr);

        if(GetOwner()->HasAuthority())
        {
            //Turn off replication for AIControllers
            //Not sure if this helps at all but no harm in calling it
            SetIsReplicated(OwningController->IsPlayerController());

            //Initialize Guid and MarkDirty for replication
            Guid = FGuid::NewGuid();
            MARK_PROPERTY_DIRTY_FROM_NAME(UTackControllerComponent, Guid, this);

            if(APlayerState* ControllerPlayerState = OwningController->PlayerState)
            {
                //TackPlayerStateComponent Should not exist yet
                check(ControllerPlayerState->FindComponentByClass<UTackPlayerStateComponent>() == nullptr);

                //Create TackPlayerStateComponent and set its Guid
                TackPlayerStateComponent = NewObject<UTackPlayerStateComponent>(ControllerPlayerState, NAME_None);
                TackPlayerStateComponent->SetTackId(Guid);
                TackPlayerStateComponent->RegisterComponent();

                //Mark PlayerStateComponent Dirty for replication
                MARK_PROPERTY_DIRTY_FROM_NAME(UTackControllerComponent, TackPlayerStateComponent, this);
            }
            else
            {
                //PlayerController PlayerStates should be initialized at this point.
                //AIControllers only need one if bWantsPlayerState is true
                check(!OwningController->IsPlayerController() && !Cast<AAIController>(OwningController)->bWantsPlayerState);
            }
        }
    }
}

void UTackControllerComponent::BeginPlay()
{
    Super::BeginPlay();
    //Authority Owner will never call PostNetInit so We need to call TackControllerInit Here
    if(GetOwner()->HasAuthority() && OwningController->IsLocalController())
        TackControllerInit();
}

void UTackControllerComponent::TackControllerInit()
{
    //Check here to make sure I don't call this anywhere but on LocalControllers
    check(OwningController->IsLocalController());
    check(Guid.IsValid());

    AddExtraLocalControllerComponents();

    if(bStartTackOnBeginPlay)
    {
        if(OwningController->IsLocalPlayerController())
            RequestServerToStartLocalClientTack();
        else//Publish AI Controller
            GetTackManager()->GetPublisher()->PublishController(OwningController);
    }
}

void UTackControllerComponent::RequestServerToStartLocalClientTack()
{
    //This Function Should only be called from PlayerControllers
    check(OwningController->IsPlayerController());

    if(
        this->HasBegunPlay() &&

        //I Believe we need to check these ass TackPlayerStateComponent Property is not Garunteed to replicate immediately
        TackPlayerStateComponent != nullptr &&
        TackPlayerStateComponent->HasBegunPlay() &&
        TackPlayerStateComponent->HasValidTackId()
        )
    {
        ServerStartLocalTack();
    }
    else
    {
        UE_LOG(LogTack, Warning, TEXT("%s - Delaying tack start until TackControllerComponent LocalPlayer and PlayerState are valid"), *GetTackManager()->GetTackLogPrefix(GetWorld()));
        GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]() {
            RequestServerToStartLocalClientTack();
            }));
    }
}

void UTackControllerComponent::ShouldStartTackOnBeginPlay(bool bNewStartTackOnBeginPlay)
{
    check(!HasBegunPlay());
    bStartTackOnBeginPlay = bNewStartTackOnBeginPlay;
    MARK_PROPERTY_DIRTY_FROM_NAME(UTackControllerComponent, bStartTackOnBeginPlay, this);
}

void UTackControllerComponent::ServerStartTack_Implementation()
{
    //This Function Should only be called from PlayerControllers
    check(OwningController->IsPlayerController());

    if(!TackManager->IsTackRunning())
        TackManager->StartTack();
}

void UTackControllerComponent::ServerStopTack_Implementation()
{
    //This Function Should only be called from PlayerControllers
    check(OwningController->IsPlayerController());

    if(TackManager->IsTackRunning())
        TackManager->StopTack();
}

void UTackControllerComponent::ClientStartLocalTack_Implementation(const FGuid& SessionId)
{
    //This Function Should only be called from PlayerControllers
    check(OwningController->IsPlayerController());

    TackManager->StartTackLocally(SessionId);
}

void UTackControllerComponent::ClientStopLocalTack_Implementation()
{
    //This Function Should only be called from PlayerControllers
    check(OwningController->IsPlayerController());

    TackManager->StopTackLocally();
}

void UTackControllerComponent::ServerStartLocalTack_Implementation()
{
    //This Function Should only be called from PlayerControllers
    check(OwningController->IsPlayerController());
    //Tack should be running when the client requests it on begin play
    check(TackManager->IsTackRunning());

    ClientStartLocalTack(TackManager->GetCurrentSessionId());
}

void UTackControllerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    FDoRepLifetimeParams SharedParams;
    SharedParams.Condition = COND_InitialOrOwner;
    SharedParams.bIsPushBased = true;
    DOREPLIFETIME_WITH_PARAMS_FAST(UTackControllerComponent, Guid, SharedParams);
    DOREPLIFETIME_WITH_PARAMS_FAST(UTackControllerComponent, bStartTackOnBeginPlay, SharedParams);
    DOREPLIFETIME_WITH_PARAMS_FAST(UTackControllerComponent, TackPlayerStateComponent, SharedParams);
}

void UTackControllerComponent::AddExtraLocalControllerComponents()
{
    const UTackSettings* Settings = GetDefault<UTackSettings>();

    //Only Add Below components to LocalPlayerControllers
    if(OwningController->IsLocalPlayerController())
    {
        //Spawn UTackClientCameraPublisherComponent
        {
            //Component Should not exist yet
            check(OwningController->FindComponentByClass<UTackClientCameraPublisherComponent>() == nullptr);
            if(Settings->bEnableCameraTransformPublisher)
            {
                UTackClientCameraPublisherComponent* const Component = NewObject<UTackClientCameraPublisherComponent>(OwningController, NAME_None, RF_Transient);
                check(Component != nullptr); // Failed to Add/SpawnComponent to LocalPlayerController
                Component->SetTackManager(GetTackManager());
                Component->SetTackIdComponent(this);
                Component->RegisterComponent();
            }
        }
        //Spawn UTackControllerInputComponent
        {
            //Component Should not exist yet
            check(OwningController->FindComponentByClass<UTackControllerInputComponent>() == nullptr);
            if(Settings->PublishesAnyInput())
            {
                UTackControllerInputComponent* const Component = NewObject< UTackControllerInputComponent >(OwningController, NAME_None, RF_Transient);
                check(Component != nullptr) // Failed to Add/SpawnComponent to LocalPlayerController
                    Component->SetTackManager(GetTackManager());
                Component->RegisterComponent();
            }
        }
        //Spawn UTackEyeTrackerComponent
        {
            //Component Should not exist yet
            check(OwningController->FindComponentByClass<UTackEyeTrackerComponent>() == nullptr)
                if(Settings->bEnableEyetrackerPublisher)
                {
                    AttemptToAddEyeTrackerComponentToLocalPlayer();
                }
        }
    }
}

void UTackControllerComponent::AttemptToAddEyeTrackerComponentToLocalPlayer()
{
    APlayerController* PlayerController = Cast<APlayerController>(OwningController);
    if(auto EyeTrackerSubsystem = UTackEyeTrackerSubsystem::GetFirstConnectedEyeTrackerSubsystem(this))
    {
        UE_LOG(LogTack, Log, TEXT("Adding TackEyeTracker Class %s"), *EyeTrackerSubsystem->GetTackEyeTrackerComponentClass()->GetName());
        UTackEyeTrackerComponent* Component = NewObject<UTackEyeTrackerComponent>(OwningController, EyeTrackerSubsystem->GetTackEyeTrackerComponentClass(), NAME_None, RF_Transient);
        check(Component != nullptr);
        Component->SetTackManager(GetTackManager());
        Component->SetTackIdComponent(this);
        Component->RegisterComponent();
    }
    else
    {
        GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]() {
            AttemptToAddEyeTrackerComponentToLocalPlayer();
            }));
    }
}