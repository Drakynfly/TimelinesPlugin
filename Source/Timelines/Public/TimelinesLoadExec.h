// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "TimelinesStructs.h"
#include "UObject/Object.h"
#include "TimelinesLoadExec.generated.h"

class USaveSystemInteropBase;

struct TIMELINES_API FLoadExecContext
{
	UObject* WorldContextObject;
	USaveSystemInteropBase* Backend;
	FTimelineAnchor Anchor;

	using FLoadExecFinished = TDelegate<void(bool, const FTimelineAnchor&)>;
	FLoadExecFinished FinishedCallback;

	void Succeed();
	void Fail(const FString& Reason);

	USaveSystemInteropBase* operator->() const { return Backend; }
};

/**
 *
 */
UCLASS(Abstract, BlueprintType)
class TIMELINES_API UTimelinesLoadExec : public UObject
{
	GENERATED_BODY()

	friend class URestorationSubsystem;

public:
	virtual void Run();

private:
	bool SetupLoadExec(const FLoadExecContext& InContext, bool RunNow);

protected:
	FLoadExecContext Context;
};

UCLASS()
class UTimelinesLoadExec_Default : public UTimelinesLoadExec
{
	GENERATED_BODY()

public:
	virtual void Run() override;

protected:
	void OnLoadComplete();
};