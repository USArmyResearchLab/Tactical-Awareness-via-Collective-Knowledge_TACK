#include "Tack.h"
#include "Serialization/TackJsonObjectSerializer.h"

DEFINE_LOG_CATEGORY(LogTack);


FTackModule* FTackModule::Instance = nullptr;

void FTackModule::StartupModule()
{
    FTackModule::Instance = this;
    DummyEyeTracker = new FTackEyeTrackerDummyFeature();
    IModularFeatures::Get().RegisterModularFeature(ITackEyeTrackerModularFeature::GetModularFeatureName(), DummyEyeTracker);

    FTackObjectSerializer::Initialize();
}

void FTackModule::ShutdownModule()
{
    delete DummyEyeTracker;
}

//Module

IMPLEMENT_MODULE(FTackModule, Tack)