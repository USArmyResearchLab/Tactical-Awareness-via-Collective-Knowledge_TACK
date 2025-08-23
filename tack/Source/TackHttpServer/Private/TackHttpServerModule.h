#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class UWorld;

DECLARE_LOG_CATEGORY_EXTERN(LogTackHttpServer, Log, All)

class FTackHttpServerModule : public IModuleInterface
{

public:

    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};