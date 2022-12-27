// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

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
	UTimelinesSettings();

	// This is the interop class that the Restoration Subsystem will use to interact with save files.
	UPROPERTY(config, EditAnywhere, meta = (MetaClass = "/Script/Timelines.SaveSystemInteropBase"), NoClear)
	FSoftClassPath BackendSystemClass;

	UPROPERTY(config, EditAnywhere, meta = (MetaClass = "/Script/Timelines.TimelinesSaveExec"), NoClear)
	FSoftClassPath SaveExecClass;

	UPROPERTY(config, EditAnywhere, meta = (MetaClass = "/Script/Timelines.TimelinesLoadExec"), NoClear)
	FSoftClassPath LoadExecClass;
};
