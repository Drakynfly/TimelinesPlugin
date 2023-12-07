// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieSaveSlotFragment.h"

#include "TimelinesStructs.generated.h"

USTRUCT()
struct FTimelineKey
{
	GENERATED_BODY()

	FTimelineKey()
	{
		Key = FGuid();
	}

	FTimelineKey(const FGuid& Value)
	{
		Key = Value;
	}

	FTimelineKey(const FString& Value)
	{
		FGuid::ParseExact(Value, EGuidFormats::Short, Key);
	}

protected:
	UPROPERTY()
	FGuid Key;

public:
	friend bool operator==(const FTimelineKey& Lhs, const FTimelineKey& RHS)
	{
		return Lhs.Key == RHS.Key;
	}

	friend bool operator!=(const FTimelineKey& Lhs, const FTimelineKey& RHS)
	{
		return !(Lhs == RHS);
	}

	bool IsValid() const { return Key.IsValid(); }

	static FTimelineKey NewKey()
	{
		return FTimelineKey(FGuid::NewGuid());
	}

	FString ToString() const
	{
		return Key.ToString(EGuidFormats::Short);
	}

	// Try to reconstruct a save key from a string
	static FTimelineKey FromString(const FString& Str)
	{
		FGuid ReconstructedGuid;
		FGuid::ParseExact(Str, EGuidFormats::Short, ReconstructedGuid);
		return FTimelineKey(ReconstructedGuid);
	}

	friend uint32 GetTypeHash(const FTimelineKey& InObject)
	{
		return GetTypeHash(InObject.Key);
	}
};

// A timeline game key is a save slot identifier used by multiple save slots to mark them all as being "versions" of the
// same game. This is how we achieve file versioning of save data.
USTRUCT(BlueprintType)
struct TIMELINES_API FTimelineGameKey : public FTimelineKey
{
	GENERATED_BODY()
};

// ID for a single point in a timeline
USTRUCT(BlueprintType)
struct TIMELINES_API FTimelinePointKey : public FTimelineKey
{
	GENERATED_BODY()
};

/**
 * An identifier for a point to restore the game to.
 */
USTRUCT(BlueprintType)
struct TIMELINES_API FTimelineAnchor : public FFaerieSaveSlotInfoFragment
{
	GENERATED_BODY()

	FTimelineAnchor() {}

	FTimelineAnchor(const FTimelineGameKey Game, const FTimelinePointKey Point)
	  : Game(Game), Point(Point) {}

	UPROPERTY(BlueprintReadOnly, Category = "Timeline Anchor")
	FTimelineGameKey Game;

	UPROPERTY(BlueprintReadOnly, Category = "Timeline Anchor")
	FTimelinePointKey Point;

	bool IsValid() const { return Game.IsValid() && Point.IsValid(); }

	FString ToString() const
	{
		return Game.ToString() + Point.ToString();
	}

	static FTimelineAnchor FromString(const FString& String)
	{
		if (String.Len() != 44)
		{
			return FTimelineAnchor();
		}

		FTimelineAnchor OutAnchor;

		OutAnchor.Game = {String.Left(22)};
		OutAnchor.Point = {String.Right(22)};

		return OutAnchor;
	}
};

struct TIMELINES_API FTimelineSaveList
{
	FTimelineGameKey GameKey;

	// List of save versions. Stored chronologically from end, so the newest is at [0]
	TArray<FTimelinePointKey> Versions;

	FTimelinePointKey MostRecent() const
	{
		if (Versions.IsEmpty())
		{
			return FTimelinePointKey();
		}
		return Versions[0];
	}

	friend bool operator==(const FTimelineSaveList& Lhs, const FTimelineKey& RHS)
	{
		return Lhs.GameKey == RHS;
	}

	friend bool operator==(const FTimelineSaveList& Lhs, const FTimelineSaveList& RHS)
	{
		return Lhs.GameKey == RHS.GameKey;
	}

	friend bool operator!=(const FTimelineSaveList& Lhs, const FTimelineSaveList& RHS)
	{
		return !(Lhs == RHS);
	}
};