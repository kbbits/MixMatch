// Copyright Epic Games, Inc. All Rights Reserved.

#include "MMGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"
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
	DefaultPawnClass = AMMPawn::StaticClass();
	// use our own player controller class
	PlayerControllerClass = AMMPlayerController::StaticClass();
	
	//Defaults
	ExperienceTierMultiplier = 1.1f;
	ValueTierMultiplier = 1.5f;
	DefaultBlockMoveSpeed = 300;
}


void AMMGameMode::BeginPlay()
{
	Super::BeginPlay();
	AssetGroupsPendingLoad.Add(FName(TEXT("Goods")));
	AssetGroupsPendingLoad.Add(FName(TEXT("Blocks")));
	InitCachedGoodsTypes();
	InitCachedBlockTypes();
	InitWeightedBlockTypeSets();
	TArray<AActor*> AllGrids;
	AMMPlayerController* PC = Cast<AMMPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	check(PC);
	if (PC) 
	{
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMMPlayGrid::StaticClass(), AllGrids);
		if (AllGrids.Num() > 0)	{
			PC->SetCurrentGrid(Cast<AMMPlayGrid>(AllGrids[0]));
		}
	}
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


bool AMMGameMode::GetRandomBlockTypeNameForCell(FName& FoundBlockTypeName, const FAddBlockContext& BlockContext)
{
	InitCachedBlockTypes();
	AMMPlayGridCell* Cell = BlockContext.AddToCell;
	check(Cell);
	if (Cell->OwningGrid == nullptr) {
		UE_LOG(LogMMGame, Error, TEXT("GameMode::GetRandomBlockTypeNameForCell - Cell at %s has null grid"), *(Cell->GetCoords()).ToString());
		return false;
	}
	if (!InitWeightedBlockTypes(Cell->OwningGrid->GetBlockTypeSetName())) {
		UE_LOG(LogMMGame, Error, TEXT("GameMode::GetRandomBlockTypeNameForCell - Could not find block type set with name %s"), *Cell->OwningGrid->GetBlockTypeSetName().ToString());
		return false;
	}
	bool bUseExclusionList = BlockContext.ExcludedBlockNames.Num() > 0;
	bool bPreventImmobile = BlockContext.OffsetAboveTopCell > 0.f || Cell->GetCoords().Y >= (Cell->OwningGrid->SizeY - 2);
	float WeightSum = 0.f;
	float PickedWeight = 0.f;
	FName PickedBlockTypeName = NAME_None;
	TArray<FWeightedBlockType> SelectedWeightedBlockTypes;
	TArray<FWeightedBlockType> FallbackWeightedBlockTypes;
	if (bUseExclusionList || bPreventImmobile)
	{
		float SpecialTotalBlockWeight = 0.f;
		for (FWeightedBlockType WBT : CurrentWeightedBlockTypes) 
		{
			// For blocks dropping into grid
			if (bPreventImmobile)
			{
				FBlockType TmpBlockType;
				if (GetBlockTypeByName(WBT.BlockTypeName, TmpBlockType)) {
					// Don't allow block types that are immobile
					if (TmpBlockType.bImmobile) {
						continue;
					}
				}
			}
			FallbackWeightedBlockTypes.Add(WBT);
			if (!BlockContext.ExcludedBlockNames.Contains(WBT.BlockTypeName)) 
			{
				SpecialTotalBlockWeight += WBT.Weight;
				SelectedWeightedBlockTypes.Add(WBT);
			}
		}
		if (SelectedWeightedBlockTypes.Num() == 0) 
		{
			// If we ended up with no block types allowed, then pick a random one to add
			int32 RandomIndex = FMath::RandRange(0, FallbackWeightedBlockTypes.Num() - 1);
			SelectedWeightedBlockTypes.Add(FallbackWeightedBlockTypes[RandomIndex]);
			SpecialTotalBlockWeight = FallbackWeightedBlockTypes[RandomIndex].Weight;
		}
		if (SelectedWeightedBlockTypes.Num() == 1) {
			// If there is only one block type, just use that type
			PickedBlockTypeName = SelectedWeightedBlockTypes[0].BlockTypeName;
		}
		else {
			PickedWeight = FMath::RandRange(0.f, SpecialTotalBlockWeight);
		}
	}
	else {
		PickedWeight = FMath::RandRange(0.f, CurrentTotalBlockWeight);
		SelectedWeightedBlockTypes = CurrentWeightedBlockTypes;
	}
	if (PickedBlockTypeName == NAME_None) 
	{
		// Haven't found a picked name yet, so find one in weighted list
		for (FWeightedBlockType WBT : SelectedWeightedBlockTypes)
		{
			WeightSum += WBT.Weight;
			if (WeightSum >= PickedWeight)
			{
				PickedBlockTypeName = WBT.BlockTypeName;
				break;
			}
		}
		if (PickedBlockTypeName == NAME_None) 
		{
			UE_LOG(LogMMGame, Error, TEXT("MMGameMode::GetRandomBlockTypeNameForCell - No weighted block type found in BlockTypeSet %s"), *CurrentWeightedBlockTypeSetName.ToString());
			return false;
		}
	}
	// Grab the actual block type from our block type name map to make sure the picked name is in our map. (should always be found)
	FBlockType* pBlockType = CachedBlockTypes.Find(PickedBlockTypeName);
	if (pBlockType) 
	{
		FoundBlockTypeName = pBlockType->Name;
		//UE_LOG(LogMMGame, Log, TEXT("MMGameMode::GetRandomBlockTypeNameForCell - Got block type %s from BlockTypeSet %s"), *FoundBlockTypeName.ToString(), *CurrentWeightedBlockTypeSetName.ToString());
		return true;
	}
	else 
	{
		UE_LOG(LogMMGame, Error, TEXT("MMGameMode::GetRandomBlockTypeNameForCell - Block type %s not found in BlockTypeSet %s"), *PickedBlockTypeName.ToString(), *CurrentWeightedBlockTypeSetName.ToString());
		return false;
	}
}


bool AMMGameMode::GetBlockClass(TSubclassOf<AMMBlock>& BlockClass)
{
	BlockClass = AMMBlock::StaticClass();
	return true;
}


float AMMGameMode::GetBlockMoveSpeed()
{
	AMMPlayerController* PC = Cast<AMMPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	check(PC);
	if (PC) {
		return DefaultBlockMoveSpeed * PC->GetBlockMoveSpeedMultiplier();
	}
	return DefaultBlockMoveSpeed;
}


bool AMMGameMode::GetGoodsForMatch_Implementation(const UBlockMatch* Match, FGoodsQuantitySet& MatchGoods)
{
	check(Match);
	MatchGoods.Goods.Empty();
	// TotalGoods is set in MMPlayGrid::ResolveMatches
	if (Match->TotalGoods.Num() > 0) 
	{
		MatchGoods.Goods.Append(Match->TotalGoods);
		return true;
	}
	InitGoodsDropper();
	if (!IsValid(GoodsDropper)) 
	{
		UE_LOG(LogMMGame, Error, TEXT("MMGameMode::GetGoodsForMatch - GoodsDropper is not valid"));
		return false;
	}	
	int32 MatchSize = Match->Blocks.Num();
	if (MatchSize == 0) {
		return false;
	}
	if (!Match->Blocks.IsValidIndex(0) || Match->Blocks[0]->Grid() == nullptr) {
		return false;
	}
	int32 BonusMatchSize = MatchSize - Match->Blocks[0]->Grid()->GetMinimumMatchSize();
	float OverallMult = 0.f;
	TArray<FGoodsQuantity> TempTotalGoods;
	
	// Iterate over each block, getting dropped goods from each
	for (AMMBlock* Block : Match->Blocks)
	{
		check(Block);
		TempTotalGoods.Append(Block->GetMatchGoods(GoodsDropper, Match));
		if (Block->GetBlockType().OverallMatchGoodsMultiplier > 0.f && Block->GetBlockType().OverallMatchGoodsMultiplier != 1.f) {
			OverallMult += Block->GetBlockType().OverallMatchGoodsMultiplier;
		}
	}
	// After normal goods, including bonus goods, have been determined, apply the cumulative overall multiplier. (if any)
	if (OverallMult > 0.f) 
	{
		MatchGoods.Goods.Append(
			UGoodsFunctionLibrary::MultiplyGoodsQuantities(
				// Use the AddGoodsQuantities function to consolidate all goods quantities to one total per goods type.}
				UGoodsFunctionLibrary::AddGoodsQuantities(TArray<FGoodsQuantity>(), TempTotalGoods), 
				OverallMult
			)
		);
	}
	else {
		// Use the AddGoodsQuantities function to consolidate all goods quantities to one total per goods type.}
		MatchGoods.Goods.Append(UGoodsFunctionLibrary::AddGoodsQuantities(TArray<FGoodsQuantity>(), TempTotalGoods));
	}
	return true;
}


int32 AMMGameMode::GetScoreForMatch(const UBlockMatch* Match)
{
	check(Match);
	int32 MatchSize = Match->Blocks.Num();
	if (MatchSize == 0) {
		return 0;
	}
	if (!Match->Blocks.IsValidIndex(0) || Match->Blocks[0]->Grid() == nullptr) {
		return 0;
	}
	int32 BonusMatchSize = MatchSize - Match->Blocks[0]->Grid()->GetMinimumMatchSize();
	float OverallMult = 0.f;
	int32 TotalScore = 0;
	for (AMMBlock* Block : Match->Blocks)
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


bool AMMGameMode::SetBlockTypeSetName(const FName& BlockTypeSetName)
{
	return InitWeightedBlockTypes(BlockTypeSetName);
}


void AMMGameMode::InitCachedBlockTypes(bool bForceRefresh)
{
	if (!(CachedBlockTypes.Num() == 0 || bForceRefresh)) { return; }
	if (!IsValid(BlocksTable)) 
	{
		UE_LOG(LogMMGame, Error, TEXT("AMMGameMode::InitCachedBlockTypes - BlocksTable is not valid"));
		return;
	}
	TArray<FSoftObjectPath> AssetsToCache;
	CachedBlockTypes.Empty(BlocksTable->GetRowMap().Num());
	for (const TPair<FName, uint8*>& It : BlocksTable->GetRowMap())
	{
		FBlockType FoundBlockType = *reinterpret_cast<FBlockType*>(It.Value);
		CachedBlockTypes.Add(It.Key, FoundBlockType);
		AssetsToCache.AddUnique(FoundBlockType.BlockClass.ToSoftObjectPath());
	}
	CacheAssets(AssetsToCache, FName(TEXT("Blocks")));
}


void AMMGameMode::InitWeightedBlockTypeSets(bool bForceRefresh)
{
	if (!(CurrentWeightedBlockTypeSets.Num() == 0 || bForceRefresh)) { return; }
	if (!IsValid(BlockWeightsTable)) {
		UE_LOG(LogMMGame, Error, TEXT("MMGameMode::InitWeightedBlockTypeSets - BlockWeightsTable is null."));
	}
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
		UE_LOG(LogMMGame, Log, TEXT("MMGameMode::InitWeightedBlockTypes - Using BlockTypeSet %s"), *BlockTypeSetName.ToString());
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
	if (!IsValid(GoodsTable)) 
	{
		UE_LOG(LogMMGame, Error, TEXT("MMGameMode::InitCachedGoodsTypes - GoodsTable is null."));
		return;
	}
	TArray<FSoftObjectPath> AssetsToCache;
	CachedGoodsTypes.Empty(GoodsTable->GetRowMap().Num());
	for (const TPair<FName, uint8*>& It : GoodsTable->GetRowMap())
	{
		FGoodsType* FoundGoodsType = reinterpret_cast<FGoodsType*>(It.Value);
		CachedGoodsTypes.Add(It.Key, *FoundGoodsType);
		AssetsToCache.AddUnique(FoundGoodsType->Thumbnail.ToSoftObjectPath());
	}
	CacheAssets(AssetsToCache, FName(TEXT("Goods")));
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


void AMMGameMode::CacheAssets(const TArray<FSoftObjectPath> AssetsToCache, const FName GroupName)
{
	if (AssetsToCache.Num() > 0)
	{
		AssetGroupsPendingLoad.AddUnique(GroupName);
		TArray<FSoftObjectPath> TmpAssets;
		for (FSoftObjectPath CurPath : AssetsToCache) {
			if (!CurPath.IsNull()) {
				TmpAssets.Add(CurPath);
			}
		}
		UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("MMGameMode::CacheAssets - %s caching %d assets"), *GroupName.ToString(), TmpAssets.Num());
		FStreamableManager& Manager = UAssetManager::GetStreamableManager();
		FStreamableDelegate Delegate = FStreamableDelegate::CreateUObject(this, &AMMGameMode::OnAssetsCached, TmpAssets, GroupName);
		Manager.RequestAsyncLoad(TmpAssets, Delegate);
	}
}


void AMMGameMode::OnAssetsCached(const TArray<FSoftObjectPath> CachedAssets, const FName GroupName)
{
	UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("MMGameMode::OnAssetsCached - %s cached %d assets"), *GroupName.ToString(), CachedAssets.Num());
	//for (FSoftObjectPath CurRef : CachedAssets)	{
	//	AssetsToCache.RemoveSingle(CurRef);
	//}
	AssetGroupsPendingLoad.RemoveSingle(GroupName);
	if (AssetGroupsPendingLoad.Num() == 0) {
		OnLoadComplete();
	}
}


void AMMGameMode::OnLoadComplete_Implementation()
{

}