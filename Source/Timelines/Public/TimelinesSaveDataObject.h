// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "TimelinesStructs.h"
#include "TimelinesSaveDataObject.generated.h"

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