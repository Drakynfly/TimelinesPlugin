// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "RestorationSubsystem.h"
#include "SaveSystemInteropBase.h"
#include "TimelinesLoadExec.h"
#include "TimelinesSaveExec.h"
#include "TimelinesSettings.h"

DEFINE_LOG_CATEGORY(LogRestorationSubsystem)

void URestorationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogRestorationSubsystem, Log, TEXT("Restoration Subsystem Initialized"));

	const auto InteropClass = GetDefault<UTimelinesSettings>()->BackendSystemClass.TryLoadClass<USaveSystemInteropBase>();

	SaveExecClass = GetDefault<UTimelinesSettings>()->SaveExecClass.TryLoadClass<UTimelinesSaveExec>();
	LoadExecClass = GetDefault<UTimelinesSettings>()->LoadExecClass.TryLoadClass<UTimelinesLoadExec>();

	if (!ensureMsgf(InteropClass,
		TEXT("An interop class must be provided for Restoration Subsystem to function. Please check project settings!")))
	{
		return;
	}

	if (!ensureMsgf(SaveExecClass,
		TEXT("A save exec class must be provided for Restoration Subsystem to function. Please check project settings!")))
	{
		return;
	}

	if (!ensureMsgf(LoadExecClass,
		TEXT("A load exec class must be provided for Restoration Subsystem to function. Please check project settings!")))
	{
		return;
	}

	Backend = NewObject<USaveSystemInteropBase>(this, InteropClass);

	// Cache all existing save slots and collate them into version lists.

	// This returns the slots already ordered by save date. So when we make version lists they will already be in the
	// correct order without us having to do anything.
	TArray<FString> SaveSlots = Backend->GetAllSlotsSorted();

	TMap<FTimelineGameKey, TArray<FString>> SaveInfoMap;

	for (const FString& SaveSlot : SaveSlots)
	{
		const TScriptInterface<ITimelinesSaveDataObject> SaveGame = GetTimelineObject({SaveSlot});

		if (!SaveGame.GetInterface())
		{
			UE_LOG(LogRestorationSubsystem, Log, TEXT("Save slot %s does not cast to ITimelinesSaveDataObject. Ensure correctly configured Timelines settings."), *SaveSlot)
			continue;
		}

		FTimelineGameKey GameKey = SaveGame->GetGameKey();

		if (GameKey.IsValid())
		{
			TArray<FString>& SaveList = SaveInfoMap.FindOrAdd(GameKey);
			SaveList.Add(SaveGame->GetSlotName());
		}
		else
		{
			UE_LOG(LogRestorationSubsystem, Error, TEXT("GameKey is invalid for save slot %s!"), *SaveSlot)
		}
	}

	for (const auto& SlotNameList : SaveInfoMap)
	{
		FTimelineSaveList NewList;
		NewList.GameKey = SlotNameList.Key;

		for (const FString& SlotName : SlotNameList.Value)
		{
			NewList.Versions.Add({SlotName});
		}

		SaveVersionLists.Add(NewList);
	}
}

TScriptInterface<ITimelinesSaveDataObject> URestorationSubsystem::GetTimelineObject(const FTimelinePointKey& Point) const
{
	ITimelinesSaveDataObject* Object = Backend->GetObjectForSlot(Point.ToString());
	return Cast<UObject>(Object);
}

UTimelinesLoadExec* URestorationSubsystem::LoadAnchor_Impl(UObject* WorldContextObject, const FTimelineAnchor& Anchor, const bool StallRunning)
{
	if (State == LoadingInProgress)
	{
		UE_LOG(LogRestorationSubsystem, Warning, TEXT("Cancelling LoadAnchor_Impl: A load is already in progress!"))
		return nullptr;
	}

	if (State == SavingInProgress)
	{
		UE_LOG(LogRestorationSubsystem, Warning, TEXT("Cancelling LoadAnchor_Impl: Cannot load while a save is in progress!"))
		return nullptr;
	}

	State = LoadingInProgress;

	UTimelinesLoadExec* LoadExec = NewObject<UTimelinesLoadExec>(this, LoadExecClass);

	if (!ensureMsgf(LoadExec, TEXT("Cancelling LoadAnchor_Impl: Unable to create SaveExec object!")))
	{
		return nullptr;
	}

	FLoadExecContext Context;
	Context.WorldContextObject = WorldContextObject;
	Context.Backend = Backend;
	Context.Anchor = Anchor;
	Context.FinishedCallback.BindUObject(this, &ThisClass::OnLoadExecFinished);

	if (!LoadExec->SetupLoadExec(Context, !StallRunning))
	{
		UE_LOG(LogRestorationSubsystem, Error, TEXT("Cancelling LoadAnchor_Impl: LoadExec::SetupSaveExec failed!"))
	}

	return LoadExec;
}

void URestorationSubsystem::GetAllMostRecentSlotVersions(TArray<TScriptInterface<ITimelinesSaveDataObject>>& OutSaveData)
{
	OutSaveData.Empty();

	for (const FTimelineSaveList& VersionList : SaveVersionLists)
	{
		if (TScriptInterface<ITimelinesSaveDataObject> FoundSave = GetTimelineObject(VersionList.MostRecent()))
		{
			OutSaveData.Add(FoundSave);
		}
	}
}

void URestorationSubsystem::GetMostRecentVersionOfGame(const FTimelineGameKey& GameKey, TScriptInterface<ITimelinesSaveDataObject>& OutSaveData)
{
	if (const FTimelineSaveList* VersionList = SaveVersionLists.FindByKey(GameKey))
	{
		OutSaveData = GetTimelineObject(VersionList->MostRecent());
	}
}

void URestorationSubsystem::GetAllVersionsOfGame(const FTimelineGameKey& GameKey, TArray<TScriptInterface<ITimelinesSaveDataObject>>& OutSaveData)
{
	OutSaveData.Empty();

	if (!GameKey.IsValid())
	{
		return;
	}

	for (const FTimelineSaveList& VersionList : SaveVersionLists)
	{
		if (VersionList.GameKey == GameKey)
		{
			for (const FTimelinePointKey& Version : VersionList.Versions)
			{
				OutSaveData.Add(GetTimelineObject(Version));
			}
		}
	}
}

bool URestorationSubsystem::CanContinueGame() const
{
	// @todo this is not even remotely going to work with non EMS systems. This only works with EMS because, the current
	// slot is persistent across game sessions. CurrentTimeline should be setup to do this, and be used instead

	if (Backend->GetCurrentSlot().IsEmpty())
	{
		return false;
	}

	for (const FTimelineSaveList& SaveVersionList : SaveVersionLists)
	{
		if (SaveVersionList.MostRecent() == Backend->GetCurrentSlot())
		{
			return true;
		}
	}

	return false;
}

void URestorationSubsystem::GetGameToContinue(FTimelineGameKey& GameKey)
{
	// @todo this is not even remotely going to work with non EMS systems. This only works with EMS because, the current
	// slot is persistent across game sessions. CurrentTimeline should be setup to do this, and be used instead

	if (Backend->GetCurrentSlot().IsEmpty())
	{
		return;
	}

	for (const FTimelineSaveList& SaveVersionList : SaveVersionLists)
	{
		if (SaveVersionList.MostRecent() == Backend->GetCurrentSlot())
		{
			GameKey = SaveVersionList.GameKey;
			return;
		}
	}
}

UTimelinesSaveExec* URestorationSubsystem::SaveGame(UObject* WorldContextObject, const bool StallRunning)
{
	if (!IsValid(WorldContextObject))
	{
		UE_LOG(LogRestorationSubsystem, Error, TEXT("Cancelling SaveGame: Invalid WorldContextObject!"))
		return nullptr;
	}

	if (State == LoadingInProgress)
	{
		UE_LOG(LogRestorationSubsystem, Warning, TEXT("Cancelling SaveGame: Cannot save while a load is in progress!"))
		return nullptr;
	}

	if (State == SavingInProgress)
	{
		UE_LOG(LogRestorationSubsystem, Warning, TEXT("Cancelling SaveGame: A save is already in progress!"))
		return nullptr;
	}

	State = SavingInProgress;

	UTimelinesSaveExec* SaveExec = NewObject<UTimelinesSaveExec>(this, SaveExecClass);

	if (!ensureMsgf(SaveExec, TEXT("Cancelling SaveGame: Unable to create SaveExec object!")))
	{
		return nullptr;
	}

	// Create new timeline point.
	const auto NewPoint = FTimelinePointKey{FTimelinePointKey::NewKey()};

	FSaveExecContext Context;
	Context.WorldContextObject = WorldContextObject;
	Context.Backend = Backend;
	Context.Anchor = {FTimelineGameKey::NewKey(), NewPoint};
	Context.FinishedCallback.BindUObject(this, &ThisClass::OnSaveExecFinished);

	if (!SaveExec->SetupSaveExec(Context, !StallRunning))
	{
		UE_LOG(LogRestorationSubsystem, Error, TEXT("Cancelling SaveGame: SaveExec::SetupSaveExec failed!"))
	}

	return SaveExec;
}

UTimelinesLoadExec* URestorationSubsystem::LoadMostRecentPointInTimeline(UObject* WorldContextObject,
																		 const FTimelineGameKey& Key, bool StallRunning)
{
	if (!IsValid(WorldContextObject))
	{
		UE_LOG(LogRestorationSubsystem, Error, TEXT("Cancelling LoadMostRecentPointInTimeline: Invalid WorldContextObject!"))
		return nullptr;
	}

	if (!Key.IsValid())
	{
		UE_LOG(LogRestorationSubsystem, Error, TEXT("Cancelling LoadPointFromAnchor: Invalid PointToRestore!"))
		return nullptr;
	}

	const auto VersionList = SaveVersionLists.FindByKey(Key);

	if (VersionList == nullptr)
	{
		UE_LOG(LogRestorationSubsystem, Error, TEXT("Cancelling LoadMostRecentPointInTimeline: Invalid Game Key!"))
		return nullptr;
	}

	// Grab the most recent version
	const FTimelinePointKey PointToRestore = VersionList->MostRecent();

	if (!PointToRestore.IsValid())
	{
		UE_LOG(LogRestorationSubsystem, Error, TEXT("Cancelling LoadMostRecentPointInTimeline: Invalid PointToRestore!"))
        return nullptr;
	}

	return LoadAnchor_Impl(WorldContextObject, {Key, PointToRestore}, StallRunning);
}

UTimelinesLoadExec* URestorationSubsystem::LoadPointFromAnchor(UObject* WorldContextObject,
															   const FTimelineAnchor& Anchor, bool StallRunning)
{
	if (!IsValid(WorldContextObject))
	{
		UE_LOG(LogRestorationSubsystem, Error, TEXT("Cancelling LoadPointFromAnchor: Invalid WorldContextObject!"))
		return nullptr;
	}

	if (!Anchor.IsValid())
	{
		UE_LOG(LogRestorationSubsystem, Error, TEXT("Cancelling LoadPointFromAnchor: Invalid Anchor!"))
		return nullptr;
	}

	// Ensure this GameKey and Version pair are valid.
	const int32 VersionIdx = SaveVersionLists.IndexOfByKey(Anchor.Game);
	if (VersionIdx == INDEX_NONE)
	{
		UE_LOG(LogRestorationSubsystem, Error, TEXT("Cancelling LoadPointFromAnchor: Invalid Anchor Game Key!"))
		return nullptr;
	}

	if (!SaveVersionLists[VersionIdx].Versions.Contains(Anchor.Point))
	{
		UE_LOG(LogRestorationSubsystem, Error, TEXT("Cancelling LoadPointFromAnchor: Invalid Anchor Point!"))
		return nullptr;
	}

	return LoadAnchor_Impl(WorldContextObject, Anchor, StallRunning);
}

void URestorationSubsystem::DeleteAllVersionsOfGame(const FTimelineGameKey& Key)
{
	const int32 VersionIdx = SaveVersionLists.IndexOfByKey(Key);

	if (VersionIdx == INDEX_NONE)
	{
		UE_LOG(LogRestorationSubsystem, Error, TEXT("SaveKey was invalid. Cancelling DeleteAllVersionsOfGame!"))
		return;
	}

	for (const FTimelinePointKey& Version : SaveVersionLists[VersionIdx].Versions)
	{
		Backend->DeleteSlot(Version.ToString());
	}
	SaveVersionLists.RemoveAt(VersionIdx);
}

void URestorationSubsystem::DeletePoint(const FTimelinePointKey& Point)
{
	FTimelineSaveList* VersionList = nullptr;

	for (FTimelineSaveList& SaveVersionList : SaveVersionLists)
	{
		for (const FTimelineKey& Version : SaveVersionList.Versions)
		{
			if (Version == Point)
			{
				VersionList = &SaveVersionList;
			}
		}
	}

	if (VersionList == nullptr)
	{
		UE_LOG(LogRestorationSubsystem, Error, TEXT("No save slot found. Cancelling DeletePoint!"))
		return;
	}

	Backend->DeleteSlot(Point.ToString());
	VersionList->Versions.Remove(Point);

	// If we removed the last version, the remove the whole list as nothing exists of this game anymore.
	if (VersionList->Versions.IsEmpty())
	{
		SaveVersionLists.Remove(*VersionList);
	}
}

void URestorationSubsystem::OnSaveExecFinished(const bool Success, const FTimelineAnchor Anchor)
{
	check(State == SavingInProgress);
	check(Anchor.IsValid());

	if (Success)
	{
		// Record this save in the version list, and set it as the currently active save.

		FTimelineSaveList* VersionList;

		// Try to find a VersionList with the CurrentTimeline
		if (CurrentTimeline.IsValid() && CurrentTimeline == Anchor.Game)
		{
			VersionList = SaveVersionLists.FindByKey(Anchor.Game);
		}
		// If the subsystem has no CurrentTimeline, then we have been loaded into a brand new game. Create a list for it.
		else
		{
			CurrentTimeline = Anchor.Game;
			VersionList = &SaveVersionLists.Add_GetRef({CurrentTimeline});
		}

		check(VersionList);

		CurrentPoint = VersionList->Versions.Insert_GetRef(Anchor.Point, 0);

		State = Loaded; // Return the state to Loaded, as the game does now exist connected to a save state.

		OnSaveCompleted.Broadcast();
	}
	else
	{
		if (CurrentPoint.IsValid())
		{
			State = Loaded;
		}
		else
		{
			State = Unloaded;
		}
	}
}

void URestorationSubsystem::OnLoadExecFinished(const bool Success, const FTimelineAnchor Anchor)
{
	check(State == LoadingInProgress);
	check(Anchor.IsValid());

	if (Success)
	{
		CurrentTimeline = Anchor.Game;
		CurrentPoint = Anchor.Point;

		State = Loaded;

		OnLoadCompleted.Broadcast();
	}
	else
	{
		if (CurrentPoint.IsValid())
		{
			State = Loaded;
		}
		else
		{
			State = Unloaded;
		}
	}
}