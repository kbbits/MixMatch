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


bool UMatchActionSpawnBlock::Perform_Implementation(const UBlockMatch* Match, const FMatchActionType& MatchActionType, const AMMBlock* TriggeringBlock)
{
	Super::Perform_Implementation(Match, MatchActionType, TriggeringBlock);
	check(Match);
	if (Match->Blocks.Num() == 0) { return true; }
	if (MatchActionType.ActionCategory != EMMMatchActionCategory::SpawnsBlocks) {
		UE_LOG(LogMMGame, Error, TEXT("MatchActionSpawnBlock MatchActionType.ActionCategory should be SpawnsBlocks."));
		return false;
	}
	AMMPlayGrid* Grid = Match->Blocks[0]->Grid();
	int32 Spawns = 0;
	FIntPoint SpawnCoords = FIntPoint::NoneValue;
	FIntPoint SpawnOffset = FIntPoint(0,0);
	FIntPoint MatchMiddleCoords = FIntPoint::DivideAndRoundDown(Match->StartCoords + Match->EndCoords, 2);
	if (Grid == nullptr) {
		UE_LOG(LogMMGame, Error, TEXT("MatchActionSpawnBlock::Perform - Cannot get grid from match"));
		return false;
	}
	for (int32 i = 0; Spawns < MatchActionType.Quantity && i < Match->Blocks.Num(); i++)
	{
		UE_LOG(LogMMGame, Log, TEXT("SpawnBlock i: %d"), i);
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
				break;
			case EMMBlockLocation::TopRow:
				SpawnCoords = ((MatchMiddleCoords + SpawnOffset) * FIntPoint(1, 0)) + FIntPoint(0, Grid->SizeY - 1);
				break;
		}
		if (Match->Orientation == EMMOrientation::Horizontal) 
		{
			FIntPoint TmpCoord = FIntPoint(i % 2, 0);
			SpawnOffset = (SpawnOffset * FIntPoint(-1, -1)) + TmpCoord;
			UE_LOG(LogMMGame, Log, TEXT("  TmpCoord: %s"), *TmpCoord.ToString());
			UE_LOG(LogMMGame, Log, TEXT("  Next SpawnOffset: %s"), *SpawnOffset.ToString());
		}
		else 
		{
			FIntPoint TmpCoord = FIntPoint(0, i % 2);
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
			Spawns++;
		}
		else {
			UE_LOG(LogMMGame, Error, TEXT("MatchActionSpawnBlock::Perform - no empty cell at %s"), *SpawnCoords.ToString());
		}
	}
	return true;
}