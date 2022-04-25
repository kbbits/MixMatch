// Fill out your copyright notice in the Description page of Project Settings.

#include "GameEffect/GameEffectDestroyAllExactMatch.h"
#include "Kismet/GameplayStatics.h"
#include "MMPlayerController.h"
#include "MMPlayGrid.h"

/*
*/
UGameEffectDestroyAllExactMatch::UGameEffectDestroyAllExactMatch()
	: Super()
{
}


void UGameEffectDestroyAllExactMatch::SetEffectParams_Implementation(const FGameEffectContext& EffectContext)
{
	if (EffectContext.FloatParams.IsValidIndex(0)) {
		BlockDamage = FMath::Max((int32)EffectContext.FloatParams[0], 0);
	}
}


TArray<FIntPoint> UGameEffectDestroyAllExactMatch::GetEffectedCoords_Implementation(const FIntPoint SelectedCoords)
{
	TArray<FIntPoint> AllDestroyCoords;
	AMMPlayerController* PC = Cast<AMMPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (!IsValid(PC)) { return AllDestroyCoords; }
	AMMPlayGrid* Grid = PC->GetCurrentGrid();
	if (!IsValid(Grid)) { return AllDestroyCoords; }
	int32 NumCells = Grid->GetCellCount();
	AMMPlayGridCell* Cell = nullptr;
	AMMBlock* Block = nullptr;
	AMMBlock* TmpBlock = nullptr;
	// Destroy all matching blocks for each selected coord
	//for (FIntPoint Coords : SelectedCoords)
	//{
	FIntPoint Coords = SelectedCoords;
	Block = Grid->GetBlock(Coords);
	if (IsValid(Block) && !Block->IsIndestructible())
	{
		AllDestroyCoords.Add(Coords);
		// Scan grid from bottom looking for all matches
		for (int32 i = 0; i < NumCells; i++)
		{
			Cell = Grid->GetCellByNumber(i);
			if (Cell)
			{
				TmpBlock = Cell->CurrentBlock;
				if (IsValid(TmpBlock) &&
					!TmpBlock->IsIndestructible() &&
					!TmpBlock->bFallingIntoGrid &&
					Block->GetBlockType().MatchCode == TmpBlock->GetBlockType().MatchCode)
				{
					AllDestroyCoords.AddUnique(TmpBlock->GetCoords());
				}
			}
		}
	}
	//}
	return AllDestroyCoords;
}

