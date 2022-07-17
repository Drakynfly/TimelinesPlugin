// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "RestorationSubsystem.h"
#include "SaveSystemInteropBase.h"
#include "TimelinesSettings.h"

DEFINE_LOG_CATEGORY(LogRestorationSubsystem)

void URestorationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogRestorationSubsystem, Log, TEXT("Restoration Subsystem Initialized"));

	const TSubclassOf<USaveSystemInteropBase> InteropClass = GetDefault<UTimelinesSettings>()->BackendSystemClass
															.TryLoadClass<USaveSystemInteropBase>();

	if (!InteropClass)
	{
		UE_LOG(LogRestorationSubsystem, Error,
			TEXT("An interop class must be provided for Restoration Subsystem to function. Please check project settings!"))
		return;
	}

	Backend = NewObject<USaveSystemInteropBase>(this, InteropClass);
	Backend->OnSaveCompleted.AddDynamic(this, &ThisClass::OnSaveComplete);
	Backend->OnLoadCompleted.AddDynamic(this, &ThisClass::OnLoadComplete);

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

void URestorationSubsystem::SaveGame(UObject* WorldContextObject)
{
	if (!IsValid(WorldContextObject))
	{
		UE_LOG(LogRestorationSubsystem, Error, TEXT("Cancelling SaveGame: Invalid WorldContextObject!"))
		return;
	}

	FTimelineSaveList* VersionList = nullptr;

	// If the subsystem has no CurrentTimeline, then we have been loaded into a brand new game. Create a key for it.
	if (!CurrentTimeline.IsValid())
	{
		CurrentTimeline = {FTimelineGameKey::NewKey()};
		VersionList = &SaveVersionLists.Add_GetRef({CurrentTimeline});
	}
	// Otherwise, there should be a VersionList with the CurrentTimeline
	else
	{
		for (FTimelineSaveList& SaveVersionList : SaveVersionLists)
		{
			if (SaveVersionList.GameKey == CurrentTimeline)
			{
				VersionList = &SaveVersionList;
			}
		}
	}

	if (VersionList == nullptr)
	{
		UE_LOG(LogRestorationSubsystem, Error, TEXT("Cancelling SaveGame: Cannot find version list!"))
		return;
	}

	check(CurrentTimeline.IsValid());
	check(VersionList);

	// Create new timeline point.
	CurrentPoint = {FTimelinePointKey::NewKey()};

	if (ITimelinesSaveDataObject* NewSaveData = Backend->CreateSlot(WorldContextObject, CurrentPoint.ToString()))
	{
		// Setup the save data with our existing meta-save.
		NewSaveData->SetGameKey(CurrentTimeline);

		if (!NewSaveData->SetupSlot(WorldContextObject))
		{
			UE_LOG(LogRestorationSubsystem, Error, TEXT("Cancelling SaveGame: SetupSlot failed!"))
			return;
		}

		// Record this in the version list and save.
		VersionList->Versions.Insert(CurrentPoint, 0);
		Backend->SaveSlot(WorldContextObject, NewSaveData);
	}
	else
	{
		UE_LOG(LogRestorationSubsystem, Error, TEXT("Cancelling SaveGame: SetupSlot failed!"))
		CurrentPoint = {FGuid()};
	}
}

void URestorationSubsystem::LoadMostRecentPointInTimeline(UObject* WorldContextObject, const FTimelineGameKey& Key)
{
	if (!IsValid(WorldContextObject))
	{
		UE_LOG(LogRestorationSubsystem, Error, TEXT("Cancelling LoadMostRecentPointInTimeline: Invalid WorldContextObject!"))
		return;
	}

	const int32 VersionIdx = SaveVersionLists.IndexOfByKey(Key);
	if (VersionIdx == INDEX_NONE)
	{
		UE_LOG(LogRestorationSubsystem, Error, TEXT("Cancelling LoadMostRecentPointInTimeline: Invalid Game Key!"))
		return;
	}

	// Grab the most recent version
	const FTimelinePointKey PointToRestore = SaveVersionLists[VersionIdx].MostRecent();

	if (PointToRestore.IsValid())
	{
		UE_LOG(LogRestorationSubsystem, Error, TEXT("Cancelling LoadMostRecentPointInTimeline: Invalid PointToRestore!"))
        return;
	}

	if (Backend->LoadSlot(WorldContextObject, PointToRestore.ToString()))
	{
		CurrentTimeline = Key;
		CurrentPoint = PointToRestore;
	}
}

void URestorationSubsystem::LoadPointFromAnchor(UObject* WorldContextObject, const FTimelineAnchor& Anchor)
{
	if (!IsValid(WorldContextObject))
	{
		UE_LOG(LogRestorationSubsystem, Error, TEXT("Cancelling LoadPointFromAnchor: Invalid WorldContextObject!"))
		return;
	}

	// Ensure this GameKey and Version pair are valid.
	const int32 VersionIdx = SaveVersionLists.IndexOfByKey(Anchor.Game);
	if (VersionIdx == INDEX_NONE)
	{
		UE_LOG(LogRestorationSubsystem, Error, TEXT("Cancelling LoadPointFromAnchor: Invalid Anchor Game Key!"))
		return;
	}

	if (!SaveVersionLists[VersionIdx].Versions.Contains(Anchor.Point))
	{
		UE_LOG(LogRestorationSubsystem, Error, TEXT("Cancelling LoadPointFromAnchor: Invalid Anchor Point!"))
		return;
	}

	if (!Anchor.Point.IsValid())
	{
		UE_LOG(LogRestorationSubsystem, Error, TEXT("Cancelling LoadPointFromAnchor: Invalid PointToRestore!"))
		return;
	}

	if (Backend->LoadSlot(WorldContextObject, Anchor.Point.ToString()))
	{
		CurrentTimeline = Anchor.Game;
		CurrentPoint = Anchor.Point;
	}
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

void URestorationSubsystem::OnSaveComplete()
{
	OnSaveCompleted.Broadcast();
}

void URestorationSubsystem::OnLoadComplete()
{
	OnLoadCompleted.Broadcast();
}