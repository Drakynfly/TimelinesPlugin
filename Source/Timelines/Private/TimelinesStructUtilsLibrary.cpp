// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "TimelinesStructUtilsLibrary.h"

FString UTimelinesStructUtilsLibrary::TimelinePointToString(const FTimelinePointKey& PointKey)
{
	return PointKey.ToString();
}

FString UTimelinesStructUtilsLibrary::TimelineGameToString(const FTimelineGameKey& GameKey)
{
	return GameKey.ToString();
}