// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "RestorationSubsystem.h"
#include "SaveSystemInteropBase.h"

DEFINE_LOG_CATEGORY(LogRestorationSubsystem)

void URestorationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	auto&& LocalData = Collection.InitializeDependency<UFaerieLocalDataSubsystem>();
	LocalData->SetOnServiceInit(Faerie::SaveData::DefaultService,
		UFaerieLocalDataSubsystem::FOnSubsystemInit::CreateWeakLambda(this,
			[this](USaveSystemInteropBase* Service)
			{
				Backend = Service;
				Backend->GetOnPreSlotSavedEvent().AddUObject(this, &ThisClass::OnPreSaveExecRun);
				Backend->GetOnSaveCompletedEvent().AddUObject(this, &ThisClass::OnSaveExecFinished);
				Backend->GetOnLoadCompletedEvent().AddUObject(this, &ThisClass::OnLoadExecFinished);

				// Cache all existing save slots and collate them into version lists.

				// This returns the slots already ordered by save date. So when we make version lists they will already be in the
				// correct order without us having to do anything.
				TArray<FString> SaveSlots = Backend->GetAllSlotsSorted();

				TMap<FTimelineGameKey, TArray<FString>> SaveInfoMap;

				for (const FString& SaveSlot : SaveSlots)
				{
					auto&& Anchor = Backend->GetFragmentData<FTimelineAnchor>(SaveSlot);
					if (!Anchor.IsValid())
					{
						continue;
					}

					if (const FTimelineGameKey GameKey = Anchor.Get<const FTimelineAnchor>().Game;
						GameKey.IsValid())
					{
						TArray<FString>& SaveList = SaveInfoMap.FindOrAdd(GameKey);
						SaveList.Add(SaveSlot);
					}
					else
					{
						UE_LOG(LogRestorationSubsystem, Error, TEXT("GameKey is invalid for save slot %s!"), *SaveSlot)
					}
				}

				for (auto&& SlotNameList : SaveInfoMap)
				{
					FTimelineSaveList NewList;
					NewList.GameKey = SlotNameList.Key;

					for (const FString& SlotName : SlotNameList.Value)
					{
						NewList.Versions.Add({SlotName});
					}

					SaveVersionLists.Add(NewList);
				}
			}));

	UE_LOG(LogRestorationSubsystem, Log, TEXT("Restoration Subsystem Initialized"));
}

void URestorationSubsystem::GetAllTimelines(TArray<FTimelineGameKey>& GameKeys)
{
	GameKeys.Empty();

	for (const FTimelineSaveList& VersionList : SaveVersionLists)
	{
		GameKeys.Add(VersionList.GameKey);
	}
}

void URestorationSubsystem::GetMostRecentVersionOfGame(const FTimelineGameKey& GameKey, FTimelineAnchor& Anchor)
{
	if (const FTimelineSaveList* VersionList = SaveVersionLists.FindByKey(GameKey))
	{
		Anchor.Game = GameKey;
		Anchor.Point = VersionList->MostRecent();
	}
}

void URestorationSubsystem::GetAllVersionsOfGame(const FTimelineGameKey& GameKey, TArray<FTimelineAnchor>& Anchors)
{
	Anchors.Empty();

	if (!GameKey.IsValid())
	{
		return;
	}

	for (const FTimelineSaveList& VersionList : SaveVersionLists)
	{
		if (VersionList.GameKey == GameKey)
		{
			for (auto Point : VersionList.Versions)
			{
				Anchors.Add({GameKey, Point});
			}
			break;
		}
	}
}

bool URestorationSubsystem::CanContinueGame() const
{
	return GetGameToContinue().IsValid();
}

FTimelineGameKey URestorationSubsystem::GetGameToContinue() const
{
	auto&& AllSlots = Backend->GetAllSlotsSorted();
	if (AllSlots.IsEmpty())
	{
		return FTimelineGameKey();
	}

	// The sorted array should have slot 0 be the most recent.
	const FString SlotName = AllSlots[0];
	if (SlotName.IsEmpty())
	{
		return FTimelineGameKey();
	}

	for (const FTimelineSaveList& SaveVersionList : SaveVersionLists)
	{
		if (SaveVersionList.MostRecent() == SlotName)
		{
			return SaveVersionList.GameKey;
		}
	}

	return FTimelineGameKey();
}

UFaerieSaveCommand* URestorationSubsystem::SaveGame(const bool StallRunning)
{
	// Save the game to a new slot
	return Backend->CreateSaveCommand(FTimelineKey::NewKey().ToString(), StallRunning);
}

UFaerieLoadCommand* URestorationSubsystem::LoadMostRecentPointInTimeline(const FTimelineGameKey& Key, const bool StallRunning)
{
	if (!Key.IsValid())
	{
		UE_LOG(LogRestorationSubsystem, Error, TEXT("Cancelling LoadPointFromAnchor: Invalid PointToRestore!"))
		return nullptr;
	}

	auto&& VersionList = SaveVersionLists.FindByKey(Key);

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

	return Backend->CreateLoadCommand(PointToRestore.ToString(), StallRunning);
}

UFaerieLoadCommand* URestorationSubsystem::LoadPointFromAnchor(const FTimelineAnchor& Anchor, const bool StallRunning)
{
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

	return Backend->CreateLoadCommand(Anchor.Point.ToString(), StallRunning);
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
		Backend->DeleteSlot(Version.ToString(), nullptr);
	}

	SaveVersionLists.RemoveAt(VersionIdx);

	OnTimelinePointRemoved.Broadcast(Key);
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

	Backend->DeleteSlot(Point.ToString(), nullptr);
	VersionList->Versions.Remove(Point);

	// If we removed the last version, the remove the whole list as nothing exists of this game anymore.
	if (VersionList->Versions.IsEmpty())
	{
		SaveVersionLists.Remove(*VersionList);
	}

	OnTimelinePointRemoved.Broadcast(VersionList->GameKey);
}

void URestorationSubsystem::OnPreSaveExecRun(const FStringView Slot)
{
	FTimelineAnchor NewAnchor;
	NewAnchor.Point = FTimelinePointKey(FString(Slot));

	if (CurrentAnchor.IsValid())
	{
		NewAnchor.Game = CurrentAnchor.Game;
	}
	else
	{
		NewAnchor.Game = FTimelineGameKey{FTimelineKey::NewKey()};
	}

	Backend->EditFragmentData<FTimelineAnchor>(Slot,
		[NewAnchor](FTimelineAnchor& Anchor)
		{
			Anchor = NewAnchor;
		});
}

void URestorationSubsystem::OnSaveExecFinished(const FStringView Slot)
{
	auto&& AnchorFragment = Backend->GetFragmentData<FTimelineAnchor>(Slot);
	if (!AnchorFragment.IsValid())
	{
		Backend->GetOnErrorEvent().Broadcast(TEXTVIEW("Anchor not found for slot!"));
		return;
	}

	const FTimelineAnchor& Anchor = AnchorFragment.Get<const FTimelineAnchor>();

	// Record this save in the version list, and set it as the currently active save.

	FTimelineSaveList* VersionList;

	// Try to find a VersionList with the CurrentTimeline
	if (CurrentAnchor.IsValid() && CurrentAnchor.Game == Anchor.Game)
	{
		VersionList = SaveVersionLists.FindByKey(Anchor.Game);
	}
	// If the subsystem has no CurrentTimeline, then we have been loaded into a brand new game. Create a list for it.
	else
	{
		CurrentAnchor.Game = Anchor.Game;
		VersionList = &SaveVersionLists.Add_GetRef({CurrentAnchor.Game});
	}

	check(VersionList);

	CurrentAnchor.Point = VersionList->Versions.Insert_GetRef(Anchor.Point, 0);

	OnTimelinePointAdded.Broadcast(CurrentAnchor);
}

void URestorationSubsystem::OnLoadExecFinished(const FStringView Slot)
{
	auto&& AnchorFragment = Backend->GetFragmentData<FTimelineAnchor>(Slot);
	if (!AnchorFragment.IsValid())
	{
		return;
	}

	// The loaded game is our new anchor.
	CurrentAnchor = AnchorFragment.Get<const FTimelineAnchor>();
}