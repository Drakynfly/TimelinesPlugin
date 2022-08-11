// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "TimelinesSaveDataObject.h"
#include "TimelinesStructs.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RestorationSubsystem.generated.h"

class USaveSystemInteropBase;
class UTimelinesSaveExec;
class UTimelinesLoadExec;

DECLARE_LOG_CATEGORY_EXTERN(LogRestorationSubsystem, Log, All)

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRestorationSubsystemEvent);

enum ERestorationSubsystemState
{
	Unloaded,
	LoadingInProgress,
	Loaded,
	SavingInProgress
};

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

private:
	UTimelinesLoadExec* LoadAnchor_Impl(UObject* WorldContextObject, const FTimelineAnchor& Anchor, bool StallRunning);

public:
	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem")
	FTimelineGameKey GetActiveSaveKey() const { return CurrentTimeline; }

	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem")
	FTimelineAnchor GetAnchor() const { return { CurrentTimeline, CurrentPoint }; }

	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem")
	int32 GetNumTimelines() const { return SaveVersionLists.Num(); }

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
	UTimelinesSaveExec* SaveGame(UObject* WorldContextObject, bool StallRunning = false);

	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem", meta = (WorldContext = "WorldContextObject"))
	UTimelinesLoadExec* LoadMostRecentPointInTimeline(UObject* WorldContextObject, const FTimelineGameKey& Key, bool StallRunning = false);

	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem", meta = (WorldContext = "WorldContextObject"))
	UTimelinesLoadExec* LoadPointFromAnchor(UObject* WorldContextObject, const FTimelineAnchor& Anchor, bool StallRunning = false);

	/** Deletes all save slots for a game */
	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem")
	void DeleteAllVersionsOfGame(const FTimelineGameKey& Key);

	/** Delete one specific point in a timeline */
	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem")
	void DeletePoint(const FTimelinePointKey& Point);

protected:
	void OnSaveExecFinished(bool Success, FTimelineAnchor Anchor);
	void OnLoadExecFinished(bool Success, FTimelineAnchor Anchor);

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
	TObjectPtr<USaveSystemInteropBase> Backend;

	UPROPERTY()
	FTimelineGameKey CurrentTimeline;

	UPROPERTY()
	FTimelinePointKey CurrentPoint;

	TSubclassOf<UTimelinesSaveExec> SaveExecClass;

	TSubclassOf<UTimelinesLoadExec> LoadExecClass;

	ERestorationSubsystemState State;
};
