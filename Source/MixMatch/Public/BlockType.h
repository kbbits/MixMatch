#pragma once

#include "Engine/DataTable.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Goods/GoodsDropSet.h"
#include "MatchAction.h"
#include "BlockType.generated.h"


/*
* Represents a type of Block that can be spawned into the play grid.
*/
USTRUCT(BlueprintType)
struct FBlockType : public FTableRowBase
{
	GENERATED_BODY()

public:
	// Internal name for this block type. Must be unique.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FName Name = NAME_None;

	// Code used for matching blocks.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FName MatchCode = NAME_None;

	// Name displayed to player
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	TSoftClassPtr<class AMMBlock> BlockClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FLinearColor PrimaryColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FLinearColor AltColor;

	// Other match codes that this block will also match to
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	TArray<FName> OtherMatchCodes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	bool bImmobile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	int32 PointsPerBlock;

	// Goods dropped per block matched
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FGoodsDropSet MatchDropGoods;

	// Multipler applied to points from matching this block type with more than minimum match size.
	// Total points =  normal points + (normal points * (NumBlocksOverMinMatchSize * BonusSizePointsMultiplier))
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	float BonusMatchPointsMultiplier = 1.f;

	// Multipler applied to quantities of dropped goods from this block type with more than  minimum match size.
	// Total goods dropped =  normal drop goods for block + (normal drop goods for block * (NumBlocksOverMinMatchSize * BonusSizeGoodsMultiplier))
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	float BonusMatchGoodsMultiplier = 1.f;

	// Multiplier applied to all points in a match.
	// Total points from a match are multiplied by this factor for each block.
	// overall total points = base total points * (sum of all OverallMatchPointsMultiplier values for blocks where OverallMatchPointsMultiplier != 1.0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	float OverallMatchPointsMultiplier = 1.f;

	// Multiplier applied to all GoodsQuantities in a match.
	// Total goods from a match are multiplied by this factor for each block.
	// overall total goods = base total goods * (sum of all OverallMatchPointsMultiplier values for blocks where OverallMatchPointsMultiplier != 1.0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		float OverallMatchGoodsMultiplier = 1.f;

	// Actions to trigger based on match size.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	TArray<FMatchActionTrigger> MatchActions;

	// Thumbnail for GUI use
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	//TAssetPtr<UTexture2D> Thumbnail;

	// Actions triggerd by matches containing this block type.
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	//	TArray<FName> BlockTags;

	FORCEINLINE bool operator==(const FBlockType& OtherType) const
	{
		if (MatchCode == OtherType.MatchCode) return true;
		for (FName MC : OtherMatchCodes)
		{
			if (MC == OtherType.MatchCode) return true;
		}
		for (FName OMC : OtherType.OtherMatchCodes)
		{
			if (OMC == MatchCode) return true;
		}
		return false;
	}

	FORCEINLINE bool operator==(const FBlockType& OtherType)
	{
		if (MatchCode == OtherType.MatchCode) return true;
		for (FName MC : OtherMatchCodes)
		{
			if (MC == OtherType.MatchCode) return true;
		}
		for (FName OMC : OtherType.OtherMatchCodes)
		{
			if (OMC == MatchCode) return true;
		}
		return false;
	}

	FORCEINLINE bool operator==(FBlockType& OtherType)
	{
		if (MatchCode == OtherType.MatchCode) return true;
		for (FName MC : OtherMatchCodes)
		{
			if (MC == OtherType.MatchCode) return true;
		}
		for (FName OMC : OtherType.OtherMatchCodes)
		{
			if (OMC == MatchCode) return true;
		}
		return false;
	}

	FORCEINLINE bool operator==(const FName& OtherMatchCode) const
	{
		if (MatchCode == OtherMatchCode) return true;
		for (FName MC : OtherMatchCodes)
		{
			if (MC == OtherMatchCode) return true;
		}
		return false;
	}

	FORCEINLINE bool operator==(const FName& OtherMatchCode)
	{
		if (MatchCode == OtherMatchCode) return true;
		for (FName MC : OtherMatchCodes)
		{
			if (MC == OtherMatchCode) return true;
		}
		return false;
	}

	FORCEINLINE bool operator==(FName& OtherMatchCode)
	{
		if (MatchCode == OtherMatchCode) return true;
		for (FName MC : OtherMatchCodes)
		{
			if (MC == OtherMatchCode) return true;
		}
		return false;
	}
};


USTRUCT(BlueprintType)
struct FBlockTypeQuantity
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FBlockType BlockType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	int32 Quantity;

	FBlockTypeQuantity()
	{
		BlockType = FBlockType();
		Quantity = 0;
	}

	FBlockTypeQuantity(const FBlockType& NewBlockType, const int32 NewQuantity)
	{
		BlockType = NewBlockType;
		Quantity = NewQuantity;
	}
};


USTRUCT(BlueprintType)
struct FWeightedBlockType : public FTableRowBase
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	float Weight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FName BlockTypeName;
};


USTRUCT(BlueprintType)
struct FWeightedBlockTypeSet : public FTableRowBase
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	TArray<FWeightedBlockType> WeightedBlockTypes;
};