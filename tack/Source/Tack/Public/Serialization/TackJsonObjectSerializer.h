#pragma once
#include "Serialization/TackJsonDomBuilder.h"

class TACK_API FTackObjectSerializer
{
public:
    static void Initialize();

    static FTackJsonObject SerializeObject(const UObject* Object);
    static void SerializeUObject(const UObject* Object, FTackJsonObject& Json);
    static void SerializeActorComponent(const UObject* Object, FTackJsonObject& Json);
    static void SerializeSceneComponent(const UObject* Object, FTackJsonObject& Json);
    static void SerializeChildActorComponent(const UObject* Object, FTackJsonObject& Json);

    static void SerializeActor(const UObject* Object, FTackJsonObject& Json);
    static void SerializeDamageType(const UObject* Object, FTackJsonObject& Json);

private:
    static bool bIsInitialized;
    static TMap<UClass*, TFunction<void(const UObject*, FTackJsonObject&)>> StaticSerializationFunctions;
};