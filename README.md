# ARL-Tack

**Tactical Awareness via Collective Knowledge (TACK)** is a collection of plugins for the Unreal Engine that record both game/world and operator/player state for both real-time
and post hoc analysis/exploitation. These plugins publish player/game state via kafka. External software can then listen to the kafka data streams.


## Installation
### Unreal Engine
Unreal Engine install instructions: https://dev.epicgames.com/documentation/en-us/unreal-engine/install-unreal-engine

Unreal Engien setup for visual studio: https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-visual-studio-development-environment-for-cplusplus-projects-in-unreal-engine

c++ project: https://dev.epicgames.com/documentation/en-us/unreal-engine/set-up-and-compile-a-cplusplus-project-in-unreal-engine

### Plugin
To install any realated Tack Unreal plugins drop them into the Plugins folder of an Unreal project. 

If using git ensure that git lfs is installed. Run ```git lfs install``` before cloning if not installed. Otherwise the binaries will be pulled incorrectly.


## ARL-Tack Usage
On initial level load in the editor tack will tackify the world and tack components will be added to the level actors. Any additional actors spawned at runtime will receive their tack components when spawned.

To start tack use the console command ```Tack.Start``` and ```Tack.End``` to stop it. 
You can programmatically start/stop tack using ```UTackStatics::StartTack``` and ```UTackStatics::StopTack``` functions. These are callable from c++.
Blueprints can call the Start Tack and End Tack nodes from the Tack Manager GameInstance Subsystem.

![screenshot of the tack start node](images/start_tack.jpg)
![screenshot of the tack stop node](images/stop_tack.jpg)

The general flow of an experiment will go like this.

- First start ARL-tack-wsl-back
- Start Tack
- Stop Tack
- Repeat Start and Stop as much as needed
- Stop tack-wsl-back once all tack sessions are stopped

To see more information about the tack-wsl-back please see its README.

There are many default topics that get published.
- tack.session
- unreal.actor
- unreal.actor.lifetime
- unreal.actor.component.overlap
- unreal.actor.component.transform
- unreal.world
- unreal.client.cameraposition
- unreal.client
- unreal.client.time
- unreal.client.input.raw.key
- unreal.client.input.raw.axis
- unreal.controller
- unreal.input.key.definition
- unreal.pawn.controller_changed

Additional topics can be made when using the publish functions


### Custom struct publishing
ARL-Tack provides the ability to publish any blueprint or c++ defined struct
```UTackStaticPublishers::BP_Publish_Struct```, (available in c++) or ```FTackModule::Get().GetBackendFeature().Publish_Struct(TopicName, StructProperty->Struct, StructPtr)``` (available in c++) functions. Blueprints can call Publish Struct from the Tack Publisher GameInstance Subsystem. If Tack has not been started these functions will not publish anything.

![screenshot of publish struct node](images/publish_struct.jpg)

### ARL-Tack editor eyetracker debug commands
For all of these commands 0 to disable 1 to enable
- Tack.EyeTracker.Debug - Toggle to enable eye tracking debug that displays the main, right, left gaze points on the screen. (Disabled by default)

- Tack.EyeTracker.DebugMain - Toggle to display the combined gaze when debug mode is on. (Enabled by default)

- Tack.EyeTracker.DebugRightEye - Toggle to display the right eye when debug mode is on. (Enabled by default)

- Tack.EyeTracker.DebugLeftEye - Toggle to display the left eye when debug mode is on. (Enabled by default)

### Saving additional data with Additional Client Settings
Additional data can be saved per client with the additional client settings. It can be retreived with blueprints from the Tack Manager Subsystem. It is a map of strings and it can be added to before tack starts and it will be saved when tack is started. It 

![addational client info blueprint](images/a_client_settings.jpg)

The Additional Client Settings can also be accessed through c++ with UTackManager ```UTackManager::GetInstance(const UObject* WorldContextObject)``` or the other ```GetTackManager()``` functions in ```UTackSubsystem```, ```UTackBaseComponent```, or ```UTackControllerComponent```


### ARL-Tack editor stat group
- A stat group exists for component trandform, actor damage, component hit, component overlap, client camera, input, client time, and eye tracker trace publishing. Also includes Tack world subsystem tick and actor tackification cycle stats.
- UI for it does not exit but the command ```Stat Tack``` exists


## ARL-Tack Settings
All ARL-Tack Settings and their descriptions are found in the project settings window in the editor ```edit->project settings```. 

### General
- __Experiment Name__ - Experiment name that will be saved in the database.
- __Additional Client Info__ - Useful to store data across lifetime of game instance

### Actor, AI, Input, Camera transform Publisher Settings
These sections of boolean values control what gets saved and by default they are all true. Camera Transform Publisher has additional options that adjusts the frequency and tolerance of when publishing happens.

### Kafka Settings
- __Local Connection String__ - This is the connection string ARL-Tack uses to connect to kafka. ex ```127.0.0.1:9092``` ```localhost:9092``` to connect to a locally hosted kafka server.
- __Client Connection String__ - This connection string is for the clients to use if it needds to be different from the server.
- __Force Client To Use Local Connection String__ - Boolean to override the default behavior of the client using the client connection string.

### Eye Tracker Settings
- __Enable Eyetracker Publisher__ - Allows the default tack eye tracker or any supported eye tracker to publish.
- __Maximum Trace Distance__ - Maximum visible trace distance that tack will compute when eye tracker publishing is enabled

### Snapshot Settings
- __Enable Snapshot Publish__ - This allows snapshots to be taken and published over kafka.
-  __Compression Quality__ - Controls the compression quality of the JPEG snapshot. 0 is the default value and equal to 80, higher values are higher compression.
- __CaptureFrameRate__ - Frame rate of the desired snapshots to be made. Max value of 24.
- __Crop Desired Size__ - Control if the desired size should be overridden. This will center crop the snapshot if it is smaller than the screen resolution This is needed for vr snapshots.
- __DesiredSize__ - The size of the desired snapshot output.

## Packaging notes
When packaging a project Settings are NOT copied over unless they are set as default and appear as ```DefaultTack.ini``` in the Config folder in the project. Otherwise the settings must be manually copied the settings folder as ```Tack.ini```. Changing the packaged defaults can be done the same way.
The location is ```\WindowsNoEditor\PACKAGED_NAME\Saved\Config\WindowsNoEditor\Tack.ini```. The easiest way to do this is to run your projects executable once and it will generate the location. Then replace the generated Tack.ini.
The same applies to shipping builds although the saved location is different for those.

## Kafka connection troubleshooting


## ARL-Tack Interfaces

### TackComponentInterface
Interface that gives actor components the tack id to keep track of them internally

### TackReceivesStateChangeInterface
Interface that gives actor components the OnTackStart and OnTackEnd functions that are called when tack starts/ends


## Tack Components

### TackIdComponent
Parent classs for the TackGenuineIdComponent.

### TackGenuineIdComponent
This component gets auto created and attached to every actor in the level. This also occurs for any actor spawned as well.

### TackBaseComponent
Base component for TackAuthorityPublisherComponent, TackClientCameraPublisherComponent, TackEyeTrackerComponent and handles accepting the tack start/stop events and getting the tack manager.

### TackControllerComponent
Component that gets spawned and attached to player controllers and handles the tack start/stop coming from the tack manager.

### TackGameModeBaseComponent
Base class for the TackGameModeComponent that adds match state publishing and adds handlers for login and logout events

### TackGameStateBaseComponent
Class that that adds match state, and client time publishing. Handles replicating the ClientKafkaConnectionString to connect to the correct kafka broker.

### TackPlayerStateComponent
Saves and replicates the same guid from TackControllerComponent.

### TackTagsComponent
Custom tags component that is replicated.


## ARL-Tack subsystems

### **TackManager**
Provides blueprints and cpp code access to the start/stop functions at runtime. Also provides access to additional client info for storing useful information there

### **TackPublisher**
Provides blueprints and cpp code access to the publisher and the ability to publish struct and data to kafka topics.

### TackWorldSubsystem
This subsystem handles the tackification (addition of tack components). Spawns and attaches UTackControllerComponent or UTackGenuineIdComponent, which are child classes of UTackIdComponent when actors are spawned. The UTackAuthorityPublisherComponent is also spawned. This handles the tackification of the world as well.

### TackSubsystem
This subsystem is the parent class for the UTackXRSubsystem and UTackSnapshotSubsystem. Overrides GetWorld function, adds GetGameInstance, and adds pointer to UTackManager

### TackEyeTrackerSubsystem
Base subsystem for other eye tracker subsystems and is child of ULocalPlayerSubsystem. 

### TackEyeTrackerDefaultSubsystem
Child Subsystem of the TackEyeTrackerSubsystem that gets created if bForceDefaultEyetrackerPublisher is true in UTackSettings. Function IsEyeTrackerConnected always returns true and GetTackEyeTrackerComponentClass returns UTackDefaultEyeTrackerComponent.

### TackLocalPlayerSubsystem
Subsystem that registers the Tack.Start and Tack.End console commands


## Things to Know

- ARL-Tack will work in single or multiplayer. This includes both dediacted and listen servers.
- Packaging a shipping build will compile out the console. Meaning you cannot use the console commands to start/stop ARL-Tack.
- ARL-Tack does no support traveling between worlds/maps. So an experiment should be conatined within one map. Or ARL-Tack must be stopped before an ```open level``` / ```server travel``` command.
#### tobii pro eyetracking 
- We used their 4.25 or pre 4.25 plugin from their sdk that we used up to 4.27 without any modifications other than changing the ```EngineVersion``` in the uplugin. Not sure where to get it with Epic's change to FAB for the engin plugins.


## Common error message troubleshooting
When tack starts it will log on screen and to file the session and the connection string to kafka:
```LogTack: Listen Server - Instance (997C44E94832B09D19939489C64B4A3E) Session (5F43C3674F4AEEA5C1936A9F5299733D) Connection (127.0.0.1:9092) - Tack Started ```

![tack start on screen log](images/tack_start_screen.jpg)

After that if the connection does not connect to kafka there will be warning errors logs on screen and to file:
```Warning: rdkafka#producer-3 - FAIL (3) - [thrd:127.0.0.1:9092/bootstrap]: 127.0.0.1:9092/bootstrap: Connect to ipv4#127.0.0.1:9092 failed: Unknown error (after 21034ms in state CONNECT)```

![kafka connection error warning](images/tack_connection_warning.jpg)

When tack ends it will log that it has stopped. If the connection error has occured it will log errors on screen and to file:
```LogTack: Listen Server - Instance (997C44E94832B09D19939489C64B4A3E) Session (5F43C3674F4AEEA5C1936A9F5299733D) - Tack Stopping```

```
LogTack: Error: rdkafka#producer-3 - ERROR(-195) - 172.31.24.21:9092/bootstrap: Connect to ipv4#172.31.24.21:9092 failed: Unknown error (after 21034ms in state CONNECT)
LogTack: Error: rdkafka#producer-3 - ERROR(-187) - 1/1 brokers are down
```

![tack stop with errors](images/tack_end_error.jpg)

### Check the connection string
Check the connection strings in the tack settings under kafka settings and confirm that is correct. The string there should match the one that was logged. For packaged builds confirm that the connection string in the Tack.ini file is correct. That file can be created by exporting it from the unreal Engine project settings.

### Check for other connection problems
A quick way to check is to ping the machine that is running kafka. This applies when in local multiplayer or ARL-tack-wsl-back is running on a separate pc.


# Known bugs
- When using seamless server travel and the player controller class changes the tack controller component will be lost and that client will not start tack.
  - Workaround is to keep the player controller class the same across seamless server travels

- ```Stat Tack``` command displays no stats from the stat group