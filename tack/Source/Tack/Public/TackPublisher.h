#pragma once
#include "Subsystems/GameInstanceSubsystem.h"
#include "Dom/JsonObject.h"
#include "TackPublisher.generated.h"

class UWorld;
class AController;

//Kafka forward declarations
struct rd_kafka_s;
typedef struct rd_kafka_s rd_kafka_t;

struct rd_kafka_topic_s;
typedef struct rd_kafka_topic_s rd_kafka_topic_t;

struct rd_kafka_headers_s;
typedef struct rd_kafka_headers_s rd_kafka_headers_t;

UCLASS(BlueprintType)
class TACK_API UTackPublisher : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    static UTackPublisher* GetInstance(const UObject* WorldContextObject);

    UTackPublisher();
    /** Implement this for initialization of instances of the system */
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    void EnablePublisher(const FGuid& SessionGuid, const FGuid& InstanceGuid, const FString& ConnectionString);
    void DisablePublisher();

    void Publish_Data(const FString& TopicName, size_t Size, void* Data, int64_t Timestamp = 0.0);
    void Publish_Data(const FString& TopicName, TArray<uint8>& Data, int64_t Timestamp = 0.0)
    {
        Publish_Data(TopicName, Data.Num(), Data.GetData(), Timestamp);
    }

    void Publish_Json(const FString& TopicName, const TSharedRef<FJsonObject> Json, int64_t Timestamp = 0.0);
    void Publish_Struct(const FString& TopicName, const UScriptStruct* StructProperty, const void* StructPtr, int64_t Timestamp = 0.0);

    //Publish Struct case for UObjects
    template <typename T, std::enable_if_t<TIsDerivedFrom<T, UObject>::Value>>
    void Publish_Struct(const FString& TopicName, T* Data, int64_t Timestamp = 0.0)
    {
        Publish_Struct(TopicName, T::StaticClass(), Data, Timestamp);
    }

    //Publish Struct case for TBaseStructures (FTransform, FBox2D, etc) You can see the full list in CoreUObject\Public\UObject\Class.h
    //Second parameter should only allow this method to be used with Structures. Based on StructOnScope class definition 
    template<typename T, typename = decltype(TBaseStructure<T>::Get())>
    void Publish_Struct(const FString& TopicName, T* Data, int64_t Timestamp = 0.0)
    {
        Publish_Struct(TopicName, TBaseStructure<T>::Get(), Data, Timestamp);
    }

    UFUNCTION(BlueprintCallable, CustomThunk, meta = (CustomStructureParam = "AnyStruct", DisplayName = "Publish Struct", ToolTip = "Publishes Struct of Any Type."))
    void BP_Publish_Struct(const FString& TopicName, UField* AnyStruct);

    //Specific publishers. Mainly used in TackManager
    void PublishTackClientStart(UWorld* World);
    void PublishTackClientEnd(UWorld* World);
    void PublishTackServerStart(UWorld* World);
    void PublishTackServerEnd(UWorld* World);

    void Publish_Client(UWorld* World);
    void PublishInputKeyDetails(UWorld* World);
    void PublishTackState(UWorld* World, FString State);
    void PublishWorldInfo(UWorld* World);

    void PublishController(AController* Controller);
    void PublishPawnControllerChanged(APawn* Pawn, AController* Controller);
    void PublishControllerOnPossessEvent(AController* Controller);

private:

    DECLARE_FUNCTION(execBP_Publish_Struct);

    UPROPERTY()
    bool bPublisherEnabled;

    FGuid PublisherSessionGuid;
    FGuid PublisherInstanceGuid;
    FString PublisherInstanceGuidString;
    FString PublisherConnectionString;

    //Private Kafka things
    rd_kafka_t* GetProducer();
    rd_kafka_topic_t* GetTopic(const FString& TopicName);
    TMap<FString, rd_kafka_topic_t*> KnownTopics;
    rd_kafka_t* c_producer;
    rd_kafka_headers_t* headers;
};