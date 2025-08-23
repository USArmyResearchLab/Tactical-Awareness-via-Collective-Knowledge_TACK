#pragma once

#include "CoreMinimal.h"
#include "ModularFeatures/TackEyeTrackerModularFeature.h"
#include "Misc/ConfigCacheIni.h"
#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"
#include "Features/IModularFeatures.h"
#include "Features/IModularFeature.h"


DECLARE_LOG_CATEGORY_EXTERN(LogTack, Log, All);

DECLARE_STATS_GROUP(TEXT("Tack"), STATGROUP_Tack, STATCAT_Advanced);

class FTackBackendDummyFeature;

class TACK_API FTackModule : public IModuleInterface
{
public:

    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    /**
      * Singleton-like access to this module's interface.  This is just for convenience!
      * Beware of calling this during the shutdown phase, though. Your module might have been
      * unloaded already.
      *
      * @return Returns singleton instance, loading the module on demand if needed
      */
    static inline FTackModule& Get()
    {
        if(FTackModule::Instance == nullptr)
        {
            return FModuleManager::LoadModuleChecked<FTackModule>("Tack");
        }
        else
        {
            return *FTackModule::Instance;
        }
    }

    /**
      * Checks to see if this module is loaded and ready.  It is only valid to call Get() if
      * IsAvailable() returns true.
      *
      * @return True if the module is loaded and ready to use
      */
    static inline bool IsAvailable()
    {
        return FModuleManager::Get().IsModuleLoaded("Tack");
    }

    //EyeTracker Feature

    /** Returns modular feature name for this module */
    FName GetEyeTrackerModularFeatureName() const
    {
        return ITackEyeTrackerModularFeature::GetModularFeatureName();
    }

    inline ITackEyeTrackerModularFeature& GetEyeTrackerFeature()
    {
        TArray<ITackEyeTrackerModularFeature*> TackModules = IModularFeatures::Get().GetModularFeatureImplementations<ITackEyeTrackerModularFeature>(GetEyeTrackerModularFeatureName());
        TackModules.Sort([](ITackEyeTrackerModularFeature& A, ITackEyeTrackerModularFeature& B) {
            return A.GetModulePriority() > B.GetModulePriority();
            });

        for(ITackEyeTrackerModularFeature* EyeTrackerFeature : TackModules)
        {
            if(EyeTrackerFeature->IsEyeTrackerConnected())
            {
                UE_LOG(LogTack, Log, TEXT("Selected TackEyetracker Features %s (%f)"), *EyeTrackerFeature->GetModuleKeyName(), EyeTrackerFeature->GetModulePriority());
                return *EyeTrackerFeature;
            }
        }
        return *DummyEyeTracker;
    }

    inline bool IsEyeTrackerFeatureAvailable() const
    {
        return IModularFeatures::Get().IsModularFeatureAvailable(GetEyeTrackerModularFeatureName());
    }

private:

    static FTackModule* Instance;

    FTackEyeTrackerDummyFeature* DummyEyeTracker;
};