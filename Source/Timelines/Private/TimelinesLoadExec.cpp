// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

// ReSharper disable CppExpressionWithoutSideEffects
// ReSharper disable CppMemberFunctionMayBeConst

#include "TimelinesLoadExec.h"
#include "RestorationSubsystem.h"
#include "SaveSystemInteropBase.h"

void FLoadExecContext::Succeed()
{
	FinishedCallback.ExecuteIfBound(true, Anchor);
}

void FLoadExecContext::Fail(const FString& Reason)
{
	UE_LOG(LogRestorationSubsystem, Error, TEXT("LoadGame failed: %s"), *Reason);
	FinishedCallback.ExecuteIfBound(false, FTimelineAnchor());
}

void UTimelinesLoadExec::Run()
{
}

bool UTimelinesLoadExec::SetupLoadExec(const FLoadExecContext& InContext, const bool RunNow)
{
	if (!IsValid(InContext.WorldContextObject))
	{
		UE_LOG(LogRestorationSubsystem, Warning, TEXT("UTimelinesSaveExec::RunChecked: WorldContextObject is invalid!"));
		return false;
	}

	if (!IsValid(InContext.Backend))
	{
		UE_LOG(LogRestorationSubsystem, Warning, TEXT("UTimelinesSaveExec::RunChecked: Backend is invalid!"));
		return false;
	}

	if (!InContext.Anchor.IsValid())
	{
		UE_LOG(LogRestorationSubsystem, Warning, TEXT("UTimelinesSaveExec::RunChecked: Anchor is invalid!"));
		return false;
	}

	Context = InContext;

	if (RunNow)
	{
		Run();
	}

	return true;
}

void UTimelinesLoadExec_Default::Run()
{
	Context->OnLoadCompletedDelegate.AddUObject(this, &ThisClass::OnLoadComplete);

	if (!Context->LoadSlot(Context.WorldContextObject, Context.Anchor.Point.ToString()))
	{
		Context.Fail("'LoadSlot' request to backend failed!");
		Context->OnLoadCompletedDelegate.RemoveAll(this);
	}
}

void UTimelinesLoadExec_Default::OnLoadComplete()
{
	Context.Succeed();
	Context->OnLoadCompletedDelegate.RemoveAll(this);
}
