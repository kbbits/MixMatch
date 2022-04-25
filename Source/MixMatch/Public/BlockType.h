#pragma once

#include "Engine/DataTable.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Goods/GoodsDropSet.h"
#include "MatchAction.h"
#include "BlockType.generated.h"

struct BlockCategory
{
	// The predefined name of the Goods block category.
	static const FName Goods;
};

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

	// This is the primary match code used for matching other blocks.
	// A match code of "Any" will match against any other block.
	// A match code of NAME_None will not match any other block unless the blocks match by category.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FName MatchCode = NAME_None;

	// Other match codes that this block will also match to
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	TArray<FName> OtherMatchCodes;

	// Categories (or groups) that this block type belongs to.
	// A block will compare it's MatchCode and OtherMatchCodes to a block's categories when determining matches.
	// Note that two blocks in the same category will not match unless they match for other reasons OR one or both of those blocks include
	// that category in their MatchCode or OtherMatchCodes.
	// 
	// Blocks that represent goods should have the GoodsCategory::Goods category added to them.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	TArray<FName> BlockCategories;

	// If this block is already in a match group, look back at the blocks in the match group before this one
	// until we find one not marked with bMatchNextToPreviousInMatchGroup and use that one to compare against
	// instead of this block.  If no prevous blocks in the match exist or they all are marked as bMatchNextToPreviousInMatchGroup
	// then this block will be compared to it's neighbor as normal.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	bool bMatchNextToPreviousInMatchGroup = false;

	// If true, this block will not match against other blocks that have the same MatchCode.
	// A unique match code should still be assigned to this block type.
	// Note: If this is true and our match code = "Any", this will still prevent the match with blocks with an "Any" match code.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	bool bPreventSelfMatch = false;

	// Name displayed to player
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	TSoftClassPtr<class AMMBlock> BlockClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FLinearColor PrimaryColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FLinearColor AltColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	bool bTakesDamage = false;
	
	// The base health for this block. Used by some special block types.
	// Not used if bTakesDamage = false (the default).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	float BaseHealth = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	bool bImmobile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	bool bIndestructible;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	int32 PointsPerBlock;

	// Goods dropped per block matched
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FGoodsDropSet MatchDropGoods;

	// Multipler applied to points from matching this block type with more than minimum match size.
	// Total points =  normal points + (normal points * (NumBlocksOverMinMatchSize * BonusSizePointsMultiplier))
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	float BonusMatchPointsMultiplier = 1.f;

	// Multipler applied to quantities of dropped goods from this block type with more than minimum match size.
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

	inline bool operator==(const FName& MatchCodeOther) const
	{
		if (MatchCode == NAME_None || MatchCodeOther == NAME_None) return false;
		if (bPreventSelfMatch && MatchCode == MatchCodeOther) return false;
		const FName Any = FName(TEXT("Any"));
		if (MatchCode == Any || MatchCodeOther == Any) return true;
		if (MatchCode == MatchCodeOther) return true;
		for (FName MyOtherMC : OtherMatchCodes) {
			if (MyOtherMC == MatchCodeOther) return true;
		}
		return false;
	}

	inline bool operator==(const FName& MatchCodeOther)
	{
		if (MatchCode == NAME_None || MatchCodeOther == NAME_None) return false;
		if (bPreventSelfMatch && MatchCode == MatchCodeOther) return false;
		const FName Any = FName(TEXT("Any"));
		if (MatchCode == Any || MatchCodeOther == Any) return true;
		if (MatchCode == MatchCodeOther) return true;
		for (FName MyOtherMC : OtherMatchCodes) {
			if (MyOtherMC == MatchCodeOther) return true;
		}
		return false;
	}

	inline bool operator==(FName& MatchCodeOther)
	{
		if (MatchCode == NAME_None || MatchCodeOther == NAME_None) return false;
		if (bPreventSelfMatch && MatchCode == MatchCodeOther) return false;
		const FName Any = FName(TEXT("Any"));
		if (MatchCode == Any || MatchCodeOther == Any) return true;
		if (MatchCode == MatchCodeOther) return true;
		for (FName MyOtherMC : OtherMatchCodes) {
			if (MyOtherMC == MatchCodeOther) return true;
		}
		return false;
	}

	inline bool operator==(const FBlockType& OtherType) const
	{
		// Use FName comparison for match codes. It will also check relevant OtherMatchCodes.
		if (*this == OtherType.MatchCode && OtherType == MatchCode) return true;
		// Check for category matches in the other block's categories
		for (FName OthersCategory : OtherType.BlockCategories)
		{
			if (MatchCode == OthersCategory) return true;
			for (FName MyOtherMC : OtherMatchCodes) {
				if (MyOtherMC == OthersCategory) return true;
			}
		}
		// Check for category matches in this block's categories
		for (FName MyCategory : BlockCategories)
		{
			if (MyCategory == OtherType.MatchCode) return true;
			for (FName OthersMC : OtherType.OtherMatchCodes) {
				if (MyCategory == OthersMC) return true;
			}
		}
		return false;
	}

	inline bool operator==(const FBlockType& OtherType)
	{
		if (*this == OtherType.MatchCode && OtherType == MatchCode) return true;
		for (FName OthersCategory : OtherType.BlockCategories)
		{
			if (MatchCode == OthersCategory) return true;
			for (FName MyOtherMC : OtherMatchCodes) {
				if (MyOtherMC == OthersCategory) return true;
			}
		}
		for (FName MyCategory : BlockCategories)
		{
			if (MyCategory == OtherType.MatchCode) return true;
			for (FName OthersMC : OtherType.OtherMatchCodes) {
				if (MyCategory == OthersMC) return true;
			}
		}
		return false;
	}

	inline bool operator==(FBlockType& OtherType)
	{
		if (*this == OtherType.MatchCode && OtherType == MatchCode) return true;
		for (FName OthersCategory : OtherType.BlockCategories)
		{
			if (MatchCode == OthersCategory) return true;
			for (FName MyOtherMC : OtherMatchCodes) {
				if (MyOtherMC == OthersCategory) return true;
			}
		}
		for (FName MyCategory : BlockCategories)
		{
			if (MyCategory == OtherType.MatchCode) return true;
			for (FName OthersMC : OtherType.OtherMatchCodes) {
				if (MyCategory == OthersMC) return true;
			}
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


USTRUCT(BlueprintType)
struct FAddBlockContext
{
	GENERATED_BODY()

public:

	/** Cell to add block to */
	UPROPERTY(BlueprintReadWrite)
	class AMMPlayGridCell* AddToCell;

	/** If this block is falling into grid, this is the distance above the grid to spawn it. */
	UPROPERTY(BlueprintReadWrite)
	float OffsetAboveTopCell = 0.0f;

	/** Should matches be prevented */
	UPROPERTY(BlueprintReadWrite)
	bool bPreventMatches = false;

	/** This add is for the initial filling of the grid at start */
	UPROPERTY(BlueprintReadWrite)
	bool bForInitialFill = false;

	/** Block names to exclude, if possible. */
	UPROPERTY(BlueprintReadWrite)
	TArray<FName> ExcludedBlockNames;
};
