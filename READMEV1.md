# TACK

**Tactical Awareness via Collective Knowledge (TACK)** is a collection of plugins for the Unreal Engine that record both game/world and operator/player state for both real-time
and post hoc analysis/exploitation. These plugins publish player/game state via kafka. External software can then listen to the kafka data streams.


## Installation

To install any realated Tack Unreal plugins drop them into the Plugins folder of an Unreal project. 

if installing from git Make sure to run ```git lfs install``` before cloning. Otherwise the binaries will be pulled incorrectly.

## Usage

Tack must be manually Started and stopped. Either through the console or a function call.

Tack supports multiple sessions at the same time going to the same tack-wsl-back.

You can start/stop tack from the console by using the commands ```Tack.Start``` and ```Tack.End```. You can open the console in non-shipping builds with ```~```.

You can programmatically start/stop tack using ```UTackStatics::StartTack``` and ```UTackStatics::StopTack``` functions. These are callable from both blueprints and c++.

Tack provides the ability to publish any blueprint or c++ defined struct by using ```UTackStaticPublishers::BP_Publish_Struct```, (available in blueprints and c++) or ```FTackModule::Get().GetBackendFeature().Publish_Struct(TopicName, StructProperty->Struct, StructPtr)``` (available in c++) functions. If Tack has not been started these functions will not publish anything.

The general flow of an experiment will go like this.

- First start tack-wsl-back
- Start Tack
- Stop Tack
- Repeat Start and Stop as much as needed
- Stop tack-wsl-back once everything is complete

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



## Options

All Tack Options and their descriptions are found in the project settings window in the editor ```edit->project settings```. 

Below are some Important Settings.

- __ConnectionString__ - this is the connection string Tack uses to connect to kafka. ex) ```localhost:9092``` to connect to a locally hosted kafka server.
- __bEnableSnapshots__ - This allows snapshots to be taken.
- __bEnableSnapshotPublish__ - This allows snapshots to be published over kafka.

### Snapshot Options

-  __CompressionQuality__ - Controls the compression quality of the JPEG snapshot. 0 is the default value and equal to 80, higher values are higher compression.

- __CaptureFrameRate__ - Frame rate of the desired snapshots to be made. Max value of 24.

- __bCropDesiredSize__ - Control if the desired size should be overridden. This will center crop the snapshot if it is smaller than the screen resolution This is needed for vr snapshots.

- __DesiredSize__ - The size of the desired snapshot output.

When packaging a project Settings are NOT copied over. You must manually copy the settings file over.
The location is "\WindowsNoEditor\PACKAGED_NAME\Saved\Config\WindowsNoEditor\Tack.ini". The easiest way to do this is to run your projects executable once and it will generate the location. Then replace the generated Tack.ini.


## Things to Know

- Tack will work in single or multiplayer. This includes both dediacted and listen servers.
- Packaging a shipping build will compile out the console. Meaning you cannot use the console commands to start/stop Tack.
- Tack does no support traveling between worlds/maps. So an experiment should be conatined within one map. Or Tack must be stopped before an ```open level``` / ```server travel``` command.

