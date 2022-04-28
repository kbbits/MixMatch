// Fill out your copyright notice in the Description page of Project Settings.

#include "GameEffect/GameEffectDestroyBlocksBase.h"
#include "Kismet/GameplayStatics.h"
#include "MixMatch/MixMatch.h"
#include "MMPlayerController.h"
#include "MMPlayGrid.h"

/*
*/
UGameEffectDestroyBlocksBase::UGameEffectDestroyBlocksBase()
	: Super()
{
	NumSelections = 1;
	BlockHandling = EMMBlockHandling::DestroysBlocks;
	static ConstructorHelpers::FClassFinder<AGameEffectPreviewActor> PreviewClassFinder(TEXT("/Game/MixMatch/Blueprints/GameEffects/GameEffectPreviewActor_BP"));
	EffectPreviewClass = PreviewClassFinder.Class;
}


void UGameEffectDestroyBlocksBase::SetEffectParams_Implementation(const FGameEffectContext& EffectContext)
{
	if (EffectContext.FloatParams.IsValidIndex(0)) {
		NumToDestroy = (int32)EffectContext.FloatParams[0];
	}
	if (EffectContext.FloatParams.IsValidIndex(1)) {
		BlockDamage = (int32)EffectContext.FloatParams[1];
	}
}


bool UGameEffectDestroyBlocksBase::CanTrigger_Implementation(const TArray<FIntPoint>& PerformCoords)
{
	AMMPlayerController* PC = Cast<AMMPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (PC == nullptr) { return false; }
	AMMPlayGrid* Grid = PC->GetCurrentGrid();
	if (Grid == nullptr) { return false; }
	AMMPlayGridCell* Cell = nullptr;
	AMMBlock* Block = nullptr;
	if (!RequiresSelection()) {
		return true;
	}
	if (PerformCoords.Num() > 0)
	{
		for (FIntPoint Coord : PerformCoords)
		{
			// Grab block at each coord. Check if is valid and destrucable.
			Block = Grid->GetBlock(Coord);
			if (IsValid(Block) && !Block->IsIndestructible() && !Block->bFallingIntoGrid) {
				// Return on the first valid one we found
				return true;
			}
		}
	}
	else {
		int32 NumCells = Grid->GetCellCount();
		// Scan grid from bottom until we find a block that is not indestructable
		for (int32 i = 0; i < NumCells; i++)
		{
			Cell = Grid->GetCellByNumber(i);
			if (Cell)
			{
				Block = Cell->CurrentBlock;
				if (IsValid(Block) && !Block->IsIndestructible()) {
					return true;
				}
			}
		}
	}
	return false;
}


bool UGameEffectDestroyBlocksBase::BeginEffect_Implementation(const TArray<FIntPoint>& PerformCoords)
{
	AMMPlayerController* PC = Cast<AMMPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (!IsValid(PC)) { return false; }
	AMMPlayGrid* Grid = PC->GetCurrentGrid();
	if (!IsValid(Grid)) { return false; }
	AMMBlock* Block = nullptr;
	int32 NumDestroyed = 0;
	if (NumToDestroy <= 0) {
		UE_LOG(LogMMGame, Warning, TEXT("GameEffectDestroyBlocksBase::BeginEffect NumToDestroy = %d"), NumToDestroy);
	}
	TArray<FIntPoint> EffectedCoords;
	for (FIntPoint Coords : PerformCoords) {
		EffectedCoords.Append(GetEffectedCoords(Coords));
	}
	// Set NumToDestroy = all coords so that all valid blocks get destroyed.
	NumToDestroy = EffectedCoords.Num();
	for (FIntPoint Coord : EffectedCoords)
	{
		// Grab block at each coord. Queue it for destruction if it is valid and destrucable.
		Block = Grid->GetBlock(Coord);
		if (IsValid(Block) && !Block->IsIndestructible())
		{
			if (Block->CanBeDamaged())
			{
				UE_LOG(LogMMGame, Log, TEXT("GameEffectDestroyBlocksBase::BeginEffect damaging block at %s"), *Block->GetCoords().ToString());
				Block->TakeDamage(BlockDamage);
				DestroyedBlockCoords.Add(Coord);
			}
			else 
			{
				UE_LOG(LogMMGame, Log, TEXT("GameEffectDestroyBlocksBase::BeginEffect queueing for destruction %s"), *Block->GetCoords().ToString());
				DestroyedBlockCoords.Add(Coord);
				Grid->BlockDestroyedByGameEffect(Block);
			}
			NumDestroyed++;
		}
		// Once we have destroyed the required amount, break out of loop
		if (NumDestroyed >= NumToDestroy) {
			break;
		}
	}
	// Return results of base class.
	return Super::BeginEffect_Implementation(PerformCoords);
}


//bool UGameEffectDestroyBlocksBase::IncrementTurn_Implementation()
//{
//	return TurnDuration > TurnsInEffect;
//}


//void UGameEffectDestroyBlocksBase::EndEffect_Implementation()
//{
//}