#pragma once

#include "MMEnums.h"
#include "Goods/GoodsQuantity.h"
#include "BlockMatch.generated.h"


UCLASS(BlueprintType, Blueprintable)
class MIXMATCH_API UBlockMatch : public UObject
{
	GENERATED_BODY()

public:

	UBlockMatch();

public:

	/** All of the blocks in the match. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	TArray<class AMMBlock*> Blocks;
	
	/** The grid orientation of this match. */
	UPROPERTY(BluePrintReadOnly)
	EMMOrientation Orientation = EMMOrientation::Unknown;

	/** The coordinates of the block in this match closest to coords(0,0) (bottom left) */
	UPROPERTY(BlueprintReadOnly)
	FIntPoint StartCoords = FIntPoint::NoneValue;

	/** The coordinates of the block in this match closest to coords(SizeX-1, SizeY-1) (top right) */
	UPROPERTY(BlueprintReadOnly)
	FIntPoint EndCoords = FIntPoint::NoneValue;

	/** Total score awarded from this match */
	UPROPERTY(BlueprintReadOnly)
	int32 TotalScore = 0;

	/** Total goods awarded from this match */
	UPROPERTY(BlueprintReadOnly)
	TArray<FGoodsQuantity> TotalGoods;

	/** Has this match been sorted? */
	UPROPERTY(BlueprintReadOnly)
	bool bSorted = false;

	/** Is this match finished resolving/processing? */
	UPROPERTY(BlueprintReadOnly)
	bool bMatchFinished = false;

	/** FUNCTIONS **/

public:

	/** Sort the blocks within the match from bottom left coords to top right coords. Also sets match's StartCoords and EndCoords. */
	UFUNCTION(BlueprintCallable)
	void Sort(const bool bForceSort = false);

	// Reset the match to it's default state with no blocks in the match.
	// Note: This does not change the bMatchFinished flag.
	UFUNCTION(BlueprintCallable)
	void Reset();
	
	/** Gets the location of the match in the world. i.e. the center of the group of blocks in the match. 
	 * Returns (-1, -1, -1) if location could not be determined. i.e. the match contains no blocks. */
	UFUNCTION(BlueprintPure)
	FVector GetWorldLocation();
};

