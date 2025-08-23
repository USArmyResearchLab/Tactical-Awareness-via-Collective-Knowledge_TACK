#include "Serialization/TackJsonObjectSerializer.h"

//UE4 Engine includes
#include "CoreMinimal.h"
#include "Landscape.h"
#include "GameFramework/Actor.h"
#include "GameFramework/DamageType.h"
#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/ChildActorComponent.h"

TMap<UClass*, TFunction<void(const UObject*, FTackJsonObject&)>> FTackObjectSerializer::StaticSerializationFunctions;
bool FTackObjectSerializer::bIsInitialized = false;

void FTackObjectSerializer::Initialize()
{
   if(!FTackObjectSerializer::bIsInitialized)
   {
      FTackObjectSerializer::StaticSerializationFunctions.Add(UObject::StaticClass(), &FTackObjectSerializer::SerializeUObject);
      FTackObjectSerializer::StaticSerializationFunctions.Add(UActorComponent::StaticClass(), &FTackObjectSerializer::SerializeActorComponent);
      FTackObjectSerializer::StaticSerializationFunctions.Add(USceneComponent::StaticClass(), &FTackObjectSerializer::SerializeSceneComponent);
      FTackObjectSerializer::StaticSerializationFunctions.Add(UChildActorComponent::StaticClass(), &FTackObjectSerializer::SerializeChildActorComponent);
      FTackObjectSerializer::StaticSerializationFunctions.Add(AActor::StaticClass(), &FTackObjectSerializer::SerializeActor);
      FTackObjectSerializer::StaticSerializationFunctions.Add(UDamageType::StaticClass(), &FTackObjectSerializer::SerializeDamageType);
      FTackObjectSerializer::bIsInitialized = true;
   }
}

FTackJsonDomBuilder::FTackObject FTackObjectSerializer::SerializeObject(const UObject* Object)
{
   FTackJsonObject Json;

   if(Object == nullptr)
      return Json;

   for(
      UClass* CurrentClass = Object->GetClass();
      CurrentClass != UObject::StaticClass();
      CurrentClass = CurrentClass->GetSuperClass()
      )
   {
      if(auto SerializationFunc = StaticSerializationFunctions.Find(CurrentClass))
      {
         (*SerializationFunc)(Object, Json);
         return Json;
      }
   }

   //Default to UObject Serializer
   FTackObjectSerializer::SerializeUObject(Object, Json);
   return Json;
}

void FTackObjectSerializer::SerializeUObject(const UObject* Object, FTackJsonObject& Json)
{
   FString d = Object->GetName();
   Json.Set("Name", Object->GetName());

   FTackJsonDomBuilder::FTackArray Array;
   UClass* ObjectClass = Object->GetClass();
   do
   {
      Array.Add(ObjectClass->GetName());
      ObjectClass = ObjectClass->GetSuperClass();
   } while(ObjectClass != UObject::StaticClass());

   Json.Set("Class", Array);
}

void FTackObjectSerializer::SerializeActorComponent(const UObject* Object, FTackJsonObject& Json)
{
   FTackObjectSerializer::SerializeUObject(Object, Json);

   auto ActorComponent = static_cast<const UActorComponent*>(Object);

   FTackJsonDomBuilder::FTackArray TagsArray;
   for(auto TagIt = ActorComponent->ComponentTags.CreateConstIterator(); TagIt; ++TagIt)
   {
      TagsArray.Add((*TagIt));
   }
   Json.Set("Tags", TagsArray);
}

void FTackObjectSerializer::SerializeSceneComponent(const UObject* Object, FTackJsonObject& Json)
{
   FTackObjectSerializer::SerializeActorComponent(Object, Json);

   auto SceneComponent = static_cast<const USceneComponent*>(Object);

   Json.Set("IsVisible", SceneComponent->IsVisible());
   Json.Set("IsCollisionEnabled", SceneComponent->IsCollisionEnabled());
   Json.Set("IsAnySimulatingPhysics", SceneComponent->IsAnySimulatingPhysics());
   Json.Set("Transform", SceneComponent->GetComponentTransform());
   Json.Set("Bounds", SceneComponent->CalcBounds(SceneComponent->GetOwner()->ActorToWorld()).GetBox());

   TOptional<FString> AttachParentName;
   if(auto AttachParent = SceneComponent->GetAttachParent())
      AttachParentName = AttachParent->GetName();
   Json.OptionalSet("AttachParent", AttachParentName);
}

void FTackObjectSerializer::SerializeChildActorComponent(const UObject* Object, FTackJsonObject& Json)
{
   FTackObjectSerializer::SerializeSceneComponent(Object, Json);

   auto ChildActorComponent = static_cast<const UChildActorComponent*>(Object);

   Json.Set("Child_id", ChildActorComponent->GetChildActor());

   TOptional<FString> ChildActorClassName;
   if(auto ChildActorClass = ChildActorComponent->GetChildActorClass())
      ChildActorClassName = ChildActorComponent->GetName();
   Json.OptionalSet("ChildActorClass", ChildActorClassName);

   Json.Set("ChildActorName", ChildActorComponent->GetChildActorName());
}

void FTackObjectSerializer::SerializeActor(const UObject* Object, FTackJsonObject& Json)
{
   check(Object->IsA<AActor>());
   FTackObjectSerializer::SerializeUObject(Object, Json);
   auto Actor = static_cast<const AActor*>(Object);

   Json.Set("id", Actor);
   Json.Set("bHidden", static_cast<bool>(Actor->IsHidden()));
   Json.Set("bNetStartup", Actor->IsNetStartupActor());
   Json.Set("bIsInPersistentLevel", Actor->IsInPersistentLevel());
   Json.Set("bCollisionEnabled", Actor->GetActorEnableCollision());
   Json.Set("LevelName", Actor->GetLevel()->GetName());
   Json.Set("Owner_id", Actor->GetOwner());
   Json.Set("Parent_id", Actor->GetParentActor());
   Json.Set("IsChildActor", Actor->IsChildActor());
   Json.Set("CreationTime", Actor->CreationTime);

   TOptional<EComponentMobility::Type> Mobility;
   if(auto RootComponent = Actor->GetRootComponent())
      Mobility = RootComponent->Mobility;
   Json.OptionalSet("Mobility", Mobility);

   Json.Set("SpawnTransform", Actor->GetTransform());
   Json.Set("Bounds", Actor->GetComponentsBoundingBox(true, true));
   Json.Set("CollisionBounds", Actor->GetComponentsBoundingBox(false, true));

   FTackJsonDomBuilder::FTackArray TagsArray;
   for(auto TagIt = Actor->Tags.CreateConstIterator(); TagIt; ++TagIt)
   {
      TagsArray.Add((*TagIt));
   }
   Json.Set("Tags", TagsArray);

   FTackJsonDomBuilder::FTackArray ComponentsArray;
   //HACK for Large Landscapes. Skip all USplineMeshComponents if set. Large landscapes cause kafka messages to be to large.
   if(Actor->IsA<ALandscape>() && !GetDefault<UTackSettings>()->bPublishLandscapeSplineMeshComponents)
   {
      Actor->ForEachComponent<UActorComponent>(false, [&ComponentsArray](UActorComponent* ActorComponent) {
         if(!ActorComponent->IsA<USplineMeshComponent>())
         {
            ComponentsArray.Add(FTackObjectSerializer::SerializeObject(ActorComponent));
         }
         });
   }
   else
   {
      Actor->ForEachComponent<UActorComponent>(false, [&ComponentsArray](UActorComponent* ActorComponent) {
         if(ActorComponent != nullptr && ActorComponent->IsValidLowLevel())
         {
            ComponentsArray.Add(FTackObjectSerializer::SerializeObject(ActorComponent));
         }
         });
   }

   Json.Set("Components", ComponentsArray);
}

void FTackObjectSerializer::SerializeDamageType(const UObject* Object, FTackJsonObject& Json)
{
   FTackObjectSerializer::SerializeUObject(Object, Json);
   auto DamageType = static_cast<const UDamageType*>(Object);

   Json.Set("bCausedByWorld", static_cast<bool>(DamageType->bCausedByWorld));
   Json.Set("bScaleMomentumByMass", static_cast<bool>(DamageType->bScaleMomentumByMass));
   Json.Set("bRadialDamageVelChange", static_cast<bool>(DamageType->bRadialDamageVelChange));
   Json.Set("DamageImpulse", DamageType->DamageImpulse);
   Json.Set("DestructibleImpulse", DamageType->DestructibleImpulse);
   Json.Set("DestructibleDamageSpreadScale", DamageType->DestructibleDamageSpreadScale);
   Json.Set("DamageFalloff", DamageType->DamageFalloff);
}