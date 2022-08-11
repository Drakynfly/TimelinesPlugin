// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

// ReSharper disable CppExpressionWithoutSideEffects
// ReSharper disable CppMemberFunctionMayBeConst

#include "TimelinesSaveExec.h"
#include "RestorationSubsystem.h"
#include "SaveSystemInteropBase.h"
#include "TimelinesSaveDataObject.h"

void FSaveExecContext::Succeed()
{
	FinishedCallback.ExecuteIfBound(true, Anchor);
}

void FSaveExecContext::Fail(const FString& Reason)
{
	UE_LOG(LogRestorationSubsystem, Error, TEXT("SaveGame failed: %s"), *Reason);
	FinishedCallback.ExecuteIfBound(false, FTimelineAnchor());
}

void UTimelinesSaveExec::Run()
{
}

bool UTimelinesSaveExec::SetupSaveExec(const FSaveExecContext& InContext, const bool RunNow)
{
	if (!IsValid(InContext.WorldContextObject))
	{
		UE_LOG(LogRestorationSubsystem, Warning, TEXT("UTimelinesSaveExec::SetupSaveExec: WorldContextObject is invalid!"));
		return false;
	}

	if (!IsValid(InContext.Backend))
	{
		UE_LOG(LogRestorationSubsystem, Warning, TEXT("UTimelinesSaveExec::SetupSaveExec: Backend is invalid!"));
		return false;
	}

	if (!InContext.Anchor.IsValid())
	{
		UE_LOG(LogRestorationSubsystem, Warning, TEXT("UTimelinesSaveExec::SetupSaveExec: Anchor is invalid!"));
		return false;
	}

	Context = InContext;

	if (RunNow)
	{
		Run();
	}

	return true;
}

void UTimelinesSaveExec_Default::Run()
{
	ITimelinesSaveDataObject* NewSaveData = Context->CreateSlot(Context.WorldContextObject, Context.Anchor.Point.ToString());

	if (NewSaveData == nullptr)
	{
		Context.Fail("'CreateSlot' request to backend failed!");
		return;
	}

	// Setup the save data with our existing meta-save.
	NewSaveData->SetGameKey(Context.Anchor.Game);

	if (!NewSaveData->SetupSlot(Context.WorldContextObject))
	{
		Context.Fail("SetupSlot failed");
		return;
	}

	Context->OnSaveCompletedDelegate.AddUObject(this, &ThisClass::OnSaveComplete);

	if (!Context->SaveSlot(Context.WorldContextObject, NewSaveData))
	{
		Context.Fail("'SaveSlot' request to backend failed!");
		Context->OnSaveCompletedDelegate.RemoveAll(this);
	}
}

void UTimelinesSaveExec_Default::OnSaveComplete()
{
	Context.Succeed();
	Context->OnSaveCompletedDelegate.RemoveAll(this);
}
