// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "TimelinesSettings.generated.h"

/**
 *
 */
UCLASS(config = Project, DefaultConfig, meta = (DisplayName = "Timelines"))
class TIMELINES_API UTimelinesSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// This is the interop class that the Restoration Subsystem will use to interact with save files.
	UPROPERTY(config, EditAnywhere, meta = (MetaClass = "SaveSystemInteropBase"), NoClear)
	FSoftClassPath BackendSystemClass;
};
