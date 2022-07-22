// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "RestorationSubsystem.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TimelinesStructs.h"
#include "TimelinesStructUtilsLibrary.generated.h"

/**
 *
 */
UCLASS()
class TIMELINES_API UTimelinesStructUtilsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Timelines|Struct Utils", meta = (BlueprintAutocast, CompactNodeTitle = "->"))
	static FString TimelinePointToString(const FTimelinePointKey& PointKey);

	UFUNCTION(BlueprintPure, Category = "Timelines|Struct Utils", meta = (BlueprintAutocast, CompactNodeTitle = "->"))
	static FString TimelineGameToString(const FTimelineGameKey& GameKey);

	UFUNCTION(BlueprintPure, Category = "Timelines|Struct Utils")
	static FTimelineAnchor GetAnchorFromObject(TScriptInterface<ITimelinesSaveDataObject> Object);
};
