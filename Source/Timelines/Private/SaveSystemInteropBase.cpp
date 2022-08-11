// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "SaveSystemInteropBase.h"

USaveSystemInteropBase::USaveSystemInteropBase()
{
}

void USaveSystemInteropBase::OnSaveComplete()
{
	OnSaveCompletedDelegate.Broadcast();
}

void USaveSystemInteropBase::OnLoadComplete()
{
	OnLoadCompletedDelegate.Broadcast();
}