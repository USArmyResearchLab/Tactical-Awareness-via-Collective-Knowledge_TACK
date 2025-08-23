#pragma once
#include "Devices/EyeTracker/TackEyeTrackerComponent.h"
#include "Devices/EyeTracker/TackDefaultEyeTrackerComponent.h"
#include <type_traits>
#include "Features/IModularFeatures.h"
#include "Features/IModularFeature.h"

class TACK_API ITackEyeTrackerModularFeature : public IModularFeature
{
public:

    static FName GetModularFeatureName()
    {
        static const FName TackBackendFeatureName(TEXT("TackEyeTracker"));
        return TackBackendFeatureName;
    }

    //This was based off of IEyeTracker. How the Config is set will probably change.
    float GetModulePriority() const
    {
        float ModulePriority = 0.f;
        FString KeyName = GetModuleKeyName();
        if(KeyName == TEXT("Dummy"))
        {
            ModulePriority = -1.0f;
        }
        else
        {
            GConfig->GetFloat(TEXT("TackEyeTrackerFeaturePriority"), (!KeyName.IsEmpty() ? *KeyName : TEXT("Dummy")), ModulePriority, GEngineIni);
        }

        return ModulePriority;
    }

    // Interface
    virtual FString GetModuleKeyName() const = 0;
    virtual TSubclassOf<UTackEyeTrackerComponent> GetTackEyeTrackerComponentClass() const = 0;
    virtual bool IsEyeTrackerConnected() = 0;
};

//Dummy Feature when no backend Feature exists
class FTackEyeTrackerDummyFeature : public ITackEyeTrackerModularFeature
{
public:

    virtual ~FTackEyeTrackerDummyFeature()
    {
    }

    virtual FString GetModuleKeyName() const override
    {
        return FString(TEXT("Dummy"));
    }

    virtual TSubclassOf<UTackEyeTrackerComponent> GetTackEyeTrackerComponentClass() const override
    {
        return UTackDefaultEyeTrackerComponent::StaticClass();
    }

    virtual bool IsEyeTrackerConnected() override
    {
        return true;
    }
};