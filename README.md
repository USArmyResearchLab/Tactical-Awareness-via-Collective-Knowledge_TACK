# Tack

## Tack Usage
On initial level load in the editor tack will tackify the world and tack components will be added to the level actors. Any additional actors spawned at runtime will receive their tack components when spawned.

To start tack use the console command ```Tack.Start``` and ```Tack.End``` to stop it. 
You can programmatically start/stop tack using ```UTackStatics::StartTack``` and ```UTackStatics::StopTack``` functions. These are callable from c++.
Blueprints can call the Start Tack and End Tack nodes from the Tack Manager GameInstance Subsystem.(maybe have an image here of the blueprint)

### Custom struct publishing
Tack provides the ability to publish any blueprint or c++ defined struct
```UTackStaticPublishers::BP_Publish_Struct```, (available in c++) or ```FTackModule::Get().GetBackendFeature().Publish_Struct(TopicName, StructProperty->Struct, StructPtr)``` (available in c++) functions. Blueprints can call Publish Struct from the Tack Publisher GameInstance Subsystem. If Tack has not been started these functions will not publish anything.

### Tack editor eyetracker debug commands
For all of these commands 0 to disable 1 to enable
- Tack.EyeTracker.Debug - Toggle to enable eye tracking debug that displays the main, right, left gaze points on the screen. (Disabled by default)

- Tack.EyeTracker.DebugMain - Toggle to display the combined gaze when debug mode is on. (Enabled by default)

- Tack.EyeTracker.DebugRightEye - Toggle to display the right eye when debug mode is on. (Enabled by default)

- Tack.EyeTracker.DebugLeftEye - Toggle to display the left eye when debug mode is on. (Enabled by default)

### Tack editor stat group
- A stat group exists for component trandform, actor damage, component hit, component overlap, client camera, input, client time, and eye tracker trace publishing. Also includes Tack world subsystem tick and actor tackification cycle stats.
- UI for it does not exit but the command ```Stat Tack``` exists


## Tack Settings
All Tack Settings and their descriptions are found in the project settings window in the editor ```edit->project settings```. 

### General
- __Experiment Name__ - Experiment name that will be saved in the database.
- __Additional Client Info__ - Useful to store data across lifetime of game instance

### Actor, AI, Input, Camera transform Publisher Settings
These sections of boolean values control what gets saved and by default they are all true. Camera Transform Publisher has additional options that adjusts the frequency and tolerance of when publishing happens.

### Kafka Settings
- __Local Connection String__ - This is the connection string Tack uses to connect to kafka. ex ```localhost:9092``` to connect to a locally hosted kafka server.
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

When packaging a project Settings are NOT copied over unless they are set as default and appear as ```DefaultTack.ini``` in the Config folder in the project. Otherwise the settings must be manually copied the settings folder as ```Tack.ini```. Changing the packaged defaults can be done the same way.
The location is ```\WindowsNoEditor\PACKAGED_NAME\Saved\Config\WindowsNoEditor\Tack.ini```. The easiest way to do this is to run your projects executable once and it will generate the location. Then replace the generated Tack.ini.

## Tack Interfaces

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


## Tack subsystems

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


# Known bugs
- When using seamless server travel and the player controller class changes the tack controller component will be lost and that client will not start tack.
  - Workaround is to keep the player controller class the same across seamless server travels

- ```Stat Tack``` command displays no stats from the stat group