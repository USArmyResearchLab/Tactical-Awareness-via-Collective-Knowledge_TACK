
#include "TackHttpServerSubsystem.h"
#include "TackHttpServerModule.h"
#include "TackSettings.h"
#include "TackManager.h"
#include "HttpServerModule.h"
#include "HttpServerResponse.h"
#include "HttpPath.h"
#include "TackStatics.h"
#include "Components/TackGameStateBaseComponent.h"

bool UTackHttpServerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    return GetDefault<UTackSettings>()->bEnableHttpServer;
}

/** Implement this for initialization of instances of the system */
void UTackHttpServerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    const UTackSettings* Settings = GetDefault<UTackSettings>();
    HttpRouter = FHttpServerModule::Get().GetHttpRouter(Settings->HttpServerPort);

    StartTackRoute = HttpRouter->BindRoute(FHttpPath(TEXT("/tack/session/start")), EHttpServerRequestVerbs::VERB_PUT, [this](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
        {
            return HandleStartTackRoute(Request, OnComplete);
        });

    StartTackRoute = HttpRouter->BindRoute(FHttpPath(TEXT("/tack/session/stop")), EHttpServerRequestVerbs::VERB_PUT, [this](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
        {
            return HandleStopTackRoute(Request, OnComplete);
        });

    StatusTackRoute = HttpRouter->BindRoute(FHttpPath(TEXT("/tack/session/status")), EHttpServerRequestVerbs::VERB_GET, [this](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
        {
            return HandleStatusTackRoute(Request, OnComplete);
        });

    FHttpServerModule::Get().StartAllListeners();
}

bool UTackHttpServerSubsystem::HandleStartTackRoute(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
    TUniquePtr<FHttpServerResponse> Response = CreateHttpResponse();

    UWorld* World = GetGameInstance()->GetWorld();
    if(World != nullptr && World->IsGameWorld())
    {
        auto TackManager = UTackManager::GetInstance(World);
        TackManager->StartTack();
        Response->Code = EHttpServerResponseCodes::Ok;
    }
    else
    {
        Response->Code = EHttpServerResponseCodes::ServiceUnavail;
        UE_LOG(LogTackHttpServer, Log, TEXT("Request to START tack has failed. GameWorld was not valid"));
    }

    OnComplete(MoveTemp(Response));
    return true;
}

bool UTackHttpServerSubsystem::HandleStopTackRoute(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
    TUniquePtr<FHttpServerResponse> Response = CreateHttpResponse();

    UWorld* World = GetGameInstance()->GetWorld();
    if(World != nullptr && World->IsGameWorld())
    {
        auto TackManager = UTackManager::GetInstance(World);
        TackManager->StopTack();

        Response->Code = EHttpServerResponseCodes::Ok;
    }
    else
    {
        Response->Code = EHttpServerResponseCodes::ServiceUnavail;
        UE_LOG(LogTackHttpServer, Log, TEXT("Request to STOP tack has failed. Could not get a valid GameWorld from GEngine"));
    }

    OnComplete(MoveTemp(Response));
    return true;
}

bool UTackHttpServerSubsystem::HandleStatusTackRoute(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
    TUniquePtr<FHttpServerResponse> Response = CreateHttpResponse();

    UWorld* World = GetGameInstance()->GetWorld();
    if(World != nullptr && World->IsGameWorld())
    {
        auto TackManager = UTackManager::GetInstance(World);

        FMemoryWriter Writer(Response->Body);
        TSharedPtr<TJsonWriter<ANSICHAR>> JsonWriter = TJsonWriter<ANSICHAR>::Create(&Writer);

        JsonWriter->WriteObjectStart();
        JsonWriter->WriteValue(TEXT("Status"), TackManager->IsTackRunning() ? TEXT("Running") : TEXT("NotRunning"));
        JsonWriter->WriteObjectEnd();

        Response->Code = EHttpServerResponseCodes::Ok;
    }
    else
    {
        Response->Code = EHttpServerResponseCodes::ServiceUnavail;
        UE_LOG(LogTackHttpServer, Log, TEXT("Request to STOP tack has failed. Could not get a valid GameWorld from GEngine"));
    }

    OnComplete(MoveTemp(Response));
    return true;

}

/** Implement this for deinitialization of instances of the system */
void UTackHttpServerSubsystem::Deinitialize()
{
    if(FHttpServerModule::IsAvailable())
    {
        FHttpServerModule::Get().StopAllListeners();
    }

    if(HttpRouter)
    {
        if(StartTackRoute.IsValid())
            HttpRouter->UnbindRoute(StartTackRoute);

        if(StopTackRoute.IsValid())
            HttpRouter->UnbindRoute(StopTackRoute);

        if(StatusTackRoute.IsValid())
            HttpRouter->UnbindRoute(StatusTackRoute);
    }

    HttpRouter.Reset();
}

TUniquePtr<FHttpServerResponse> UTackHttpServerSubsystem::CreateHttpResponse(EHttpServerResponseCodes InResponseCode)
{
    TUniquePtr<FHttpServerResponse> Response = MakeUnique<FHttpServerResponse>();
    AddCORSHeaders(Response.Get());
    AddContentTypeHeaders(Response.Get(), TEXT("application/json"));
    Response->Code = InResponseCode;
    return Response;
}

void UTackHttpServerSubsystem::AddCORSHeaders(FHttpServerResponse* InOutResponse)
{
    check(InOutResponse != nullptr);
    InOutResponse->Headers.Add(TEXT("Access-Control-Allow-Origin"), { TEXT("*") });
    InOutResponse->Headers.Add(TEXT("Access-Control-Allow-Methods"), { TEXT("PUT, POST, GET, OPTIONS") });
    InOutResponse->Headers.Add(TEXT("Access-Control-Allow-Headers"), { TEXT("Origin, X-Requested-With, Content-Type, Accept") });
    InOutResponse->Headers.Add(TEXT("Access-Control-Max-Age"), { TEXT("600") });
}

void UTackHttpServerSubsystem::AddContentTypeHeaders(FHttpServerResponse* InOutResponse, FString InContentType)
{
    InOutResponse->Headers.Add(TEXT("content-type"), { MoveTemp(InContentType) });
}

bool UTackHttpServerSubsystem::ValidateContentType(const FHttpServerRequest& InRequest, FString InContentType, const FHttpResultCallback& InCompleteCallback)
{
    FString ErrorText;
    if(!IsRequestContentType(InRequest, MoveTemp(InContentType), &ErrorText))
    {
        TUniquePtr<FHttpServerResponse> Response = CreateHttpResponse();
        CreateUTF8ErrorMessage(ErrorText, Response->Body);
        InCompleteCallback(MoveTemp(Response));
        return false;
    }
    return true;
}

bool UTackHttpServerSubsystem::IsRequestContentType(const FHttpServerRequest& InRequest, const FString& InContentType, FString* OutErrorText)
{
    if(const TArray<FString>* ContentTypeHeaders = InRequest.Headers.Find(TEXT("Content-Type")))
    {
        if(ContentTypeHeaders->Num() > 0 && (*ContentTypeHeaders)[0] == InContentType)
        {
            return true;
        }
    }

    if(OutErrorText)
    {
        *OutErrorText = FString::Printf(TEXT("Request content type must be %s"), *InContentType);
    }
    return false;
}

void UTackHttpServerSubsystem::CreateUTF8ErrorMessage(const FString& InMessage, TArray<uint8>& OutUTF8Message)
{
    ConvertToUTF8(FString::Printf(TEXT("{ \"errorMessage\": \"%s\" }"), *InMessage), OutUTF8Message);
}

void UTackHttpServerSubsystem::ConvertToUTF8(TConstArrayView<uint8> InTCHARPayload, TArray<uint8>& OutUTF8Payload)
{
    int32 StartIndex = OutUTF8Payload.Num();
    OutUTF8Payload.AddUninitialized(FTCHARToUTF8_Convert::ConvertedLength((TCHAR*)InTCHARPayload.GetData(), InTCHARPayload.Num() / sizeof(TCHAR)) * sizeof(ANSICHAR));
    FTCHARToUTF8_Convert::Convert((ANSICHAR*)(OutUTF8Payload.GetData() + StartIndex), (OutUTF8Payload.Num() - StartIndex) / sizeof(ANSICHAR), (TCHAR*)InTCHARPayload.GetData(), InTCHARPayload.Num() / sizeof(TCHAR));
}

void UTackHttpServerSubsystem::ConvertToUTF8(const FString& InString, TArray<uint8>& OutUTF8Payload)
{
    int32 StartIndex = OutUTF8Payload.Num();
    OutUTF8Payload.AddUninitialized(FTCHARToUTF8_Convert::ConvertedLength(*InString, InString.Len()) * sizeof(ANSICHAR));
    FTCHARToUTF8_Convert::Convert((ANSICHAR*)(OutUTF8Payload.GetData() + StartIndex), (OutUTF8Payload.Num() - StartIndex) / sizeof(ANSICHAR), *InString, InString.Len());
}