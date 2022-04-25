// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/DataTable.h"
#include "Goods/GoodsType.h"
#include "Goods/UsableGoodsType.h"
#include "Goods/GoodsQuantity.h"
#include "BlockMatch.h"
#include "BlockType.h"
#include "MatchAction.h"
//#include "MMPlayGridCell.h"
#include "MMGameMode.generated.h"

class AMMPlayGridCell;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGameEffectBegan, const UGameEffect*, GameEffect, const TArray<FIntPoint>&, EffectCoords);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameEffectEnded, const UGameEffect*, GameEffect);


/** GameMode class to specify pawn and playercontroller */
UCLASS(minimalapi)
class AMMGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMMGameMode();

public:

	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
	FOnGameEffectBegan OnGameEffectBegan;

	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
	FOnGameEffectEnded OnGameEffectEnded;

	/** FGoodsType rows. Describes all goods. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UDataTable* GoodsTable;

	/** FUsableGoodsType rows. Entries for goods that are usable. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UDataTable* UsableGoodsTable;

	/** FBlockType rows. Describes all block types. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UDataTable* BlocksTable;

	/** FWeightedBlockTypeSet rows. Describes all block type sets. */
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

	/** The maximum size that the player's action bar can expand to. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 MaxActionBarSize = 10;

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

	UPROPERTY()
	TMap<FName, FUsableGoodsType> CachedUsableGoodsTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDebugLog = true;

private:

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	float DefaultBlockMoveSpeed;

	UPROPERTY()
	TArray<UGameEffect*> ActiveGameEffects;

	UPROPERTY()
	TArray<FName> AssetGroupsPendingLoad;
	
	// ####### FUNCTIONS

public:

	virtual void BeginPlay() override;
		
	/** Retrieve the GoodsType info with the given name. */
	UFUNCTION(BlueprintPure)
	FGoodsType GetGoodsData(const FName& GoodsName, bool& bFound);

	/** Get the GoodsTypeData and associated UsableGoodsType data, if any. 
	 *  Returns true if UsableGoodsType data exists for the given goods name. */
	UFUNCTION(BlueprintPure)
	bool GetUsableGoodsData(const FName& GoodsName, FGoodsType& GoodsType, FUsableGoodsType& UsableGoodsType, bool& bFound);

	/** Get the GoodsData and UsableGoodsData (if any) as a UUsableGoods object instance. */
	UFUNCTION(BlueprintCallable)
	UUsableGoods* GetUsableGoods(const FName& GoodsName, bool& bFound);

	/** Get our GoodsDropper instance. */
	UFUNCTION(BlueprintCallable)
	class UGoodsDropper* GetGoodsDropper();

	/** Retrieve the BlockType info with the given name. */
	bool GetBlockTypeByName(const FName& BlockTypeName, FBlockType& FoundBlockType);
	
	/** Determine the block type name for spawning a new block. Play grids may default to this implementation but often have their own logic. */
	bool GetRandomBlockTypeNameForCell(FName& FoundBlockTypeName, const FAddBlockContext& BlockContext);

	/** Currently always returns AMMBlock::StaticClass() */
	bool GetBlockClass(TSubclassOf<class AMMBlock>& BlockClass);

	/** Speed, units/second, blocks move on the grid */
	UFUNCTION(BlueprintPure)
	float GetBlockMoveSpeed();

	/** Get the goods dropped by this block. */
	UFUNCTION(BlueprintCallable)
	bool GetGoodsForBlock(const AMMBlock* Block, FGoodsQuantitySet& BlockGoods);

	/** Get the list of goods dropped by the given match */
	UFUNCTION(BlueprintNativeEvent)
	bool GetGoodsForMatch(const UBlockMatch* Match, FGoodsQuantitySet& MatchGoods);
	virtual bool GetGoodsForMatch_Implementation(const UBlockMatch* Match, FGoodsQuantitySet& MatchGoods);

	/** Calculate the total score for the given match */
	UFUNCTION(BlueprintCallable)
	int32 GetScoreForMatch(const UBlockMatch* Match);

	/** Set the BlockTypeSetName */
	bool SetBlockTypeSetName(const FName& BlockTypeSetName);

	/** Load and cache this group of assets. */
	void CacheAssets(const TArray<FSoftObjectPath> AssetsToCache, const FName GroupName);

	/** GameEffects stuff */

	/** Perform the given GameEffect associated with this action.
	 * Returns: true if action operation was successful. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool AddGameEffect(const FGameEffectContext& EffectContext, const TArray<FIntPoint>& EffectCoords);

	/** Increment each active effect by one turn.  End any effects with no remaining duration. */
	UFUNCTION()
	void IncrementGameEffectsTurn();

	UFUNCTION()
	void EndGameEffect(UGameEffect* GameEffect);

	/** Call EndEffect on all active effects and clear the list. */
	UFUNCTION(BlueprintCallable)
	void EndAllActiveGameEffects();

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



