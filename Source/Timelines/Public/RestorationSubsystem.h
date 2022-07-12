// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TimelinesStructs.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RestorationSubsystem.generated.h"

UINTERFACE()
class TIMELINES_API UTimelinesSaveDataObject : public UInterface
{
	GENERATED_BODY()
};

class TIMELINES_API ITimelinesSaveDataObject
{
	GENERATED_BODY()

public:
	virtual FTimelineGameKey GetGameKey() const = 0;
	virtual void SetGameKey(const FTimelineGameKey& Key) = 0;
	virtual FString GetSlotName() const = 0;
	virtual bool SetupSlot(UObject* WorldContextObject) = 0;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSaveSystemInteropEvent);

UCLASS(Abstract)
class TIMELINES_API USaveSystemInterop : public UObject
{
	GENERATED_BODY()

	friend class URestorationSubsystem;

public:
	USaveSystemInterop();

protected:
	virtual ITimelinesSaveDataObject* GetObjectForSlot(const FString& SlotName)
		PURE_VIRTUAL(USaveSystemInterop::GetObjectForSlot, return nullptr; )

	template <typename TTimelinesSaveDataClass> TTimelinesSaveDataClass* GetObjectForSlotNative(const FString& SlotName)
	{
		static_assert(TPointerIsConvertibleFromTo<TTimelinesSaveDataClass, ITimelinesSaveDataObject>::Value, "'T' template parameter to GetObjectForSlot must be derived from ITimelinesSaveDataObject");
		return Cast<TTimelinesSaveDataClass>(GetObjectForSlot(SlotName));
	}

	virtual TArray<FString> GetAllSlotsSorted() const
		PURE_VIRTUAL(USaveSystemInterop::GetAllSlotsSorted, return TArray<FString>(); )

	virtual FString GetCurrentSlot() const
		PURE_VIRTUAL(USaveSystemInterop::GetCurrentSlot, return FString(); )

	virtual ITimelinesSaveDataObject* CreateSlot(UObject* WorldContextObject, const FString& SlotName)
	PURE_VIRTUAL(USaveSystemInterop::CreateSlot, return nullptr; )

	// This function's implementation must call OnSaveComplete at some point.
	virtual bool SaveSlot(UObject* WorldContextObject, const ITimelinesSaveDataObject* Slot)
		PURE_VIRTUAL(USaveSystemInterop::SaveSlot, return false; )

	// This function's implementation must call OnLoadComplete at some point.
	virtual bool LoadSlot(UObject* WorldContextObject, const FString& SlotName)
		PURE_VIRTUAL(USaveSystemInterop::WorldContextObject, return false; )

	virtual bool DeleteSlot(const FString& SlotName)
		PURE_VIRTUAL(USaveSystemInterop::DeleteSlot, return false; )

	UFUNCTION()
	virtual void OnSaveComplete();

	UFUNCTION()
	virtual void OnLoadComplete();

	UPROPERTY(BlueprintAssignable, Category = "Save System Interop|Events")
	FSaveSystemInteropEvent OnSaveCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Save System Interop|Events")
	FSaveSystemInteropEvent OnLoadCompleted;
};

DECLARE_LOG_CATEGORY_EXTERN(LogRestorationSubsystem, Log, All)

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRestorationSubsystemEvent);

/**
 * "Restoration" is the global save slot manager that wraps around a backend to provide save slot versioning.
 * Save games are interacted with using the FTimelineGameKey and FTimelinePointKey
 */
UCLASS()
class TIMELINES_API URestorationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

protected:
	TScriptInterface<ITimelinesSaveDataObject> GetTimelineObject(const FTimelinePointKey& Point) const;

public:
	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem")
	FTimelineGameKey GetActiveSaveKey() const { return CurrentTimeline; }

	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem")
	void GetAllMostRecentSlotVersions(TArray<TScriptInterface<ITimelinesSaveDataObject>>& OutSaveData);

	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem")
	void GetMostRecentVersionOfGame(const FTimelineGameKey& GameKey, TScriptInterface<ITimelinesSaveDataObject>& OutSaveData);

	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem")
	void GetAllVersionsOfGame(const FTimelineGameKey& GameKey, TArray<TScriptInterface<ITimelinesSaveDataObject>>& OutSaveData);

	/** Is there a configured game to continue from */
	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem")
	bool CanContinueGame() const;

	/** Get the last played game */
	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem")
	void GetGameToContinue(FTimelineGameKey& GameKey);

	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem", meta = (WorldContext = "WorldContextObject"))
	void SaveGame(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem", meta = (WorldContext = "WorldContextObject"))
	void LoadMostRecentVersionOfGame(UObject* WorldContextObject, const FTimelineGameKey& Key);

	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem", meta = (WorldContext = "WorldContextObject"))
	void LoadVersionOfGame(UObject* WorldContextObject, const FTimelineAnchor& Anchor);

	/** Deletes all save slots for a game */
	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem")
	void DeleteAllVersionsOfGame(const FTimelineGameKey& Key);

	/** Delete one specific point in a timeline */
	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem")
	void DeletePoint(const FTimelinePointKey& Point);

protected:
	UFUNCTION()
	void OnSaveComplete();

	UFUNCTION()
	void OnLoadComplete();

public:
	UPROPERTY(BlueprintAssignable, Category = "Restoration Subsystem|Events")
	FRestorationSubsystemEvent OnSaveCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Restoration Subsystem|Events")
	FRestorationSubsystemEvent OnLoadCompleted;

protected:
	UPROPERTY()
	TArray<FTimelineSaveList> SaveVersionLists;

private:
	UPROPERTY()
	TObjectPtr<USaveSystemInterop> Backend;

	UPROPERTY()
	FTimelineGameKey CurrentTimeline;
};