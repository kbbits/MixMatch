// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DataTable.h"
#include "GoodsDropChance.h"
#include "GoodsDropSet.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FGoodsDropSet : public FTableRowBase
{
	GENERATED_BODY()
	
public:

	FGoodsDropSet()
		: Super()
	{
		bAsWeightedList = false;
		MinWeightedPicks = 1;
		MaxWeightedPicks = 1;
	}

	// Name of this drop set. Must be unique.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		FName Name;

	/*
	Determines how the list of GoodsDropChances will be evaluated.
		False - All items in the list will each have a % chance to be evaluated during drop based on the Chance property of the entry 0.0-1.0.
		True -  A number between MinWeightedPicks and MaxWeightedPicks of items in the list will be evaluated during drop. Each pick from the list is one 
				GoodsDropChance, where each entry is given a weight equal to it's Chance variable, > 0.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		bool bAsWeightedList;

	// If bAsWeightedList == true, this is the minimum number of picks from the weighted list to make.
	// Default = 1
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		int32 MinWeightedPicks;

	// If bAsWeightedList == true, this is the maximum number of picks from the weighted list to make.
	// Default = 1
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		int32 MaxWeightedPicks;

	// List of goods drop chances. Each entry has a chance of being dropped when this table is evaluated. See: AsWeightedList.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (TitleProperty = "GoodsQuantities"))
		TArray<FGoodsDropChance> GoodsChances;


	FORCEINLINE bool operator==(const FGoodsDropSet& OtherSet) const
	{
		if (Name != OtherSet.Name) return false;
		return true;
	}

	FORCEINLINE bool operator==(const FGoodsDropSet& OtherSet)
	{
		if (Name != OtherSet.Name) return false;
		return true;
	}

	FORCEINLINE bool operator==(FGoodsDropSet& OtherSet)
	{
		if (Name != OtherSet.Name) return false;
		return true;
	}

	FORCEINLINE bool operator==(const FName& OtherName) const
	{
		if (Name != OtherName) return false;
		return true;
	}

	FORCEINLINE bool operator==(const FName& OtherName)
	{
		if (Name != OtherName) return false;
		return true;
	}

	FORCEINLINE bool operator==(FName& OtherName)
	{
		if (Name != OtherName) return false;
		return true;
	}

};
