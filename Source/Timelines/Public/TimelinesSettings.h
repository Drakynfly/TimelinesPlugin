// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "TimelinesSettings.generated.h"

class USaveSystemInteropBase;
class UTimelinesSaveExec;
class UTimelinesLoadExec;

/**
 *
 */
UCLASS(config = Project, DefaultConfig, meta = (DisplayName = "Timelines"))
class TIMELINES_API UTimelinesSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UTimelinesSettings();

	// UDeveloperSettings implementation
	virtual FName GetCategoryName() const override;
	// End UDeveloperSettings implementation

	// This is the interop class that the Restoration Subsystem will use to interact with save files.
	UPROPERTY(config, EditAnywhere, NoClear)
	TSoftClassPtr<USaveSystemInteropBase> BackendSystemClass;

	UPROPERTY(config, EditAnywhere, NoClear)
	TSoftClassPtr<UTimelinesSaveExec> SaveExecClass;

	UPROPERTY(config, EditAnywhere, NoClear)
	TSoftClassPtr<UTimelinesLoadExec> LoadExecClass;
};