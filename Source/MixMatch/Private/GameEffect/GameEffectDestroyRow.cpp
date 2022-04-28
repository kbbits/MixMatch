// Fill out your copyright notice in the Description page of Project Settings.

#include "GameEffect/GameEffectDestroyRow.h"
#include "Kismet/GameplayStatics.h"
#include "MMPlayerController.h"
#include "MMPlayGrid.h"

/*
*/
UGameEffectDestroyRow::UGameEffectDestroyRow()
	: Super()
{
}


void UGameEffectDestroyRow::SetEffectParams_Implementation(const FGameEffectContext& EffectContext)
{
	if (EffectContext.FloatParams.IsValidIndex(0)) {
		BlockDamage = (int32)EffectContext.FloatParams[0];
	}
}


TArray<FIntPoint> UGameEffectDestroyRow::GetEffectedCoords_Implementation(const FIntPoint SelectedCoords)
{
	TArray<FIntPoint> AllDestroyCoords;
	AMMPlayerController* PC = Cast<AMMPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (!IsValid(PC)) { return AllDestroyCoords; }
	AMMPlayGrid* Grid = PC->GetCurrentGrid();
	if (!IsValid(Grid)) { return AllDestroyCoords; }
	FIntPoint DestroyCoords;	
	TArray<int32> DestroyedRows;
	AMMBlock* Block = nullptr;
	// Destroy row for each selected coord
	//for (FIntPoint Coords : SelectedCoords)
	//{
	FIntPoint Coords = SelectedCoords;
	if (!DestroyedRows.Contains(Coords.Y))
	{
		DestroyCoords = Coords;
		// Iterate the row adding them to coords to destroy
		for (int32 Col = 0; Col < Grid->SizeX; Col++)
		{
			DestroyCoords.X = Col;
			Block = Grid->GetBlock(DestroyCoords);
			if (IsValid(Block) && !Block->IsIndestructible()) {
				AllDestroyCoords.AddUnique(DestroyCoords);
			}
		}
		DestroyedRows.Add(Coords.Y);
	}
	//}
	return AllDestroyCoords;
}

