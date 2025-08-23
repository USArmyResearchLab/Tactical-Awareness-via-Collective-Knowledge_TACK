#include "TackPublisher.h"
#include "TackManager.h"
#include "TackSettings.h"

//Json Serialization
#include "Serialization/JsonWriter.h"
#include "Serialization/TackJsonStructSerializer.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/MemoryWriter.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameStateBase.h"
#include "Misc/App.h"
#include "SocketSubsystem.h"

//Kafka includes
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#endif

#if !PLATFORM_HOLOLENS
THIRD_PARTY_INCLUDES_START
#include "librdkafka/rdkafka.h"
THIRD_PARTY_INCLUDES_END
#endif

#if PLATFORM_WINDOWS
#include "Windows/HideWindowsPlatformTypes.h"
#endif


namespace callbacks
{
    void kafka_error_cb(rd_kafka_t* rk, int err, const char* reason, void* opaque)
    {
        FString LogString = FString::Printf(TEXT("%s - ERROR(%d) - %s"), StringCast<TCHAR>(rd_kafka_name(rk)).Get(), err, StringCast<TCHAR>(reason).Get());

        if(GEngine)
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, *LogString);
        UE_LOG(LogTack, Error, TEXT("%s"), *LogString);
    }

    void kafka_log_cb(const rd_kafka_t* rk, int level, const char* fac, const char* buf)
    {
        FString LogString = FString::Printf(TEXT("%s - %s (%d) - %s"), StringCast<TCHAR>(rd_kafka_name(rk)).Get(), StringCast<TCHAR>(fac).Get(), level, StringCast<TCHAR>(buf).Get());

        if(GEngine)
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, *LogString);

        UE_LOG(LogTack, Warning, TEXT("%s"), *LogString);
    }
}

UTackPublisher* UTackPublisher::GetInstance(const UObject* WorldContextObject)
{
    if(UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
    {
        if(UGameInstance* GameInstance = World->GetGameInstance())
        {
            return GameInstance->GetSubsystem<UTackPublisher>();
        }
    }
    return nullptr;
}

UTackPublisher::UTackPublisher() : UGameInstanceSubsystem()
{
    bPublisherEnabled = false;
}

void UTackPublisher::EnablePublisher(const FGuid& SessionGuid, const FGuid& InstanceGuid, const FString& ConnectionString)
{
    if(!bPublisherEnabled)
    {
        bPublisherEnabled = true;
        PublisherSessionGuid = SessionGuid;
        PublisherInstanceGuid = InstanceGuid;
        PublisherInstanceGuidString = InstanceGuid.ToString();
        PublisherConnectionString = ConnectionString;
    }
}

void UTackPublisher::DisablePublisher()
{
    if(bPublisherEnabled)
    {
        bPublisherEnabled = false;

        if(c_producer != nullptr)
        {
#if !PLATFORM_HOLOLENS
            rd_kafka_flush(GetProducer(), 3000);
#endif
        }

        for(auto& Elem : KnownTopics)
        {
            if(Elem.Value != nullptr)
            {
#if !PLATFORM_HOLOLENS
                rd_kafka_topic_destroy(Elem.Value);
#endif
            }
        }
        KnownTopics.Empty();

        if(c_producer != nullptr)
        {
#if !PLATFORM_HOLOLENS
            rd_kafka_destroy(c_producer);
#endif
            c_producer = nullptr;
        }

        if(headers != nullptr)
        {
#if !PLATFORM_HOLOLENS
            rd_kafka_headers_destroy(headers);
#endif
            headers = nullptr;
        }

        PublisherSessionGuid.Invalidate();
        PublisherInstanceGuid.Invalidate();
        PublisherConnectionString = FString();
        PublisherInstanceGuidString = FString();
    }
}

void UTackPublisher::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UTackPublisher::Deinitialize()
{
    Super::Deinitialize();
    DisablePublisher();
}

rd_kafka_t* UTackPublisher::GetProducer()
{
#if !PLATFORM_HOLOLENS
    if(c_producer == nullptr)
    {
        char _errstr[512];
        rd_kafka_conf_t* conf = rd_kafka_conf_new();

        const UTackSettings* Settings = GetDefault<UTackSettings>();

        if(rd_kafka_conf_set(conf, "bootstrap.servers", StringCast<ANSICHAR>(*PublisherConnectionString).Get(), _errstr, sizeof(_errstr)) != RD_KAFKA_CONF_OK)
        {
            UE_LOG(LogTack, Warning, TEXT("%s"), StringCast<TCHAR>(_errstr).Get());
            rd_kafka_conf_destroy(conf);
            return nullptr;
        }

        if(rd_kafka_conf_set(conf, "linger.ms", "5", _errstr, sizeof(_errstr)) != RD_KAFKA_CONF_OK)
        {
            UE_LOG(LogTack, Warning, TEXT("%s"), StringCast<TCHAR>(_errstr).Get());
            rd_kafka_conf_destroy(conf);
            return nullptr;
        };

        if(rd_kafka_conf_set(conf, "enable.idempotence", "true", _errstr, sizeof(_errstr)) != RD_KAFKA_CONF_OK)
        {
            UE_LOG(LogTack, Warning, TEXT("%s"), StringCast<TCHAR>(_errstr).Get());
            rd_kafka_conf_destroy(conf);
            return nullptr;
        };

        if(rd_kafka_conf_set(conf, "compression.codec", "lz4", _errstr, sizeof(_errstr)) != RD_KAFKA_CONF_OK)
        {
            UE_LOG(LogTack, Warning, TEXT("%s"), StringCast<TCHAR>(_errstr).Get());
            rd_kafka_conf_destroy(conf);
            return nullptr;
        };

        if(rd_kafka_conf_set(conf, "partitioner", "consistent", _errstr, sizeof(_errstr)) != RD_KAFKA_CONF_OK)
        {
            UE_LOG(LogTack, Warning, TEXT("%s"), StringCast<TCHAR>(_errstr).Get());
            rd_kafka_conf_destroy(conf);
            return nullptr;
        };

        rd_kafka_conf_set_error_cb(conf, &callbacks::kafka_error_cb);
        rd_kafka_conf_set_log_cb(conf, &callbacks::kafka_log_cb);

        c_producer = rd_kafka_new(RD_KAFKA_PRODUCER, conf, _errstr, sizeof(_errstr));
        if(c_producer == nullptr)
        {
            UE_LOG(LogTack, Warning, TEXT("failed to create new producer: %s"), StringCast<TCHAR>(_errstr).Get());
            rd_kafka_conf_destroy(conf);
            return nullptr;
        }

        headers = rd_kafka_headers_new(2);
        auto HostName = StringCast<ANSICHAR>(FPlatformProcess::ComputerName());
        rd_kafka_header_add(headers, "host", -1, HostName.Get(), -1);
        rd_kafka_header_add(headers, "type", -1, "json", -1);
        rd_kafka_header_add(headers, "time_unit", -1, "us", -1);
        auto InstanceGuid = StringCast<ANSICHAR>(*PublisherInstanceGuid.ToString());
        rd_kafka_header_add(headers, "tack_client_id", -1, InstanceGuid.Get(), -1);
        auto SessionGuid = StringCast<ANSICHAR>(*PublisherSessionGuid.ToString());
        rd_kafka_header_add(headers, "tack_session_id", -1, SessionGuid.Get(), -1);
    }
#endif
    return c_producer;
}

rd_kafka_topic_t* UTackPublisher::GetTopic(const FString& TopicName)
{
    rd_kafka_topic_t* topic = KnownTopics.FindOrAdd(TopicName);
#if !PLATFORM_HOLOLENS
    if(topic == nullptr)
    {
        char _errstr[512];

        rd_kafka_topic_conf_t* topic_conf = rd_kafka_topic_conf_new();

        if(rd_kafka_topic_conf_set(topic_conf, "acks", "all", _errstr, sizeof(_errstr)) != RD_KAFKA_CONF_OK)
        {
            UE_LOG(LogTack, Warning, TEXT("%s"), StringCast<TCHAR>(_errstr).Get());
            rd_kafka_topic_conf_destroy(topic_conf);
            return nullptr;
        }

        topic = rd_kafka_topic_new(GetProducer(), StringCast<ANSICHAR>(*TopicName).Get(), topic_conf);
    }
#endif
    return topic;
}

void UTackPublisher::Publish_Data(const FString& TopicName, size_t Size, void* Data, int64_t Timestamp)
{
#if !PLATFORM_HOLOLENS
    static constexpr int64_t DEFAULT_TIMESTAMP = 0.0;

    //Don't publish unless tack is running
    if(!bPublisherEnabled)
        return;

    if(rd_kafka_topic_t* Topic = GetTopic(TopicName))
    {
        rd_kafka_headers_t* headersCopy = rd_kafka_headers_copy(headers);
        auto Key = StringCast<ANSICHAR>(*PublisherInstanceGuidString);

        rd_kafka_resp_err_t err = rd_kafka_producev(
            c_producer,
            RD_KAFKA_V_RKT(Topic),
            RD_KAFKA_V_VALUE(Data, Size),
            RD_KAFKA_V_KEY(Key.Get(), Key.Length()),
            RD_KAFKA_V_TIMESTAMP(Timestamp == DEFAULT_TIMESTAMP ? UTackStatics::GetCurrentTimestamp() : Timestamp),
            RD_KAFKA_V_HEADERS(headersCopy),
            /* Copy the message payload so the `buf` can
             * be reused for the next message. */
            RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
            RD_KAFKA_V_END);

        if(err != RD_KAFKA_RESP_ERR_NO_ERROR)
        {
            rd_kafka_headers_destroy(headersCopy);
            UE_LOG(LogTack, Warning, TEXT("%s : %s"), StringCast<TCHAR>(rd_kafka_err2name(err)).Get(), StringCast<TCHAR>(rd_kafka_err2str(err)).Get());
        }
    }
#endif
}

void UTackPublisher::Publish_Json(const FString& TopicName, const TSharedRef<FJsonObject> Json, int64_t Timestamp)
{
    TArray<uint8> Buffer;
    FMemoryWriter MemoryWriter(Buffer);
    TSharedRef<TJsonWriter<ANSICHAR, TCondensedJsonPrintPolicy<ANSICHAR>>> JsonWriter = TJsonWriter<ANSICHAR, TCondensedJsonPrintPolicy<ANSICHAR>>::Create(&MemoryWriter);
    FJsonSerializer::Serialize(Json, JsonWriter);
    Publish_Data(TopicName, Buffer, Timestamp);
}

void UTackPublisher::Publish_Struct(const FString& TopicName, const UScriptStruct* StructProperty, const void* StructPtr, int64_t Timestamp)
{
    TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
    if(FTackJsonStructSerializer::UStructToJsonObject(StructProperty, StructPtr, JsonObject))
    {
        Publish_Json(TopicName, JsonObject, Timestamp);
    }
}

DEFINE_FUNCTION(UTackPublisher::execBP_Publish_Struct)
{
    P_GET_PROPERTY(FStrProperty, TopicName);
    Stack.Step(Stack.Object, NULL);
    // Grab the last property found when we walked the stack
    // This does not contains the property value, only its type information
    FProperty* MostRecentProperty = Stack.MostRecentProperty;
    // Grab the base address where the struct actually stores its data
    // This is where the property value is truly stored
    void* StructPtr = Stack.MostRecentPropertyAddress;
    P_FINISH;
    if(FStructProperty* StructProperty = CastField<FStructProperty>(MostRecentProperty))
    {
        P_NATIVE_BEGIN;
        P_THIS->Publish_Struct(TopicName, StructProperty->Struct, StructPtr);
        P_NATIVE_END;
    }
    else
    {
        FBlueprintExceptionInfo ExceptionInfo(
            EBlueprintExceptionType::AccessViolation,
            FText::FromString(TEXT("AnyStruct parameter must be a struct type."))
        );
        FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
    }
}

void UTackPublisher::PublishTackServerStart(UWorld* World)
{
    check(World->IsServer());

    //Publish Starting Tack State Message
    PublishTackState(World, TEXT("Start"));

    //Publish Key Definitions
    PublishInputKeyDetails(World);

    //Publish World Info
    PublishWorldInfo(World);

    //Publish all AIControllers
    for(FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator)
    {
        if(AAIController* AIController = Cast<AAIController>(Iterator->Get()))
        {
            PublishController(AIController);
        }
    }

    //Publish all pawns
    for(TActorIterator<APawn> It(World); It; ++It)
    {
        if(APawn* Pawn = *It)
        {
            PublishPawnControllerChanged(Pawn, Pawn->GetController());
        }
    }
}

void UTackPublisher::PublishTackServerEnd(UWorld* World)
{
    check(World->IsServer());
    PublishTackState(World, TEXT("End"));
}

void UTackPublisher::PublishTackClientStart(UWorld* World)
{
    Publish_Client(World);

    if(APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController(World))
    {
        PublishController(PlayerController);
    }
}

void UTackPublisher::PublishTackClientEnd(UWorld* World)
{
    //Nothing to publish on client end yet
}

void UTackPublisher::PublishTackState(UWorld* World, FString State)
{
    const UTackSettings* Settings = GetDefault<UTackSettings>();
    FTackJsonDomBuilder::FTackObject Object;
    Object.Set(TEXT("ServerWorldTime"), World->GetGameState()->GetServerWorldTimeSeconds());
    Object.Set(TEXT("State"), State);
    Object.Set(TEXT("ProjectName"), FApp::GetProjectName());
    Object.Set(TEXT("ExperimentName"), Settings->ExperimentName);

    Publish_Json(TEXT("tack.session"), Object.AsJsonObject());
}

void UTackPublisher::PublishController(AController* Controller)
{
    FTackJsonDomBuilder::FTackObject Json;
    Json.Set("Controller_id", Controller);
    Json.Set("IsPlayer", Controller->IsPlayerController());
    Json.Set("HasPlayerState", Controller->PlayerState != nullptr);
    Json.Set("IsLocalServerPlayerController", Controller->IsLocalPlayerController());
    if(Controller->IsPlayerController())
    {
        Json.Set("UniqueId", Cast<APlayerController>(Controller)->PlayerState->GetUniqueId()->ToString());
    }
    else
    {
        Json.Set("UniqueId", nullptr);
    }
    Json.Set("ServerWorldTime", Controller->GetWorld()->GetGameState()->GetServerWorldTimeSeconds());

    Publish_Json(TEXT("unreal.controller"), Json.AsJsonObject());
}

void UTackPublisher::PublishPawnControllerChanged(APawn* Pawn, AController* Controller)
{
    FTackJsonDomBuilder::FTackObject Object;
    Object.Set("Pawn_id", Pawn);
    Object.Set("Controller_id", Controller);
    Object.Set("ServerWorldTime", Pawn->GetWorld()->GetGameState()->GetServerWorldTimeSeconds());

    Publish_Json(TEXT("unreal.pawn.controller_changed"), Object.AsJsonObject());
}

void UTackPublisher::PublishControllerOnPossessEvent(AController* Controller)
{
    FTackJsonDomBuilder::FTackObject Object;

    const FGuid& ControllerTackId = UTackStatics::GetTackIdFromActor(Controller);
    Object.Set(TEXT("Controller_id"), ControllerTackId);
    if(!ControllerTackId.IsValid())
        UE_LOG(LogTack, Warning, TEXT("AIController doesn't have player state. Controller_id will be missing from 'unreal.controller.pawn' message"));

    Object.Set(TEXT("Pawn_id"), UTackStatics::GetTackIdFromActor(Controller->GetPawn()));
    Object.Set(TEXT("ServerWorldTime"), Controller->GetWorld()->GetGameState()->GetServerWorldTimeSeconds());

    Publish_Json(TEXT("unreal.controller.pawn"), Object.AsJsonObject());
}

void UTackPublisher::Publish_Client(UWorld* World)
{
    FTackJsonDomBuilder::FTackObject Object;
    auto Manager = UTackManager::GetInstance(World);
    Object.Set(TEXT("id"), Manager->GetLocalInstanceId());
    Object.Set(TEXT("AppInstanceId"), FApp::GetInstanceId().ToString());
    Object.Set(TEXT("AppInstanceName"), FApp::GetInstanceName());
    Object.Set(TEXT("EngineBuildDate"), FApp::GetBuildDate());
    Object.Set(TEXT("GraphicsRHI"), FApp::GetGraphicsRHI());

    //Player Name will be null if running on dedicated server
    if(APlayerController* LocalController = GetGameInstance()->GetFirstLocalPlayerController())
    {
        Object.Set(TEXT("PlayerName"), LocalController->PlayerState->GetPlayerName());
    }
    else
    {
        Object.Set(TEXT("PlayerName"), nullptr);
    }

    Object.Set(TEXT("ProjectName"), FApp::GetProjectName());
    Object.Set(TEXT("ServerWorldTime"), World->GetGameState()->GetServerWorldTimeSeconds());
    Object.Set(TEXT("EngineVersion"), UKismetSystemLibrary::GetEngineVersion());
    Object.Set(TEXT("PlatformUserName"), UKismetSystemLibrary::GetPlatformUserName());

    FString NetMode;
    switch(World->GetNetMode())
    {
    case NM_Standalone:        NetMode = "Standalone"; break;
    case NM_DedicatedServer:   NetMode = "Dedicated"; break;
    case NM_ListenServer:      NetMode = "Listen";  break;
    case NM_Client:            NetMode = "Client"; break;
    }
    Object.Set(TEXT("NetMode"), NetMode);

    Object.Set(TEXT("IsServer"), World->IsServer());
    Object.Set(TEXT("IsClient"), World->IsClient());
    Object.Set(TEXT("IsDedicatedServer"), IsRunningDedicatedServer());

    //add client constants
    FTackJsonDomBuilder::FTackObject AdditionalClientInfoJson;
    {
        //Parse Settings Additional Vars
        for(const TPair<FString, FString>& Pair : GetDefault<UTackSettings>()->AdditionalClientInfo)
        {
            //Values set during runtime can override values set in Settings file
            if(!Manager->AdditionalClientInfo.Contains(Pair.Key))
            {
                AdditionalClientInfoJson.Set(Pair.Key, Pair.Value);
            }
        }

        //Parse Managers Additional Vars
        for(const TPair<FString, FString>& Pair : Manager->AdditionalClientInfo)
        {
            AdditionalClientInfoJson.Set(Pair.Key, Pair.Value);
        }
    }
    Object.Set(TEXT("AdditionalClientInfo"), AdditionalClientInfoJson.AsJsonValue());

    TOptional<FString> local_ip;
    TOptional<FString> hostname;
    if(auto SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM))
    {
        TArray<TSharedPtr<FInternetAddr>> OutAddresses;
        SocketSubsystem->GetLocalAdapterAddresses(OutAddresses);

        if(OutAddresses.Num() > 0)
            local_ip = OutAddresses[0]->ToString(false);

        FString query_hostname;
        if(SocketSubsystem->GetHostName(query_hostname))
            hostname = query_hostname;
    }

    Object.OptionalSet(TEXT("local_ip"), local_ip);
    Object.OptionalSet(TEXT("hostname"), hostname);

    TOptional<FIntPoint> DesktopResolution;
    if(!IsRunningDedicatedServer())
        DesktopResolution = GEngine->GetGameUserSettings()->GetDesktopResolution();
    Object.OptionalSet(TEXT("DesktopResolution"), DesktopResolution);

    Publish_Json(TEXT("unreal.client"), Object.AsJsonObject());
}

void UTackPublisher::PublishWorldInfo(UWorld* World)
{
    auto WorldSettings = World->GetWorldSettings();

    FTackJsonDomBuilder::FTackObject Object;
    Object.Set("MapName", World->GetMapName());
    Object.Set("WorldToMeters", WorldSettings->WorldToMeters);
    Object.Set("KillZ", WorldSettings->KillZ);

    FTackJsonDomBuilder::FTackArray LevelsArray;
    for(const FLevelCollection& LevelCollection : World->GetLevelCollections())
    {
        for(auto Level : LevelCollection.GetLevels())
        {
            LevelsArray.Add(Level->URL.Map);
        }
    }
    Object.Set("Levels", LevelsArray);

    Publish_Json(TEXT("unreal.world"), Object.AsJsonObject());
}

void UTackPublisher::PublishInputKeyDetails(UWorld* World)
{
    TArray<FKey> AllKeys;
    EKeys::GetAllKeys(AllKeys);

    for(auto& Key : AllKeys)
    {
        if(!Key.IsValid())
            continue;

        FTackJsonDomBuilder::FTackObject Object;
        Object.Set(TEXT("Name"), Key.ToString());
        Object.Set(TEXT("ShortDisplayName"), Key.GetDisplayName(false).ToString());
        Object.Set(TEXT("LongDisplayName"), Key.GetDisplayName(true).ToString());
        Object.Set(TEXT("MenuCategory"), Key.GetMenuCategory().ToString());

        Object.Set(TEXT("bIsModifierKey"), Key.IsModifierKey());
        Object.Set(TEXT("bIsGamepadKey"), Key.IsGamepadKey());
        Object.Set(TEXT("bIsTouch"), Key.IsTouch());
        Object.Set(TEXT("bIsMouseButton"), Key.IsMouseButton());
        Object.Set(TEXT("bIsButtonAxis"), Key.IsButtonAxis());
        Object.Set(TEXT("bIsAxis1D"), Key.IsAxis1D());
        Object.Set(TEXT("bIsAxis2D"), Key.IsAxis2D());
        Object.Set(TEXT("bIsAxis3D"), Key.IsAxis3D());
        Object.Set(TEXT("bIsDigital"), Key.IsDigital());
        Object.Set(TEXT("bIsAnalog"), Key.IsAnalog());

        FString PairedAxis;
        switch(Key.GetPairedAxis())
        {
        case EPairedAxis::X: PairedAxis = TEXT("X"); break;
        case EPairedAxis::Y: PairedAxis = TEXT("Y"); break;
        case EPairedAxis::Z: PairedAxis = TEXT("Z"); break;
        default:             PairedAxis = TEXT("Unpaired"); break;
        }
        Object.Set(TEXT("PairedAxis"), PairedAxis);

        Object.Set(TEXT("PairedAxisKey"), Key.GetPairedAxisKey().ToString());
        Object.Set(TEXT("bIsBindableInBlueprints"), Key.IsBindableInBlueprints());
        Object.Set(TEXT("bShouldUpdateAxisWithoutSamples"), Key.ShouldUpdateAxisWithoutSamples());
        Object.Set(TEXT("bIsBindableToActions"), Key.IsBindableToActions());
        Object.Set(TEXT("bIsDeprecated"), Key.IsDeprecated());

        Publish_Json(TEXT("unreal.input.key.definition"), Object.AsJsonObject());
    }
}