// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "TimelinesSettings.h"

#include "TimelinesLoadExec.h"
#include "TimelinesSaveExec.h"

UTimelinesSettings::UTimelinesSettings()
{
	SaveExecClass = UTimelinesSaveExec_Default::StaticClass();
	LoadExecClass = UTimelinesLoadExec_Default::StaticClass();
}

FName UTimelinesSettings::GetCategoryName() const
{
	return FApp::GetProjectName();
}