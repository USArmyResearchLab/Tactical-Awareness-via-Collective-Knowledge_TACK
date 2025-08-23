#include "Serialization/TackJsonStructSerializer.h"
#include "JsonObjectConverter.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/EnumProperty.h"
#include "UObject/TextProperty.h"
#include "UObject/PropertyPortFlags.h"
#include "UObject/Package.h"

namespace Helpers
{
    template<typename T>
    TSharedPtr<FJsonValue> SerializeValue(const void* Value)
    {
        return FTackJsonDomBuilder::Serialize(*reinterpret_cast<const T*>(Value));
    }
}

bool FTackJsonStructSerializer::UStructToJsonObject(const UStruct* StructDefinition, const void* Struct, TSharedRef<FJsonObject> OutJsonObject, int64 CheckFlags, int64 SkipFlags)
{
    return  UStructToJsonAttributes(StructDefinition, Struct, OutJsonObject->Values, CheckFlags, SkipFlags);
}

bool FTackJsonStructSerializer::UStructToJsonAttributes(const UStruct* StructDefinition, const void* Struct, TMap< FString, TSharedPtr<FJsonValue> >& OutJsonAttributes, int64 CheckFlags, int64 SkipFlags)
{
    if(SkipFlags == 0)
    {
        // If we have no specified skip flags, skip deprecated, transient and skip serialization by default when writing
        SkipFlags |= CPF_Deprecated | CPF_Transient;
    }

    if(StructDefinition == FJsonObjectWrapper::StaticStruct())
    {
        // Just copy it into the object
        const FJsonObjectWrapper* ProxyObject = (const FJsonObjectWrapper*)Struct;

        if(ProxyObject->JsonObject.IsValid())
        {
            OutJsonAttributes = ProxyObject->JsonObject->Values;
        }
        return true;
    }

    for(TFieldIterator<FProperty> It(StructDefinition); It; ++It)
    {
        FProperty* Property = *It;

        // Check to see if we should ignore this property
        if(CheckFlags != 0 && !Property->HasAnyPropertyFlags(CheckFlags))
        {
            continue;
        }
        if(Property->HasAnyPropertyFlags(SkipFlags))
        {
            continue;
        }

        FString VariableName = Property->GetAuthoredName();
        const void* Value = Property->ContainerPtrToValuePtr<uint8>(Struct);

        // convert the property to a FJsonValue
        TSharedPtr<FJsonValue> JsonValue = UPropertyToJsonValue(Property, Value, CheckFlags, SkipFlags);
        if(!JsonValue.IsValid())
        {
            FFieldClass* PropClass = Property->GetClass();
            UE_LOG(LogTack, Error, TEXT("UStructToJsonObject - Unhandled property type '%s': %s"), *PropClass->GetName(), *Property->GetPathName());
            return false;
        }

        // set the value on the output object
        OutJsonAttributes.Add(VariableName, JsonValue);
    }
    return true;
}

TSharedPtr<FJsonValue> FTackJsonStructSerializer::UPropertyToJsonValue(FProperty* Property, const void* Value, int64 CheckFlags, int64 SkipFlags, FProperty* OuterProperty)
{
    if(Property->ArrayDim == 1)
    {
        return ConvertScalarFPropertyToJsonValue(Property, Value, CheckFlags, SkipFlags, OuterProperty);
    }

    TArray< TSharedPtr<FJsonValue> > Array;
    for(int Index = 0; Index != Property->ArrayDim; ++Index)
    {
        Array.Add(ConvertScalarFPropertyToJsonValue(Property, (char*)Value + Index * Property->ElementSize, CheckFlags, SkipFlags, OuterProperty));
    }
    return MakeShared<FJsonValueArray>(Array);
}

/** Convert property to JSON, assuming either the property is not an array or the value is an individual array element */
TSharedPtr<FJsonValue> FTackJsonStructSerializer::ConvertScalarFPropertyToJsonValue(FProperty* Property, const void* Value, int64 CheckFlags, int64 SkipFlags, FProperty* OuterProperty)
{
    if(FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
    {
        // export enums as strings
        UEnum* EnumDef = EnumProperty->GetEnum();
        FString StringValue = EnumDef->GetNameStringByValue(EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(Value));
        return MakeShared<FJsonValueString>(StringValue);
    }
    else if(FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
    {
        // see if it's an enum
        UEnum* EnumDef = NumericProperty->GetIntPropertyEnum();
        if(EnumDef != NULL)
        {
            // export enums as strings
            FString StringValue = EnumDef->GetNameStringByValue(NumericProperty->GetSignedIntPropertyValue(Value));
            return MakeShared<FJsonValueString>(StringValue);
        }

        // We want to export numbers as numbers
        if(NumericProperty->IsFloatingPoint())
        {
            return MakeShared<FJsonValueNumber>(NumericProperty->GetFloatingPointPropertyValue(Value));
        }
        else if(NumericProperty->IsInteger())
        {
            return MakeShared<FJsonValueNumber>(NumericProperty->GetSignedIntPropertyValue(Value));
        }

        // fall through to default
    }
    else if(FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
    {
        // Export bools as bools
        return MakeShared<FJsonValueBoolean>(BoolProperty->GetPropertyValue(Value));
    }
    else if(FStrProperty* StringProperty = CastField<FStrProperty>(Property))
    {
        return MakeShared<FJsonValueString>(StringProperty->GetPropertyValue(Value));
    }
    else if(FTextProperty* TextProperty = CastField<FTextProperty>(Property))
    {
        return MakeShared<FJsonValueString>(TextProperty->GetPropertyValue(Value).ToString());
    }
    else if(FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
    {
        TArray< TSharedPtr<FJsonValue> > Out;
        FScriptArrayHelper Helper(ArrayProperty, Value);
        for(int32 i = 0, n = Helper.Num(); i < n; ++i)
        {
            TSharedPtr<FJsonValue> Elem = FTackJsonStructSerializer::UPropertyToJsonValue(ArrayProperty->Inner, Helper.GetRawPtr(i), CheckFlags & (~CPF_ParmFlags), SkipFlags, ArrayProperty);
            if(Elem.IsValid())
            {
                // add to the array
                Out.Push(Elem);
            }
        }
        return MakeShared<FJsonValueArray>(Out);
    }
    else if(FSetProperty* SetProperty = CastField<FSetProperty>(Property))
    {
        TArray< TSharedPtr<FJsonValue> > Out;
        FScriptSetHelper Helper(SetProperty, Value);
        for(int32 i = 0, n = Helper.Num(); n; ++i)
        {
            if(Helper.IsValidIndex(i))
            {
                TSharedPtr<FJsonValue> Elem = FTackJsonStructSerializer::UPropertyToJsonValue(SetProperty->ElementProp, Helper.GetElementPtr(i), CheckFlags & (~CPF_ParmFlags), SkipFlags, SetProperty);
                if(Elem.IsValid())
                {
                    // add to the array
                    Out.Push(Elem);
                }
                --n;
            }
        }
        return MakeShared<FJsonValueArray>(Out);
    }
    else if(FMapProperty* MapProperty = CastField<FMapProperty>(Property))
    {
        TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();

        FScriptMapHelper Helper(MapProperty, Value);
        for(int32 i = 0, n = Helper.Num(); n; ++i)
        {
            if(Helper.IsValidIndex(i))
            {
                TSharedPtr<FJsonValue> KeyElement = FTackJsonStructSerializer::UPropertyToJsonValue(MapProperty->KeyProp, Helper.GetKeyPtr(i), CheckFlags & (~CPF_ParmFlags), SkipFlags, MapProperty);
                TSharedPtr<FJsonValue> ValueElement = FTackJsonStructSerializer::UPropertyToJsonValue(MapProperty->ValueProp, Helper.GetValuePtr(i), CheckFlags & (~CPF_ParmFlags), SkipFlags, MapProperty);
                if(KeyElement.IsValid() && ValueElement.IsValid())
                {
                    FString KeyString;
                    if(!KeyElement->TryGetString(KeyString))
                    {
                        MapProperty->KeyProp->ExportTextItem(KeyString, Helper.GetKeyPtr(i), nullptr, nullptr, 0);
                        if(KeyString.IsEmpty())
                        {
                            UE_LOG(LogTack, Error, TEXT("Unable to convert key to string for property %s."), *MapProperty->GetName())
                                KeyString = FString::Printf(TEXT("Unparsed Key %d"), i);
                        }
                    }
                    Out->SetField(KeyString, ValueElement);
                }
                --n;
            }
        }

        return MakeShared<FJsonValueObject>(Out);
    }
    else if(FStructProperty* StructProperty = CastField<FStructProperty>(Property))
    {
        if(StructProperty->Struct == TBaseStructure<FVector>::Get())
        {
            return Helpers::SerializeValue<FVector>(Value);
        }
        else if(StructProperty->Struct == TBaseStructure<FTransform>::Get())
        {
            return Helpers::SerializeValue<FTransform>(Value);
        }
        else if(StructProperty->Struct == TBaseStructure<FHitResult>::Get())
        {
            return Helpers::SerializeValue<FHitResult>(Value);
        }

        UScriptStruct::ICppStructOps* TheCppStructOps = StructProperty->Struct->GetCppStructOps();
        // Intentionally exclude the JSON Object wrapper, which specifically needs to export JSON in an object representation instead of a string
        if(StructProperty->Struct != FJsonObjectWrapper::StaticStruct() && TheCppStructOps && TheCppStructOps->HasExportTextItem())
        {
            FString OutValueStr;
            TheCppStructOps->ExportTextItem(OutValueStr, Value, nullptr, nullptr, PPF_None, nullptr);
            return MakeShared<FJsonValueString>(OutValueStr);
        }

        TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
        if(FTackJsonStructSerializer::UStructToJsonObject(StructProperty->Struct, Value, Out, CheckFlags & (~CPF_ParmFlags), SkipFlags))
        {
            return MakeShared<FJsonValueObject>(Out);
        }
    }
    else if(FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
    {
        // Instanced properties should be copied by value, while normal UObject* properties should output as asset references
        UObject* Object = ObjectProperty->GetObjectPropertyValue(Value);

        if(Object == nullptr)
        {
            return MakeShared<FJsonValueNull>();
        }
        else if(AActor* Actor = Cast<AActor>(Object))
        {
            const FGuid& TackId = UTackStatics::GetTackIdFromActor(Actor);
            if(TackId.IsValid())
            {
                return MakeShared<FJsonValueString>(TackId.ToString(EGuidFormats::DigitsWithHyphens));
            }
            else
            {
                return MakeShared<FJsonValueNull>();
            }
        }
        else if(Object && (ObjectProperty->HasAnyPropertyFlags(CPF_PersistentInstance) || (OuterProperty && OuterProperty->HasAnyPropertyFlags(CPF_PersistentInstance))))
        {
            TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();

            Out->SetStringField("_ClassName", Object->GetClass()->GetFName().ToString());
            if(FTackJsonStructSerializer::UStructToJsonObject(ObjectProperty->GetObjectPropertyValue(Value)->GetClass(), Object, Out, CheckFlags, SkipFlags))
            {
                TSharedRef<FJsonValueObject> JsonObject = MakeShared<FJsonValueObject>(Out);
                JsonObject->Type = EJson::Object;
                return JsonObject;
            }
        }
        else
        {
            FString StringValue;
            Property->ExportTextItem(StringValue, Value, nullptr, nullptr, PPF_None);
            return MakeShared<FJsonValueString>(StringValue);
        }
    }
    else
    {
        // Default to export as string for everything else
        FString StringValue;
        Property->ExportTextItem(StringValue, Value, NULL, NULL, PPF_None);
        return MakeShared<FJsonValueString>(StringValue);
    }

    // invalid
    return TSharedPtr<FJsonValue>();
}