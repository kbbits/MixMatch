// Fill out your copyright notice in the Description page of Project Settings.

#include "GameEffect/GameEffectSpawnBlocksBase.h"
#include "Kismet/GameplayStatics.h"
#include "MixMatch/MixMatch.h"
#include "MMPlayerController.h"
#include "MMPlayGrid.h"

/*
*/
UGameEffectSpawnBlocksBase::UGameEffectSpawnBlocksBase()
	: Super()
{
	NumSelections = 1;
	BlockHandling = EMMBlockHandling::SpawnsBlocks;
}


void UGameEffectSpawnBlocksBase::SetEffectParams_Implementation(const FGameEffectContext& EffectContext)
{
	if (EffectContext.StringParams.Num() > 0) {
		SpawnBlockName = FName(EffectContext.StringParams[0]);
	}
	NumToSpawn = 1;
	if (EffectContext.FloatParams.Num() > 0) {
		NumToSpawn = (int32)EffectContext.FloatParams[0];
	}
}


bool UGameEffectSpawnBlocksBase::CanTrigger_Implementation(const TArray<FIntPoint>& PerformCoords)
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
			// Grab block at each coord. Destroy it if is valid and destrucable.
			Block = Grid->GetBlock(Coord);
			if (Block && !Block->IsIndestructible()) {
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
				if (Block && !Block->IsIndestructible()) {
					return true;
				}
			}
		}
	}
	return false;
}


bool UGameEffectSpawnBlocksBase::BeginEffect_Implementation(const TArray<FIntPoint>& PerformCoords)
{
	AMMPlayerController* PC = Cast<AMMPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (!IsValid(PC)) { return false; }
	AMMPlayGrid* Grid = PC->GetCurrentGrid();
	if (!IsValid(Grid)) { return false; }
	int32 Spawns = 0;
	if (NumToSpawn <= 0) {
		UE_LOG(LogMMGame, Warning, TEXT("GameEffectSpawnBlock::BeginEffect NumToSpawn = %d"), NumToSpawn);
		return false;
	}
	for (FIntPoint SpawnCoords : PerformCoords)
	{
		AMMPlayGridCell* Cell = Grid->GetCell(SpawnCoords);
		if (Cell && Cell->CurrentBlock == nullptr)
		{
			FAddBlockContext BlockContext;
			BlockContext.AddToCell = Cell;
			if (Grid->AddBlockInCell(SpawnBlockName, BlockContext)) {
				Spawns++;
				UE_LOG(LogMMGame, Log, TEXT("GameEffectSpawnBlock::BeginEffect spawned block %d of %d at: %s"), Spawns, NumToSpawn, *SpawnCoords.ToString());
			}
		}
		if (Spawns >= NumToSpawn) {
			break;
		}
	}
	if (Spawns < NumToSpawn) {
		UE_LOG(LogMMGame, Warning, TEXT("GameEffectSpawnBlock::BeginEffect %d spawns requested but only spawned %d"), NumToSpawn, Spawns);
	}
	return Super::BeginEffect_Implementation(PerformCoords);
}


//bool UGameEffectDestroyBlocksBase::IncrementTurn_Implementation()
//{
//	return TurnDuration > TurnsInEffect;
//}


//void UGameEffectDestroyBlocksBase::EndEffect_Implementation()
//{
//}