// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "SaveSystemInteropBase.h"

USaveSystemInteropBase::USaveSystemInteropBase()
{
}

void USaveSystemInteropBase::OnSaveComplete()
{
	OnSaveCompleted.Broadcast();
}

void USaveSystemInteropBase::OnLoadComplete()
{
	OnLoadCompleted.Broadcast();
}