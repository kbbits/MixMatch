// Fill out your copyright notice in the Description page of Project Settings.

#include "MatchActionDestroyRow.h"
#include "Kismet/GameplayStatics.h"
#include "MixMatch/MixMatch.h"
#include "MMGameMode.h"
#include "MMBlock.h"
#include "MMPlayGrid.h"


/*
*/
UMatchActionDestroyRow::UMatchActionDestroyRow()
	: Super()
{
}

bool UMatchActionDestroyRow::Perform_Implementation(const UBlockMatch* Match, const FMatchActionType& MatchActionType, const AMMBlock* TriggeringBlock)
{
	Super::Perform_Implementation(Match, MatchActionType, TriggeringBlock);
	check(Match);
	if (Match->Blocks.Num() == 0) { return true; }
	if (MatchActionType.ActionCategory != EMMMatchActionCategory::DestroysBlocks) {
		UE_LOG(LogMMGame, Error, TEXT("MatchActionDestroyRow MatchActionType.ActionCategory should be DestroysBlocks."));
		return false;
	}
	AMMPlayGrid* Grid = Match->Blocks[0]->Grid();
	FIntPoint DestroyCoords;
	AMMBlock* BlockToDestroy = nullptr;
	if (TriggeringBlock != nullptr) {
		DestroyCoords = TriggeringBlock->GetCoords();
	}
	else {
		DestroyCoords = FIntPoint::DivideAndRoundDown(Match->StartCoords + Match->EndCoords, 2);
	}
	if (Grid == nullptr) {
		UE_LOG(LogMMGame, Error, TEXT("MatchActionDestroyRow::Perform - Cannot get grid from match"));
		return false;
	}
	for (int32 Col = 0; Col < Grid->SizeY; Col++)
	{
		DestroyCoords.Y = Col;
		UE_LOG(LogMMGame, Log, TEXT("DestroyCoords: %s"), *DestroyCoords.ToString());
		BlockToDestroy = Grid->GetBlock(DestroyCoords);
		if (IsValid(BlockToDestroy) && !BlockToDestroy->IsMatched()) 
		{
			DestroyedBlockCoords.Add(DestroyCoords);
			Grid->BlockDestroyedByMatchAction(BlockToDestroy);			
		}
	}
	return true;
}