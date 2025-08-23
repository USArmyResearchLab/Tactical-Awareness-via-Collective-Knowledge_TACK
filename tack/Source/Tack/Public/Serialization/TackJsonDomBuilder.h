#pragma once
#include "JsonDomBuilder.h"

class AActor;
class AActorComponent;
class UTackIdComponent;

class TACK_API FTackJsonDomBuilder
{
public:

    class FTackArray;

    static TSharedRef<FJsonValueObject> Serialize(const FIntPoint& Point);
    static TSharedRef<FJsonValueObject> Serialize(const FVector& Vector);
    static TSharedRef<FJsonValueObject> Serialize(const FTransform& Transform);
    static TSharedRef<FJsonValueObject> Serialize(const FBox& Box);
    static TSharedRef<FJsonValueObject> Serialize(const FBoxSphereBounds& BoxSphereBounds);
    static TSharedRef<FJsonValueObject> Serialize(const FHitResult& HitResult);
    static TSharedRef<FJsonValueObject> Serialize(const FVector2D& Vector2D);
    static TSharedRef<FJsonValueObject> Serialize(const FQuat& Quat);

    class TACK_API FTackObject
    {
    public:

        TSharedRef<FJsonValueObject> AsJsonValue() const { return Object.AsJsonValue(); }
        TSharedRef<FJsonObject> AsJsonObject() const { return Object.AsJsonObject(); }

        FTackObject& Set(const FString& Key, const FJsonDomBuilder::FArray& Arr) { Object.Set(Key, Arr); return *this; }
        FTackObject& Set(const FString& Key, const FJsonDomBuilder::FObject& Obj) { Object.Set(Key, Obj); return *this; }
        FTackObject& Set(const FString& Key, TYPE_OF_NULLPTR) { Object.Set(Key, nullptr); return *this; }
        FTackObject& Set(const FString& Key, TSharedPtr<FJsonValue> Value) { Object.Set(Key, Value); return *this; }
        FTackObject& Set(const FString& Key, const FString& Str) { Object.Set(Key, Str); return *this; }
        FTackObject& Set(const FString& Key, const ANSICHAR* Str) { Object.Set(Key, Str); return *this; }
        FTackObject& Set(const FString& Key, const TCHAR* Str) { Object.Set(Key, Str); return *this; }

        template <class FNumber>
        typename TEnableIf<!TIsSame<FNumber, bool>::Value&& TIsIntegral<FNumber>::Value, FTackObject&>::Type
            Set(const FString& Key, FNumber Number) { Object.Set(Key, Number); return *this; }

        template <class FNumber>
        typename TEnableIf<!TIsSame<FNumber, bool>::Value&& TIsFloatingPoint<FNumber>::Value, FTackObject&>::Type
            Set(const FString& Key, FNumber Number)
        {
            if(FMath::IsFinite(Number))
                Object.Set(Key, Number);
            else
                Object.Set(Key, nullptr);
            return *this;
        }

        template <class FBool>
        typename TEnableIf<TIsSame<FBool, bool>::Value, FTackObject&>::Type
            Set(const FString& Key, FBool Boolean) { Object.Set(Key, Boolean);return *this; }


        // Custom defined types
        FTackObject& Set(const FString& Key, const TSharedRef<FJsonObject>& Obj) { return Set(Key, MakeShared<FJsonValueObject>(Obj)); }
        FTackObject& Set(const FString& Key, const TSharedPtr<FJsonObject>& Obj) { return Set(Key, Obj.ToSharedRef()); }
        FTackObject& Set(const FString& Key, const FTackObject& Obj) { return Set(Key, Obj.AsJsonValue()); }
        FTackObject& Set(const FString& Key, const FTackArray& Arr) { return Set(Key, Arr.AsJsonValue()); }
        FTackObject& Set(const FString& Key, const FGuid& Guid) { return Guid.IsValid() ? Set(Key, Guid.ToString(EGuidFormats::DigitsWithHyphens)) : Set(Key, nullptr); }
        FTackObject& Set(const FString& Key, const FName& Name) { return !Name.IsNone() ? Set(Key, Name.ToString()) : Set(Key, nullptr); }
        FTackObject& Set(const FString& Key, const AActor* Actor);
        FTackObject& Set(const FString& Key, const UActorComponent* Component);


        template<typename T>
        typename TEnableIf<TIsEnumClass<T>::Value || TIsEnum<T>::Value, FTackObject&>::Type
            Set(const FString& Key, const T& EnumValue) { return Set(Key, UEnum::GetValueAsString(EnumValue)); }

        template<typename T>
        FTackObject& OptionalSet(const FString& Key, const TOptional<T>& Value)
        {
            return Value.IsSet() ? Set(Key, Value.GetValue()) : Set(Key, nullptr);
        }

        FTackObject& Set(const FString& Key, const FIntPoint& IntPoint) { return Set(Key, FTackJsonDomBuilder::Serialize(IntPoint)); }
        FTackObject& Set(const FString& Key, const FVector& Vector) { return Set(Key, FTackJsonDomBuilder::Serialize(Vector)); }
        FTackObject& Set(const FString& Key, const FTransform& Transform) { return Set(Key, FTackJsonDomBuilder::Serialize(Transform)); }
        FTackObject& Set(const FString& Key, const FBox& Box) { return Set(Key, FTackJsonDomBuilder::Serialize(Box)); }
        FTackObject& Set(const FString& Key, const FBoxSphereBounds& BoxSphereBounds) { return Set(Key, FTackJsonDomBuilder::Serialize(BoxSphereBounds)); }
        FTackObject& Set(const FString& Key, const FHitResult& HitResult) { return Set(Key, FTackJsonDomBuilder::Serialize(HitResult)); }
        FTackObject& Set(const FString& Key, const FQuat& Quat) { return Set(Key, FTackJsonDomBuilder::Serialize(Quat)); }


    private:
        FJsonDomBuilder::FObject Object;
    };

    class TACK_API FTackArray
    {
    public: // Passthroughs to FJsonDomBuilder::FArray

        TSharedRef<FJsonValueArray> AsJsonValue() const { return Array.AsJsonValue(); }
        int Num() const { return Array.Num(); }

        FTackArray& Add(const FJsonDomBuilder::FArray& Arr) { Array.Add(Arr);   return *this; }
        FTackArray& Add(const FJsonDomBuilder::FObject& Obj) { Array.Add(Obj);   return *this; }
        FTackArray& Add(const FString& Str) { Array.Add(Str);   return *this; }
        FTackArray& Add(const ANSICHAR* Str) { return Add(FString(Str)); }
        FTackArray& Add(const TCHAR* Str) { return Add(FString(Str)); }

        template <class FNumber>
        typename TEnableIf<TIsIntegral<FNumber>::Value || TIsFloatingPoint<FNumber>::Value, FTackArray&>::Type
            Add(FNumber Number) { Array.Add(Number);      return *this; }

        FTackArray& Add(bool Boolean) { Array.Add(Boolean); return *this; }
        FTackArray& Add(TYPE_OF_NULLPTR) { Array.Add(nullptr); return *this; }
        FTackArray& Add(TSharedPtr<FJsonValue> Value) { Array.Add(Value);   return *this; }

        /** Add multiple values */
        template <class... FValue>
        typename TEnableIf<(sizeof...(FValue) > 1), FTackArray&>::Type
            Add(FValue&&... Value)
        {
            // This should be implemented with a fold expression when our compilers support it
            int Temp[] = { 0, (Add(Forward<FValue>(Value)), 0)... };
            (void)Temp;
            return *this;
        }

        // custom types
        FTackArray& Add(const TSharedRef<FJsonObject> Obj) { return Add(MakeShared<FJsonValueObject>(Obj)); }
        FTackArray& Add(const TSharedPtr<FJsonObject> Obj) { return Add(Obj.ToSharedRef()); }
        FTackArray& Add(const FTackObject& Obj) { return Add(Obj.AsJsonValue()); }
        FTackArray& Add(const FTackArray& Arr) { return Add(Arr.AsJsonValue()); }
        FTackArray& Add(const FGuid& Guid) { return Guid.IsValid() ? Add(Guid.ToString(EGuidFormats::DigitsWithHyphens)) : Add(nullptr); }
        FTackArray& Add(const FName& Name) { return !Name.IsNone() ? Add(Name.ToString()) : Add(nullptr); }
        FTackArray& Add(const AActor* Actor);
        FTackArray& Add(const UActorComponent* Component);

        template<typename T>
        typename TEnableIf<TIsEnumClass<T>::Value || TIsEnum<T>::Value, FTackObject&>::Type
            Add(const T& EnumValue) { return Add(UEnum::GetValueAsString(EnumValue)); }

        template<typename T>
        FTackArray& OptionalAdd(const TOptional<T>& Value)
        {
            return Value.IsSet() ? Add(Value.GetValue()) : Add(nullptr);
        }

        FTackArray& Add(const FIntPoint& IntPoint) { return Add(FTackJsonDomBuilder::Serialize(IntPoint)); }
        FTackArray& Add(const FVector& Vector) { return Add(FTackJsonDomBuilder::Serialize(Vector)); }
        FTackArray& Add(const FTransform& Transform) { return Add(FTackJsonDomBuilder::Serialize(Transform)); }
        FTackArray& Add(const FBox& Box) { return Add(FTackJsonDomBuilder::Serialize(Box)); }
        FTackArray& Add(const FBoxSphereBounds& BoxSphereBounds) { return Add(FTackJsonDomBuilder::Serialize(BoxSphereBounds)); }
        FTackArray& Add(const FHitResult& HitResult) { return Add(FTackJsonDomBuilder::Serialize(HitResult)); }
        FTackArray& Add(const FQuat& Quat) { return Add(FTackJsonDomBuilder::Serialize(Quat)); }

    private:
        FJsonDomBuilder::FArray Array;
    };
};

using FTackJsonObject = FTackJsonDomBuilder::FTackObject;
using FTackJsonArray = FTackJsonDomBuilder::FTackArray;