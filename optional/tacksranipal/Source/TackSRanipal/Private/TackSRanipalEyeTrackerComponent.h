#pragma once
#include "Devices/EyeTracker/TackEyeTrackerComponent.h"
#include "Eye/SRanipal_API_Eye.h"
#include "Eye/SRanipal_Eyes_Enums.h"
#include "TackSRanipalEyeTrackerComponent.generated.h"


//We do this to force export the hidden callback in the SRanipal binary 
using VivesrEyeDataCallback = void(*)(ViveSR::anipal::Eye::EyeData const& data);
using VivesrEyeDataCallback_v2 = void(*)(ViveSR::anipal::Eye::EyeData_v2 const& data);

extern "C" {
    namespace ViveSR {
        /** Animation pal
        */
        namespace anipal {
            namespace Eye {

                /* Register a callback function to receive eye camera related data when the module has new outputs.
                [in] function pointer of callback
                [out] error code. please refer Error in ViveSR_Enums.h
                */
                SR_ANIPAL int RegisterEyeDataCallback_v2(VivesrEyeDataCallback_v2 callback);

                /* Unegister a callback function to stop receiving eye camera related data.
                [in] function pointer of callback
                [out] error code. please refer Error in ViveSR_Enums.h
                */
                SR_ANIPAL int UnregisterEyeDataCallback_v2(VivesrEyeDataCallback_v2 callback);
            }
        }
    }
}

using FSRanipalEyeDataTuple = TTuple<ViveSR::anipal::Eye::EyeData_v2, uint64>;

UCLASS()
class UTackSRanipalEyeTrackerComponent : public UTackEyeTrackerComponent
{
    GENERATED_BODY()
public:

    UTackSRanipalEyeTrackerComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    virtual void OnTackStart_Implementation() override;
    virtual void OnTackEnd_Implementation() override;

private:

    void CovertToUnrealLocation(FVector& vector) const;
    void ApplyUnrealWorldToMeterScale(FVector& vector) const;
    void CovertToUnrealQuaternion(FQuat& quat) const;

    static std::atomic<float> CurrentServerWorldTime;
    static TQueue<FSRanipalEyeDataTuple, EQueueMode::Spsc> EyeDataQueue;
    static void EyeDataCallback_v2(ViveSR::anipal::Eye::EyeData_v2 const& eye_data);

    bool SRanipalConnected;


    void ProcessEyeData(FSRanipalEyeDataTuple const& EyeDataTuple);
    FTackJsonDomBuilder::FTackObject ParseSRanipalData(const ViveSR::anipal::Eye::EyeData_v2& EyeData) const;
    FTackJsonDomBuilder::FTackObject ParseSingleEyeData(const ViveSR::anipal::Eye::SingleEyeData& SingleEyeData) const;
    FTackJsonDomBuilder::FTackObject ParseCombinedEyeData(const ViveSR::anipal::Eye::CombinedEyeData& CombinedEyeData) const;
    void CleanUp();
};