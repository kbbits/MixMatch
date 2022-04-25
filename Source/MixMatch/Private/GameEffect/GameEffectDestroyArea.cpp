// Fill out your copyright notice in the Description page of Project Settings.

#include "GameEffect/GameEffectDestroyArea.h"
#include "Kismet/GameplayStatics.h"
#include "MMPlayerController.h"
#include "MMPlayGrid.h"

/*
*/
UGameEffectDestroyArea::UGameEffectDestroyArea()
	: Super()
{
}


void UGameEffectDestroyArea::SetEffectParams_Implementation(const FGameEffectContext& EffectContext)
{
	if (EffectContext.FloatParams.IsValidIndex(0))
	{
		DistanceX = FMath::Max((int32)EffectContext.FloatParams[0], 0);
	}
	if (EffectContext.FloatParams.IsValidIndex(1)) {
		DistanceY = FMath::Max((int32)EffectContext.FloatParams[1], 0);
	}
	if (EffectContext.FloatParams.IsValidIndex(2)) {
		BlockDamage = FMath::Max((int32)EffectContext.FloatParams[2], 0);
	}
}


TArray<FIntPoint> UGameEffectDestroyArea::GetEffectedCoords_Implementation(const FIntPoint SelectedCoords)
{
	TArray<FIntPoint> AllDestroyCoords;
	AMMPlayerController* PC = Cast<AMMPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (!IsValid(PC)) { return AllDestroyCoords; }
	AMMPlayGrid* Grid = PC->GetCurrentGrid();
	if (!IsValid(Grid)) { return AllDestroyCoords; }
	FIntPoint BottomLeft;
	FIntPoint TopRight;
	FIntPoint TmpDestroyCoords;	
	AMMBlock* Block = nullptr;
	// Destroy area for each selected coord
	//for (FIntPoint Coords : SelectedCoords)
	//{
	FIntPoint Coords = SelectedCoords;
	Block = Grid->GetBlock(Coords);
	if (IsValid(Block) && !Block->IsIndestructible()) {
		AllDestroyCoords.AddUnique(Coords);
	}
	BottomLeft = Coords - FIntPoint(DistanceX, DistanceY);
	TopRight = Coords + FIntPoint(DistanceX, DistanceY);
	// Iterate the cols and rows adding the coords to destroy
	for (int32 Col = BottomLeft.X; Col <= TopRight.X; Col++)
	{
		for (int32 Row = BottomLeft.Y; Row <= TopRight.Y; Row++)
		{
			TmpDestroyCoords = FIntPoint(Col, Row);
			Block = Grid->GetBlock(TmpDestroyCoords);
			if (IsValid(Block) && !Block->IsIndestructible()) {
				AllDestroyCoords.AddUnique(TmpDestroyCoords);
			}
		}
	}
	//}
	return AllDestroyCoords;
}


