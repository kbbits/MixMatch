// Copyright Epic Games, Inc. All Rights Reserved.

#include "MMGameMode.h"
#include "../MixMatch.h"
#include "MMPlayerController.h"
#include "MMPawn.h"
#include "MMPlayGrid.h"
#include "BlockType.h"
#include "MMBlock.h"
#include "Goods/GoodsFunctionLibrary.h"
#include "Goods/GoodsDropper.h"

AMMGameMode::AMMGameMode()
{
	// no pawn by default
	DefaultPawnClass = AMMPawn::StaticClass();
	// use our own player controller class
	PlayerControllerClass = AMMPlayerController::StaticClass();
	//GoodsDropper = CreateDefaultSubobject<UGoodsDropper>(TEXT("GoodsDropper"));
	//if (GoodsDropper == nullptr) {
	//	UE_LOG(LogMMGame, Error, TEXT("MMGameMode constructor - Could not create GoodsDropper."));
	//}

	BlockMoveSpeed = 100;
}


void AMMGameMode::BeginPlay()
{
	InitCachedGoodsTypes();
	InitWeightedBlockTypeSets();
}

FGoodsType AMMGameMode::GetGoodsData(const FName& GoodsName, bool& bFound)
{
	InitCachedGoodsTypes();
	if (CachedGoodsTypes.Contains(GoodsName)) 
	{
		bFound = true;
		return *CachedGoodsTypes.Find(GoodsName);
	}
	bFound = false;
	return FGoodsType();
}

UGoodsDropper* AMMGameMode::GetGoodsDropper()
{
	InitGoodsDropper();
	return GoodsDropper;
}


bool AMMGameMode::GetBlockTypeByName(const FName& BlockTypeName, FBlockType& FoundBlockType)
{
	InitCachedBlockTypes();
	FBlockType* pBlockType = CachedBlockTypes.Find(BlockTypeName);
	if (pBlockType) {
		FoundBlockType = *pBlockType;
		return true;
	}
	else {
		FoundBlockType = FBlockType();
		UE_LOG(LogMMGame, Error, TEXT("MMGameMode::GetBlockTypeByName - Unknown block type: %s"), *BlockTypeName.ToString());
		return false;
	}
}

bool AMMGameMode::GetRandomBlockTypeNameForCell(const AMMPlayGridCell* Cell, FName& FoundBlockTypeName)
{
	InitCachedBlockTypes();
	if (Cell->OwningGrid == nullptr) {
		UE_LOG(LogMMGame, Error, TEXT("GameMode::GetRandomBlockTypeNameForCell - Cell at %s has null grid"), *(Cell->GetCoords()).ToString());
		return false;
	}
	if (!InitWeightedBlockTypes(Cell->OwningGrid->GetBlockTypeSetName())) {
		UE_LOG(LogMMGame, Error, TEXT("GameMode::GetRandomBlockTypeNameForCell - Could not find block type set with name %s"), *Cell->OwningGrid->GetBlockTypeSetName().ToString());
		return false;
	}
	float WeightSum = 0.f;
	float PickedWeight = FMath::RandRange(0.f, CurrentTotalBlockWeight);
	for (FWeightedBlockType WBT : CurrentWeightedBlockTypes)
	{
		WeightSum += WBT.Weight;
		if (WeightSum >= PickedWeight) 
		{
			FBlockType* pBlockType = CachedBlockTypes.Find(WBT.BlockTypeName);
			if (pBlockType) {
				FoundBlockTypeName = pBlockType->Name;
				return true;
			}
			else {
				return false;
			}
		}
	}
	return false;
}


bool AMMGameMode::GetBlockClass(TSubclassOf<AMMBlock>& BlockClass)
{
	BlockClass = AMMBlock::StaticClass();
	return true;
}


bool AMMGameMode::GetGoodsForMatch_Implementation(const FBlockMatch& Match, FGoodsQuantitySet& MatchGoods)
{
	InitGoodsDropper();
	if (!IsValid(GoodsDropper)) {
		UE_LOG(LogMMGame, Error, TEXT("MMGameMode::GetGoodsForMatch - GoodsDropper is not valid"));
		return false;
	}
	MatchGoods.Goods.Empty();
	int32 MatchSize = Match.Blocks.Num();
	if (MatchSize == 0) {
		return false;
	}
	if (!Match.Blocks.IsValidIndex(0) || Match.Blocks[0]->Grid() == nullptr) {
		return false;
	}
	int32 BonusMatchSize = MatchSize - Match.Blocks[0]->Grid()->GetMinimumMatchSize();
	float OverallMult = 0.f;
	TArray<FGoodsQuantity> TempTotalGoods;
	/*
	// Create a map that tracks how many of each block type are in the match
	TMap<FName, FBlockTypeQuantity> BlockTypeMap;
	for (AMMBlock* Block : Match.Blocks)
	{
		FName BlockTypeName = Block->GetBlockType().Name;
		if (BlockTypeMap.Contains(BlockTypeName)) {
			BlockTypeMap[BlockTypeName].Quantity++;
		}
		else {
			BlockTypeMap.Add(BlockTypeName, FBlockTypeQuantity(Block->GetBlockType(), 1));
		}
	}
	// Iterate over the count of block types
	for (const TPair<FName, FBlockTypeQuantity>& It : BlockTypeMap) 
	{
		// Get normal goods for a single block of this type.
		TArray<FGoodsQuantity> NormalGoods = GoodsDropper->EvaluateGoodsDropSet(It.Value.BlockType.MatchDropGoods);
		if (NormalGoods.Num() > 0)
		{
			if (BonusMatchSize > 0)
			{
				// Multiply the goods for this block type by (the quantity of this block type in the match + bonus multiplier based on size of match).
				MatchGoods.Goods = UGoodsFunctionLibrary::AddGoodsQuantities(
					MatchGoods.Goods,
					UGoodsFunctionLibrary::MultiplyGoodsQuantities(
						NormalGoods,
						(float)It.Value.Quantity + (((float)It.Value.Quantity) * (BonusMatchSize * It.Value.BlockType.BonusMatchGoodsMultiplier))
					)					
				);
			}
			else {
				// Multiply the goods for this block type by the quantity of this type in the match.
				MatchGoods.Goods = UGoodsFunctionLibrary::AddGoodsQuantities(
					MatchGoods.Goods,
					UGoodsFunctionLibrary::MultiplyGoodsQuantities(
						NormalGoods,
						((float)It.Value.Quantity)
					)
				);
			}
		}
	}
	*/
	// Iterate over each block, getting dropped goods from each
	for (AMMBlock* Block : Match.Blocks)
	{
		if (BonusMatchSize == 0 || Block->GetBlockType().BonusMatchGoodsMultiplier == 0.f) {
			TempTotalGoods.Append(Block->GetMatchGoods(GoodsDropper));
		}
		else {
			TempTotalGoods.Append(UGoodsFunctionLibrary::MultiplyGoodsQuantities(Block->GetMatchGoods(GoodsDropper), (BonusMatchSize * Block->GetBlockType().BonusMatchGoodsMultiplier) + 1.f));
		}
		if (Block->GetBlockType().OverallMatchGoodsMultiplier > 0.f && Block->GetBlockType().OverallMatchGoodsMultiplier != 1.f) {
			OverallMult += Block->GetBlockType().OverallMatchGoodsMultiplier;
		}
	}
	// After normal goods, including bonus goods, have been determined, apply the overall multipliers from each block type. (if any)
	if (OverallMult > 0.f) {
		MatchGoods.Goods.Append(
			UGoodsFunctionLibrary::MultiplyGoodsQuantities(
				// Use the AddGoodsQuantities function to consolidate all goods quantities to one total per goods type.}
				UGoodsFunctionLibrary::AddGoodsQuantities(TArray<FGoodsQuantity>(), TempTotalGoods), 
				OverallMult)
		);
	}
	else {
		// Use the AddGoodsQuantities function to consolidate all goods quantities to one total per goods type.}
		MatchGoods.Goods.Append(UGoodsFunctionLibrary::AddGoodsQuantities(TArray<FGoodsQuantity>(), TempTotalGoods));
	}		
	return true;
}


int32 AMMGameMode::GetScoreForMatch(const FBlockMatch& Match)
{
	int32 MatchSize = Match.Blocks.Num();
	if (MatchSize == 0) {
		return 0;
	}
	if (!Match.Blocks.IsValidIndex(0) || Match.Blocks[0]->Grid() == nullptr) {
		return 0;
	}
	int32 BonusMatchSize = MatchSize - Match.Blocks[0]->Grid()->GetMinimumMatchSize();
	float OverallMult = 0.f;
	int32 TotalScore = 0;
	for (AMMBlock* Block : Match.Blocks)
	{
		if (BonusMatchSize > 0) {
			TotalScore += Block->GetBlockType().PointsPerBlock + (Block->GetBlockType().PointsPerBlock * (BonusMatchSize * Block->GetBlockType().BonusMatchPointsMultiplier));
		}
		else {
			TotalScore += Block->GetBlockType().PointsPerBlock;
		}
		if (0.f != Block->GetBlockType().OverallMatchPointsMultiplier != 1.f) {
			OverallMult += Block->GetBlockType().OverallMatchPointsMultiplier;
		}
	}
	if (OverallMult > 0.f) {
		TotalScore = TotalScore * OverallMult;
	}
	// Now apply any OverallMatchPointsMultiplier(s)
	return TotalScore;
}


void AMMGameMode::InitCachedBlockTypes(bool bForceRefresh)
{
	if (!(CachedBlockTypes.Num() == 0 || bForceRefresh)) { return; }
	if (!IsValid(BlocksTable)) {
		UE_LOG(LogMMGame, Error, TEXT("AMMGameMode::InitCachedBlockTypes - BlocksTable is not valid"));
		return;
	}
	CachedBlockTypes.Empty(BlocksTable->GetRowMap().Num());
	for (const TPair<FName, uint8*>& It : BlocksTable->GetRowMap())
	{
		FBlockType FoundBlockType = *reinterpret_cast<FBlockType*>(It.Value);
		CachedBlockTypes.Add(It.Key, FoundBlockType);
	}
}


void AMMGameMode::InitWeightedBlockTypeSets(bool bForceRefresh)
{
	if (!(CurrentWeightedBlockTypeSets.Num() == 0 || bForceRefresh)) { return; }
	CurrentWeightedBlockTypeSets.Empty(BlockWeightsTable->GetRowMap().Num());
	CurrentTotalBlockWeight = 0.f;
	for (const TPair<FName, uint8*>& It : BlockWeightsTable->GetRowMap())
	{
		FWeightedBlockTypeSet WBlockTypeSet = *reinterpret_cast<FWeightedBlockTypeSet*>(It.Value);
		if (WBlockTypeSet.WeightedBlockTypes.Num() > 0) {
			CurrentWeightedBlockTypeSets.Add(WBlockTypeSet.Name, WBlockTypeSet);
		}
	}
}


bool AMMGameMode::InitWeightedBlockTypes(const FName& BlockTypeSetName, bool bForceRefresh)
{
	if (BlockTypeSetName == CurrentWeightedBlockTypeSetName && !(CurrentWeightedBlockTypes.Num() == 0 || bForceRefresh)) {
		return true;
	}
	InitWeightedBlockTypeSets();
	CurrentTotalBlockWeight = 0.f;
	CurrentWeightedBlockTypes.Empty();
	if (CurrentWeightedBlockTypeSets.Contains(BlockTypeSetName))
	{		
		CurrentWeightedBlockTypeSetName = BlockTypeSetName;
		CurrentWeightedBlockTypes.Append(CurrentWeightedBlockTypeSets[BlockTypeSetName].WeightedBlockTypes);
		for (FWeightedBlockType WBT : CurrentWeightedBlockTypes)
		{
			if (WBT.Weight > 0.f)			{
				CurrentTotalBlockWeight += WBT.Weight;
			}
		}
		return true;
	}
	else {
		UE_LOG(LogMMGame, Error, TEXT("MMGameMode::InitWeightedBlockTypes - Block type set with name %s not found in weighted block type sets table."), *BlockTypeSetName.ToString());
		return false;
	}
}


void AMMGameMode::InitCachedGoodsTypes(bool bForceRefresh)
{
	if (CachedGoodsTypes.Num() > 0 && !bForceRefresh) { return; }
	CachedGoodsTypes.Empty(GoodsTable->GetRowMap().Num());
	for (const TPair<FName, uint8*>& It : GoodsTable->GetRowMap())
	{
		FGoodsType FoundGoodsType = *reinterpret_cast<FGoodsType*>(It.Value);
		CachedGoodsTypes.Add(It.Key, FoundGoodsType);
	}
}


void AMMGameMode::InitGoodsDropper(bool bForceRefresh)
{
	if (GoodsDropper != nullptr && !bForceRefresh) {
		return;
	}
	if (GoodsDropper == nullptr) {
		GoodsDropper = NewObject<UGoodsDropper>(this, "GoodsDropper");
	}
	if (GoodsDropper != nullptr)
	{
		if (IsValid(GoodsDropperTable))	{
			GoodsDropper->AddDropTableDataToLibrary(GoodsDropperTable);
		}
		else {
			UE_LOG(LogMMGame, Error, TEXT("MMGameMode::InitGoodsDropper - GoodsDropperTable not valid."));
		}
		GoodsDropper->SeedRandomStream(FMath::RandRange(1, INT_MAX));
	}
}