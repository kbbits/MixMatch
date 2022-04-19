// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/DataTable.h"
#include "Goods/GoodsType.h"
#include "Goods/GoodsQuantity.h"
#include "BlockMatch.h"
#include "BlockType.h"
#include "MatchAction.h"
//#include "MMPlayGridCell.h"
#include "MMGameMode.generated.h"

class AMMPlayGridCell;

/** GameMode class to specify pawn and playercontroller */
UCLASS(minimalapi)
class AMMGameMode : public AGameModeBase
{
	GENERATED_BODY()


public:
	AMMGameMode();

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UDataTable* GoodsTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UDataTable* BlocksTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UDataTable* BlockWeightsTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UDataTable* GoodsDropperTable;


	/** When caclulating experience for crafting a recipe higher than Tier 1:
	 *  total experience = ExperienceTierMultiplier * (the total experience for crafting recipes needed for this recipe's ingredients)
	 *  Default = 1.1 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
		float ExperienceTierMultiplier;

	/** When calculating value of goods, this multiplier is applied at each tier of the recipe chain. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
		float ValueTierMultiplier;

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	class UGoodsDropper* GoodsDropper;

	UPROPERTY()
	FName CurrentWeightedBlockTypeSetName;

	UPROPERTY()
	TMap<FName, FWeightedBlockTypeSet> CurrentWeightedBlockTypeSets;

	UPROPERTY()
	TArray<FWeightedBlockType> CurrentWeightedBlockTypes;


	float CurrentTotalBlockWeight;

	UPROPERTY()
	TMap<FName, FBlockType> CachedBlockTypes;

	UPROPERTY()
	TMap<FName, FGoodsType> CachedGoodsTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDebugLog = true;

private:

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	float DefaultBlockMoveSpeed;

	UPROPERTY()
	TArray<FName> AssetGroupsPendingLoad;
	
	// ####### FUNCTIONS

public:

	virtual void BeginPlay() override;
		
	/** Retrieve the GoodsType info with the given name. */
	UFUNCTION(BlueprintPure)
	FGoodsType GetGoodsData(const FName& GoodsName, bool& bFound);

	UFUNCTION(BlueprintCallable)
	class UGoodsDropper* GetGoodsDropper();

	/** Retrieve the BlockType info with the given name. */
	bool GetBlockTypeByName(const FName& BlockTypeName, FBlockType& FoundBlockType);
	
	/** Determine the block type name for spawning a new block. Play grids may default to this implementation but often have their own logic. */
	bool GetRandomBlockTypeNameForCell(FName& FoundBlockTypeName, const FAddBlockContext& BlockContext);

	bool GetBlockClass(TSubclassOf<class AMMBlock>& BlockClass);

	/** how fast, units/second, blocks move on the grid */
	UFUNCTION(BlueprintPure)
	float GetBlockMoveSpeed();

	/** Get the list of goods dropped by the given match */
	UFUNCTION(BlueprintNativeEvent)
	bool GetGoodsForMatch(const UBlockMatch* Match, FGoodsQuantitySet& MatchGoods);
	virtual bool GetGoodsForMatch_Implementation(const UBlockMatch* Match, FGoodsQuantitySet& MatchGoods);

	/** Calculate the total score for the given match */
	UFUNCTION(BlueprintCallable)
	int32 GetScoreForMatch(const UBlockMatch* Match);

	bool SetBlockTypeSetName(const FName& BlockTypeSetName);

	void CacheAssets(const TArray<FSoftObjectPath> AssetsToCache, const FName GroupName);

protected:

	void InitCachedBlockTypes(bool bForceRefresh = false);

	void InitWeightedBlockTypeSets(bool bForceRefresh = false);

	bool InitWeightedBlockTypes(const FName& BlockTypeSetName, bool bForceRefresh = false);

	void InitCachedGoodsTypes(bool bForceRefresh = false);

	void InitGoodsDropper(bool bForceRefresh = false);
	
	void OnAssetsCached(const TArray<FSoftObjectPath> CachedAssets, const FName GroupName);

	UFUNCTION(BlueprintNativeEvent)
	void OnLoadComplete();

};



