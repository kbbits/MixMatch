// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DataTable.h"
#include "GoodsQuantity.h"
#include "GoodsDropChance.generated.h"


USTRUCT(BlueprintType)
struct FGoodsDropChance : public FTableRowBase
{
	GENERATED_BODY()

public:
	FGoodsDropChance()
		: Super()
	{
		Chance = 1.0f;
	}

	// See: GoodsDropSet.AsWeightedList for use.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		float Chance;
	
	// When this instance is evaluaed during drop, all goods ranges in this list will be evaluated and included in the drop.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (TitleProperty = "GoodsName"))
		TArray<FGoodsQuantityRange> GoodsQuantities;

	// Array of names of GoodsDropSets to also be evaluated and included during drop.
	// Caution: do not create a GoodsDropSet that contains a GoodsDropChance that references it's own GoodsDropSet. (i.e. don't create circular references)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		TArray<FName> OtherGoodsDrops;
};