// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "TimelinesSaveDataObject.h"
#include "SaveSystemInteropBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSaveSystemInteropEvent);

class URestorationSubsystem;

UCLASS(Abstract, Within = (RestorationSubsystem))
class TIMELINES_API USaveSystemInteropBase : public UObject
{
	GENERATED_BODY()

	friend URestorationSubsystem;

public:
	USaveSystemInteropBase();

protected:
	virtual ITimelinesSaveDataObject* GetObjectForSlot(const FString& SlotName)
		PURE_VIRTUAL(USaveSystemInteropBase::GetObjectForSlot, return nullptr; )

	template <typename TTimelinesSaveDataClass> TTimelinesSaveDataClass* GetObjectForSlotNative(const FString& SlotName)
	{
		static_assert(TPointerIsConvertibleFromTo<TTimelinesSaveDataClass, ITimelinesSaveDataObject>::Value, "'T' template parameter to GetObjectForSlot must be derived from ITimelinesSaveDataObject");
		return Cast<TTimelinesSaveDataClass>(GetObjectForSlot(SlotName));
	}

	virtual TArray<FString> GetAllSlotsSorted() const
		PURE_VIRTUAL(USaveSystemInteropBase::GetAllSlotsSorted, return TArray<FString>(); )

	virtual FString GetCurrentSlot() const
		PURE_VIRTUAL(USaveSystemInteropBase::GetCurrentSlot, return FString(); )

	virtual ITimelinesSaveDataObject* CreateSlot(UObject* WorldContextObject, const FString& SlotName)
	PURE_VIRTUAL(USaveSystemInteropBase::CreateSlot, return nullptr; )

	// This function's implementation must call OnSaveComplete at some point.
	virtual bool SaveSlot(UObject* WorldContextObject, const ITimelinesSaveDataObject* Slot)
		PURE_VIRTUAL(USaveSystemInteropBase::SaveSlot, return false; )

	// This function's implementation must call OnLoadComplete at some point.
	virtual bool LoadSlot(UObject* WorldContextObject, const FString& SlotName)
		PURE_VIRTUAL(USaveSystemInteropBase::WorldContextObject, return false; )

	virtual bool DeleteSlot(const FString& SlotName)
		PURE_VIRTUAL(USaveSystemInteropBase::DeleteSlot, return false; )

	UFUNCTION()
	virtual void OnSaveComplete();

	UFUNCTION()
	virtual void OnLoadComplete();

	UPROPERTY(BlueprintAssignable, Category = "Save System Interop|Events")
	FSaveSystemInteropEvent OnSaveCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Save System Interop|Events")
	FSaveSystemInteropEvent OnLoadCompleted;
};