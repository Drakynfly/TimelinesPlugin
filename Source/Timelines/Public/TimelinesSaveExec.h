// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "TimelinesStructs.h"
#include "UObject/Object.h"
#include "TimelinesSaveExec.generated.h"

class USaveSystemInteropBase;

struct TIMELINES_API FSaveExecContext
{
	UObject* WorldContextObject;
	USaveSystemInteropBase* Backend;
	FTimelineAnchor Anchor;

	DECLARE_DELEGATE_TwoParams(FSaveExecFinished, bool /* Success */, FTimelineAnchor /* Anchor */)
	FSaveExecFinished FinishedCallback;

	void Succeed();
	void Fail(const FString& Reason);

	USaveSystemInteropBase* operator->() const { return Backend; }
};

/**
 *
 */
UCLASS(Abstract, BlueprintType)
class TIMELINES_API UTimelinesSaveExec : public UObject
{
	GENERATED_BODY()

	friend class URestorationSubsystem;

public:
	virtual void Run();

private:
	bool SetupSaveExec(const FSaveExecContext& InContext, bool RunNow);

protected:
	FSaveExecContext Context;
};

UCLASS()
class UTimelinesSaveExec_Default : public UTimelinesSaveExec
{
	GENERATED_BODY()

public:
	virtual void Run() override;

protected:
	void OnSaveComplete();
};
