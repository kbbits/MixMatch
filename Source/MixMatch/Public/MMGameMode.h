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

private:

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	float DefaultBlockMoveSpeed;

	// ####### FUNCTIONS

public:

	virtual void BeginPlay() override;
		
	UFUNCTION(BlueprintPure)
	FGoodsType GetGoodsData(const FName& GoodsName, bool& bFound);

	UFUNCTION(BlueprintCallable)
	class UGoodsDropper* GetGoodsDropper();

	bool GetBlockTypeByName(const FName& BlockTypeName, FBlockType& FoundBlockType);
	
	bool GetRandomBlockTypeNameForCell(const AMMPlayGridCell* Cell, FName& FoundBlockTypeName);

	bool GetRandomBlockTypeNameForCell(const AMMPlayGridCell* Cell, FName& FoundBlockTypeName, const TArray<FName>& ExcludedBlockNames);

	bool GetBlockClass(TSubclassOf<class AMMBlock>& BlockClass);

	UFUNCTION(BlueprintPure)
	float GetBlockMoveSpeed();

	UFUNCTION(BlueprintNativeEvent)
	bool GetGoodsForMatch(const UBlockMatch* Match, FGoodsQuantitySet& MatchGoods);
	virtual bool GetGoodsForMatch_Implementation(const UBlockMatch* Match, FGoodsQuantitySet& MatchGoods);

	UFUNCTION(BlueprintCallable)
	int32 GetScoreForMatch(const UBlockMatch* Match);

	bool SetBlockTypeSetName(const FName& BlockTypeSetName);

protected:

	void InitCachedBlockTypes(bool bForceRefresh = false);

	void InitWeightedBlockTypeSets(bool bForceRefresh = false);

	bool InitWeightedBlockTypes(const FName& BlockTypeSetName, bool bForceRefresh = false);

	void InitCachedGoodsTypes(bool bForceRefresh = false);

	void InitGoodsDropper(bool bForceRefresh = false);

};



