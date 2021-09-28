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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BlockMoveSpeed;

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
	TArray<FWeightedBlockType> CurrentWeightedBlockTypes;

	float CurrentTotalBlockWeight;

	UPROPERTY()
	TMap<FName, FBlockType> CachedBlockTypes;

	UPROPERTY()
	TMap<FName, FGoodsType> CachedGoodsTypes;

public:

	virtual void BeginPlay() override;
		
	UFUNCTION(BlueprintPure)
	FGoodsType GetGoodsData(const FName& GoodsName, bool& bFound);

	UFUNCTION(BlueprintCallable)
	class UGoodsDropper* GetGoodsDropper();

	bool GetBlockTypeByName(const FName& BlockTypeName, FBlockType& FoundBlockType);
	
	bool GetRandomBlockTypeNameForCell(const AMMPlayGridCell* Cell, FName& FoundBlockTypeName);

	bool GetBlockClass(TSubclassOf<class AMMBlock>& BlockClass);

	UFUNCTION(BlueprintNativeEvent)
	bool GetGoodsForMatch(const FBlockMatch& Match, FGoodsQuantitySet& MatchGoods);
	virtual bool GetGoodsForMatch_Implementation(const FBlockMatch& Match, FGoodsQuantitySet& MatchGoods);

	UFUNCTION(BlueprintCallable)
	int32 GetScoreForMatch(const FBlockMatch& Match);

protected:

	void InitCachedBlockTypes(bool bForceRefresh = false);

	void InitWeightedBlockTypes(bool bForceRefresh = false);

	void InitCachedGoodsTypes(bool bForceRefresh = false);

	void InitGoodsDropper(bool bForceRefresh = false);

};



