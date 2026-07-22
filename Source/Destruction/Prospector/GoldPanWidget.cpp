// Copyright Epic Games, Inc. All Rights Reserved.

#include "GoldPanWidget.h"

void UGoldPanWidget::HandleProgressChanged(float NormalizedJunkRemaining)
{
	OnJunkProgressChanged(NormalizedJunkRemaining);
}

void UGoldPanWidget::HandleMinigameFinished(bool bSuccess, float GoldBankedGrams)
{
	OnMinigameEnded(bSuccess, GoldBankedGrams);
}
