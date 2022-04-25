// Fill out your copyright notice in the Description page of Project Settings.

#include "GameEffect/GameEffectDestroyRowColAligned.h"
#include "Kismet/GameplayStatics.h"
#include "MMPlayerController.h"
#include "MMPlayGrid.h"
#include "GameEffect/GameEffectDestroyRow.h"
#include "GameEffect/GameEffectDestroyColumn.h"

/*
*/
UGameEffectDestroyRowColAligned::UGameEffectDestroyRowColAligned()
	: Super()
{
}


bool UGameEffectDestroyRowColAligned::BeginEffect_Implementation(const TArray<FIntPoint>& PerformCoords)
{
	int32 RowNum = -1;
	EMMOrientation Orientation = EMMOrientation::Horizontal;
	// Check orientation of coords
	for (FIntPoint Coords : PerformCoords)
	{
		if (RowNum < 0) {
			RowNum = Coords.Y;
		}
		else 
		{
			// If we find a Y coord that doesn't match, use vertical orienation
			if (Coords.Y != RowNum)
			{
				Orientation = EMMOrientation::Vertical;
				break;
			}
		}
	}
	// Use Row or Col destroy effects depending on orientation
	UGameEffect* GameEffect = nullptr;
	if (Orientation == EMMOrientation::Vertical) {
		GameEffect = NewObject<UGameEffect>(this->GetOuter(), UGameEffectDestroyColumn::StaticClass());
	}
	else {
		GameEffect = NewObject<UGameEffect>(this->GetOuter(), UGameEffectDestroyRow::StaticClass());
	}
	if (GameEffect)	{
		return GameEffect->BeginEffect(PerformCoords);
	}
	return false;
}
