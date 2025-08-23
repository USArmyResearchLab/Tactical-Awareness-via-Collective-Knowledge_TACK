#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "IHttpRouter.h"
#include "HttpServerConstants.h"
#include "TackHttpServerSubsystem.generated.h"

UCLASS()
class UTackHttpServerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:

    /** Override to control if the Subsystem should be created at all.
     * For example you could only have your system created on servers.
     * It's important to note that if using this is becomes very important to null check whenever getting the Subsystem.
     *
     * Note: This function is called on the CDO prior to instances being created!
     *
     */
    virtual bool ShouldCreateSubsystem(UObject* Outer) const;

    /** Implement this for initialization of instances of the system */
    virtual void Initialize(FSubsystemCollectionBase& Collection);

    /** Implement this for deinitialization of instances of the system */
    virtual void Deinitialize();

private:
    TSharedPtr<IHttpRouter> HttpRouter;

    FHttpRouteHandle StartTackRoute;
    FHttpRouteHandle StopTackRoute;
    FHttpRouteHandle StatusTackRoute;

    //Routes

    bool HandleStartTackRoute(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
    bool HandleStopTackRoute(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
    bool HandleStatusTackRoute(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);

    //Functions copied from WebRemoteControlUtils

    TUniquePtr<FHttpServerResponse> CreateHttpResponse(EHttpServerResponseCodes InResponseCode = EHttpServerResponseCodes::BadRequest);

    void AddCORSHeaders(FHttpServerResponse* InOutResponse);
    void AddContentTypeHeaders(FHttpServerResponse* InOutResponse, FString InContentType);
    bool ValidateContentType(const FHttpServerRequest& InRequest, FString InContentType, const FHttpResultCallback& InCompleteCallback);
    bool IsRequestContentType(const FHttpServerRequest& InRequest, const FString& InContentType, FString* OutErrorText);
    void CreateUTF8ErrorMessage(const FString& InMessage, TArray<uint8>& OutUTF8Message);
    void ConvertToUTF8(TConstArrayView<uint8> InTCHARPayload, TArray<uint8>& OutUTF8Payload);
    void ConvertToUTF8(const FString& InString, TArray<uint8>& OutUTF8Payload);
};