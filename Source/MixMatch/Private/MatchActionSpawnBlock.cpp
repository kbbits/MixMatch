// Fill out your copyright notice in the Description page of Project Settings.

#include "MatchActionSpawnBlock.h"
#include "Kismet/GameplayStatics.h"
#include "../MixMatch.h"
#include "MMGameMode.h"
#include "MMBlock.h"
#include "MMPlayGrid.h"


/*
*/
UMatchActionSpawnBlock::UMatchActionSpawnBlock()
	: Super()
{
	LocationType = EMMBlockLocation::InMatch;
}


bool UMatchActionSpawnBlock::Perform_Implementation(const UBlockMatch* Match, const FMatchActionType& MatchActionType)
{
	Super::Perform_Implementation(Match, MatchActionType);
	check(Match);
	if (Match->Blocks.Num() == 0) { return true; }
	AMMPlayGrid* Grid = Match->Blocks[0]->Grid();
	FIntPoint SpawnCoords = FIntPoint::NoneValue;
	FIntPoint SpawnOffset = FIntPoint(0,0);
	FIntPoint MatchMiddleCoords = FIntPoint::DivideAndRoundDown(Match->StartCoords + Match->EndCoords, 2);
	int32 SpawnBatches = 0;
	int32 BatchTries = 0;
	if (Grid == nullptr) {
		UE_LOG(LogMMGame, Error, TEXT("MatchActionSpawnBlock::Perform - Cannot get grid from match"));
		return false;
	}
	switch(MatchActionType.ActionQuantityType)
	{
		case EMMBlockQuantity::PerMatch :
			SpawnBatches = 1;
			break;
		case EMMBlockQuantity::PerBlock :
			SpawnBatches = Match->Blocks.Num();
			break;
		case EMMBlockQuantity::PerBlockOverMin :
			SpawnBatches = Match->Blocks.Num() - Grid->GetMinimumMatchSize();
			break;
	}
	for (int32 BatchNum = 1; BatchNum <= SpawnBatches; BatchNum++) 
	{
		BatchTries = 0;
		for (int32 i = 0; i < MatchActionType.Quantity && BatchTries < Match->Blocks.Num(); i++)
		{
			BatchTries++;
			UE_LOG(LogMMGame, Log, TEXT("SpawnBlock Batch: %d  i: %d"), BatchNum, i);
			UE_LOG(LogMMGame, Log, TEXT("  MiddleCoord: %s"), *MatchMiddleCoords.ToString());
			UE_LOG(LogMMGame, Log, TEXT("  SpawnOffset: %s"), *SpawnOffset.ToString());
			bool bFoundMoved = false;
			switch (LocationType) {
				case EMMBlockLocation::OnMovedBlock:
					for (AMMBlock* Block : Match->Blocks)
					{
						if (Block->bMovedByPlayer) 
						{
							SpawnCoords = Block->GetCoords();
							bFoundMoved = true;
							break;
						}
					}
					if (bFoundMoved) {
						break;
					}
				case EMMBlockLocation::InMatch:
					SpawnCoords = MatchMiddleCoords + SpawnOffset;					
					//SpawnOffset += Match.Orientation == EMMOrientation::Horizontal ? SpawnOffset + FIntPoint(1, 0) : SpawnOffset + FIntPoint(0, 1);
					break;
				case EMMBlockLocation::TopRow:
					SpawnCoords = ((MatchMiddleCoords + SpawnOffset) * FIntPoint(1, 0)) + FIntPoint(0, Grid->SizeY - 1);
					//SpawnCoords = FIntPoint(i + SpawnOffset.X, Grid->SizeY - 1);
					//SpawnOffset += FIntPoint(1, 0);
					break;
					
			}
			if (Match->Orientation == EMMOrientation::Horizontal) 
			{
				FIntPoint TmpCoord = FIntPoint((i + BatchNum) % 2, 0);
				SpawnOffset = (SpawnOffset * FIntPoint(-1, -1)) + TmpCoord;
				UE_LOG(LogMMGame, Log, TEXT("  TmpCoord: %s"), *TmpCoord.ToString());
				UE_LOG(LogMMGame, Log, TEXT("  Next SpawnOffset: %s"), *SpawnOffset.ToString());
			}
			else 
			{
				FIntPoint TmpCoord = FIntPoint(0, (i + BatchNum) % 2);
				SpawnOffset = (SpawnOffset * FIntPoint(-1, -1)) + TmpCoord;
				UE_LOG(LogMMGame, Log, TEXT("  TmpCoord: %s"), *TmpCoord.ToString());
				UE_LOG(LogMMGame, Log, TEXT("  Next SpawnOffset: %s"), *SpawnOffset.ToString());
			}
			AMMPlayGridCell* SpawnCell = Grid->GetCell(SpawnCoords);
			if (SpawnCell && SpawnCell->CurrentBlock == nullptr) {
				FAddBlockContext BlockContext;
				BlockContext.AddToCell = SpawnCell;
				UE_LOG(LogMMGame, Log, TEXT("  MatchActionSpawnBlock::Perform spawning block at: %s"), *SpawnCoords.ToString());
				Grid->AddBlockInCell(FName(MatchActionType.ActionParam), BlockContext);
			}
			else {
				UE_LOG(LogMMGame, Error, TEXT("MatchActionSpawnBlock::Perform - no empty cell at %s"), *SpawnCoords.ToString());
				i--;
			}
		}
	}
	return true;
}