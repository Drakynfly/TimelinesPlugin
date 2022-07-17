// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TimelinesSaveDataObject.h"
#include "TimelinesStructs.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RestorationSubsystem.generated.h"

class USaveSystemInteropBase;

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
	FTimelineAnchor GetAnchor() { return { CurrentTimeline, CurrentPoint }; }

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
	void LoadMostRecentPointInTimeline(UObject* WorldContextObject, const FTimelineGameKey& Key);

	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem", meta = (WorldContext = "WorldContextObject"))
	void LoadPointFromAnchor(UObject* WorldContextObject, const FTimelineAnchor& Anchor);

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
	TObjectPtr<USaveSystemInteropBase> Backend;

	UPROPERTY()
	FTimelineGameKey CurrentTimeline;

	UPROPERTY()
	FTimelinePointKey CurrentPoint;
};