// Fill out your copyright notice in the Description page of Project Settings.

#include "GameEffect/GameEffectDestroyColumn.h"
#include "Kismet/GameplayStatics.h"
#include "MMPlayerController.h"
#include "MMPlayGrid.h"

/*
*/
UGameEffectDestroyColumn::UGameEffectDestroyColumn()
	: Super()
{
}


void UGameEffectDestroyColumn::SetEffectParams_Implementation(const FGameEffectContext& EffectContext)
{
	if (EffectContext.FloatParams.IsValidIndex(0)) {
		BlockDamage = (int32)EffectContext.FloatParams[0];
	}
}


TArray<FIntPoint> UGameEffectDestroyColumn::GetEffectedCoords_Implementation(const FIntPoint SelectedCoords)
{
	TArray<FIntPoint> AllDestroyCoords;
	AMMPlayerController* PC = Cast<AMMPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (!IsValid(PC)) { return AllDestroyCoords; }
	AMMPlayGrid* Grid = PC->GetCurrentGrid();
	if (!IsValid(Grid)) { return AllDestroyCoords; }
	FIntPoint DestroyCoords;	
	TArray<int32> DestroyedCols;
	AMMBlock* Block = nullptr;
	// Destroy the column for each selected coord
	//for (FIntPoint Coords : SelectedCoords)
	//{
	FIntPoint Coords = SelectedCoords;
	if (!DestroyedCols.Contains(Coords.X))
	{
		DestroyCoords = Coords;
		// Iterate up the column adding them to coords to destroy
		for (int32 Row = 0; Row < Grid->SizeY; Row++)
		{
			DestroyCoords.Y = Row;
			Block = Grid->GetBlock(DestroyCoords);
			if (IsValid(Block) && !Block->IsIndestructible()) {
				AllDestroyCoords.AddUnique(DestroyCoords);
			}
		}
		DestroyedCols.Add(Coords.X);
	}
	//}
	return AllDestroyCoords;
}
