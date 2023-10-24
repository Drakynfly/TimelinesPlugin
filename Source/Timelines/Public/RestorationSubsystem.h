// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "TimelinesStructs.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RestorationSubsystem.generated.h"

class USaveSystemInteropBase;
class UFaerieSaveCommand;
class UFaerieLoadCommand;

DECLARE_LOG_CATEGORY_EXTERN(LogRestorationSubsystem, Log, All)

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTimelineSubsystemPointAdded, const FTimelineAnchor&, Anchor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTimelineSubsystemPointRemoved, const FTimelineGameKey&, Game);

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

public:
	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem")
	FTimelineGameKey GetActiveSaveKey() const { return CurrentAnchor.Game; }

	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem")
	FTimelineAnchor GetAnchor() const { return CurrentAnchor; }

	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem")
	int32 GetNumTimelines() const { return SaveVersionLists.Num(); }

	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem")
	void GetAllTimelines(TArray<FTimelineGameKey>& GameKeys);

	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem")
	void GetMostRecentVersionOfGame(const FTimelineGameKey& GameKey, FTimelineAnchor& Anchor);

	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem")
	void GetAllVersionsOfGame(const FTimelineGameKey& GameKey, TArray<FTimelineAnchor>& Anchors);

	/** Is there a configured game to continue from */
	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem")
	bool CanContinueGame() const;

	/** Get the last played game */
	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem")
	FTimelineGameKey GetGameToContinue() const;

	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem", meta = (WorldContext = "WorldContextObject"))
	UFaerieSaveCommand* SaveGame(bool StallRunning = false);

	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem", meta = (WorldContext = "WorldContextObject"))
	UFaerieLoadCommand* LoadMostRecentPointInTimeline(const FTimelineGameKey& Key, bool StallRunning = false);

	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem", meta = (WorldContext = "WorldContextObject"))
	UFaerieLoadCommand* LoadPointFromAnchor(const FTimelineAnchor& Anchor, bool StallRunning = false);

	/** Deletes all save slots for a game */
	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem")
	void DeleteAllVersionsOfGame(const FTimelineGameKey& Key);

	/** Delete one specific point in a timeline */
	UFUNCTION(BlueprintCallable, Category = "Restoration Subsystem")
	void DeletePoint(const FTimelinePointKey& Point);

protected:
	void OnPreSaveExecRun(FStringView Slot);
	void OnSaveExecFinished(FStringView Slot);
	void OnLoadExecFinished(FStringView Slot);

	// Broadcast when a new point is created on a timeline.
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FTimelineSubsystemPointAdded OnTimelinePointAdded;

	// Broadcast when a new point is created on a timeline.
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FTimelineSubsystemPointRemoved OnTimelinePointRemoved;

	UPROPERTY()
	TArray<FTimelineSaveList> SaveVersionLists;

private:
	UPROPERTY()
	TObjectPtr<USaveSystemInteropBase> Backend;

	TWeakObjectPtr<class UFaerieLocalDataSubsystem> LocalData;

	UPROPERTY()
	FTimelineAnchor CurrentAnchor;
};