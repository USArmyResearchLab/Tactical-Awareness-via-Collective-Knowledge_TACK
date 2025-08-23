#include "Subsystems/TackLocalPlayerSubsystem.h"

//UE includes
#include "HAL/ConsoleManager.h"

//Tack Includes
#include "Components/TackControllerComponent.h"
#include "TackManager.h"

void UTackLocalPlayerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    InitializeCommands();
}

void UTackLocalPlayerSubsystem::InitializeCommands()
{

    if(!IConsoleManager::Get().IsNameRegistered(TEXT("Tack.Start")))
    {
        //Initialize Start Command
        StartCommand = MakeUnique<FAutoConsoleCommand>
            (
                TEXT("Tack.Start"),
                TEXT("Starts Tack"),
                FConsoleCommandDelegate::CreateLambda([this]()
                    {
                        FLocalPlayerContext LocalPlayerContext(GetLocalPlayer());
                        FString LogPrefix = UTackManager::GetInstance(LocalPlayerContext.GetWorld())->GetTackLogPrefix(LocalPlayerContext.GetWorld());
                        UE_LOG(LogTack, Log, TEXT("%s - Tack.Start Console Command Entered"), *LogPrefix);

                        if(!LocalPlayerContext.IsValid())
                        {
                            UE_LOG(LogTack, Error, TEXT("%s - Could not START Tack - LocalPlayerContext was invalid"), *LogPrefix);
                            return;
                        }

                        if(auto PlayerController = LocalPlayerContext.GetPlayerController())
                        {
                            if(auto TackControllerComponent = PlayerController->FindComponentByClass<UTackControllerComponent>())
                            {
                                TackControllerComponent->ServerStartTack();
                            }
                            else
                            {
                                UE_LOG(LogTack, Error, TEXT("%s - Could not START Tack. Failed to find UTackPlayerStateComponent on PlayerState"), *LogPrefix);
                            }
                        }
                        else
                        {
                            UE_LOG(LogTack, Error, TEXT("%s - Could not START Tack. Failed to LocalPlayer PlayerState"), *LogPrefix);
                        }
                    })
            );
    }

    if(!IConsoleManager::Get().IsNameRegistered(TEXT("Tack.End")))
    {
        //Initialize End Command
        EndCommand = MakeUnique<FAutoConsoleCommand>
            (
                TEXT("Tack.End"),
                TEXT("Stops Tack"),
                FConsoleCommandDelegate::CreateLambda([this]()
                    {
                        FLocalPlayerContext LocalPlayerContext(GetLocalPlayer());
                        FString LogPrefix = UTackManager::GetInstance(LocalPlayerContext.GetWorld())->GetTackLogPrefix(LocalPlayerContext.GetWorld());
                        UE_LOG(LogTack, Log, TEXT("%s - Tack.Start Console Command Entered"), *LogPrefix);

                        if(!LocalPlayerContext.IsValid())
                        {
                            UE_LOG(LogTack, Error, TEXT("%s - Could not STOP Tack - LocalPlayerContext was invalid"), *LogPrefix);
                            return;
                        }

                        if(auto PlayerController = LocalPlayerContext.GetPlayerController())
                        {
                            if(auto TackControllerComponent = PlayerController->FindComponentByClass<UTackControllerComponent>())
                            {
                                TackControllerComponent->ServerStopTack();
                            }
                            else
                            {
                                UE_LOG(LogTack, Error, TEXT("%s - Could not STOP Tack. Failed to find UTackPlayerStateComponent on PlayerState"), *LogPrefix);
                            }
                        }
                        else
                        {
                            UE_LOG(LogTack, Error, TEXT("%s - Could not STOP Tack. Failed to LocalPlayer PlayerState"), *LogPrefix);
                        }
                    })
            );
    }
}