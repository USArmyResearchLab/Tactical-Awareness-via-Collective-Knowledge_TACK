#include "Serialization/TackJsonDomBuilder.h"
#include "TackStatics.h"

//UE4 Engine includes
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"


//FTackObject

FTackJsonDomBuilder::FTackObject& FTackJsonDomBuilder::FTackObject::Set(const FString& Key, const AActor* Actor)
{
   return Set(Key, UTackStatics::GetTackIdFromActor(Actor));
}

FTackJsonDomBuilder::FTackObject& FTackJsonDomBuilder::FTackObject::Set(const FString& Key, const UActorComponent* Component)
{
   return Component != nullptr ? Set(Key, Component->GetName()) : Set(Key, nullptr);
}

//FTackArray

FTackJsonDomBuilder::FTackArray& FTackJsonDomBuilder::FTackArray::Add(const AActor* Actor)
{
   return Add(UTackStatics::GetTackIdFromActor(Actor));
}

FTackJsonDomBuilder::FTackArray& FTackJsonDomBuilder::FTackArray::Add(const UActorComponent* Component)
{
   return Component != nullptr ? Add(Component->GetName()) : Add(Component, nullptr);
}

//Static Serializers
TSharedRef<FJsonValueObject> FTackJsonDomBuilder::Serialize(const FIntPoint& Point)
{
   FJsonDomBuilder::FObject Object;
   Object.Set(TEXT("X"), Point.X);
   Object.Set(TEXT("Y"), Point.Y);
   return Object.AsJsonValue();
}

TSharedRef<FJsonValueObject> FTackJsonDomBuilder::Serialize(const FVector& Vector)
{
   FTackJsonDomBuilder::FTackObject Object;
   Object.Set(TEXT("X"), Vector.X);
   Object.Set(TEXT("Y"), Vector.Y);
   Object.Set(TEXT("Z"), Vector.Z);
   return Object.AsJsonValue();
}

TSharedRef<FJsonValueObject> FTackJsonDomBuilder::Serialize(const FVector2D& Vector2D)
{
   FTackJsonDomBuilder::FTackObject Object;
   Object.Set(TEXT("X"), Vector2D.X);
   Object.Set(TEXT("Y"), Vector2D.Y);
   return Object.AsJsonValue();
}

TSharedRef<FJsonValueObject> FTackJsonDomBuilder::Serialize(const FQuat& Quat)
{
   FTackJsonDomBuilder::FTackObject Object;
   Object.Set(TEXT("X"), Quat.X);
   Object.Set(TEXT("Y"), Quat.Y);
   Object.Set(TEXT("Z"), Quat.Z);
   Object.Set(TEXT("W"), Quat.W);
   return Object.AsJsonValue();
}

TSharedRef<FJsonValueObject> FTackJsonDomBuilder::Serialize(const FTransform& Transform)
{
   FTackJsonDomBuilder::FTackObject Object;
   Object.Set(TEXT("Location"), Transform.GetLocation());
   Object.Set(TEXT("Rotation"), Transform.GetRotation().Euler());
   Object.Set(TEXT("Quat"), Transform.GetRotation());
   Object.Set(TEXT("Scale3D"), Transform.GetScale3D());
   Object.Set(TEXT("Forward"), Transform.GetUnitAxis(EAxis::X));
   Object.Set(TEXT("Right"), Transform.GetUnitAxis(EAxis::Y));
   Object.Set(TEXT("Up"), Transform.GetUnitAxis(EAxis::Z));
   return Object.AsJsonValue();
}

TSharedRef<FJsonValueObject> FTackJsonDomBuilder::Serialize(const FBox& Box)
{
   FTackJsonDomBuilder::FTackObject Object;
   FVector Origin;
   FVector Extent;
   Box.GetCenterAndExtents(Origin, Extent);
   Object.Set(TEXT("Origin"), Origin);
   Object.Set(TEXT("Extent"), Extent);
   return Object.AsJsonValue();
}

TSharedRef<FJsonValueObject> FTackJsonDomBuilder::Serialize(const FBoxSphereBounds& BoxSphereBounds)
{
   FTackJsonDomBuilder::FTackObject Object;
   Object.Set(TEXT("Origin"), BoxSphereBounds.Origin);
   Object.Set(TEXT("BoxExtent"), BoxSphereBounds.BoxExtent);
   Object.Set(TEXT("SphereRadius"), BoxSphereBounds.SphereRadius);
   return Object.AsJsonValue();
}

TSharedRef<FJsonValueObject> FTackJsonDomBuilder::Serialize(const FHitResult& HitResult)
{
   FTackJsonDomBuilder::FTackObject Object;
   Object.Set("bBlockingHit", HitResult.bBlockingHit);
   Object.Set("bStartPenetrating", HitResult.bStartPenetrating);
   Object.Set("FaceIndex", HitResult.FaceIndex);
   Object.Set("Time", HitResult.Time);
   Object.Set("Distance", HitResult.Distance);
   Object.Set("Location", HitResult.Location);
   Object.Set("ImpactPoint", HitResult.ImpactPoint);
   Object.Set("Normal", HitResult.Normal);
   Object.Set("ImpactNormal", HitResult.ImpactNormal);
   Object.Set("TraceStart", HitResult.TraceStart);
   Object.Set("TraceEnd", HitResult.TraceEnd);
   Object.Set("PenetractionDepth", HitResult.PenetrationDepth);
   Object.Set("Actor", HitResult.GetActor());

   Object.OptionalSet("Component", (HitResult.GetComponent() != nullptr) ? HitResult.GetComponent()->GetName() : TOptional<FString>());
   Object.Set("BoneName", HitResult.BoneName);
   Object.Set("MyBoneName", HitResult.MyBoneName);

   return Object.AsJsonValue();
}