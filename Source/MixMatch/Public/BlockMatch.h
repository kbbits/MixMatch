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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	TArray<class AMMBlock*> Blocks;
	
	UPROPERTY(BluePrintReadOnly)
	EMMOrientation Orientation = EMMOrientation::Unknown;

	UPROPERTY(BlueprintReadOnly)
	FIntPoint StartCoords = FIntPoint::NoneValue;

	UPROPERTY(BlueprintReadOnly)
	FIntPoint EndCoords = FIntPoint::NoneValue;

	/** Total score awarded from this match */
	UPROPERTY(BlueprintReadOnly)
	int32 TotalScore = 0;

	/** Total goods awarded from this match */
	UPROPERTY(BlueprintReadOnly)
	TArray<FGoodsQuantity> TotalGoods;

	UPROPERTY(BlueprintReadOnly)
	bool bSorted = false;

	UPROPERTY(BlueprintReadOnly)
	bool bMatchFinished = false;

	/** FUNCTIONS **/

public:

	UFUNCTION(BlueprintCallable)
	void Sort(const bool bForceSort = false);

	// Reset the match to it's default state with no blocks in the match.
	// Note: This does not change the bMatchFinished flag.
	UFUNCTION(BlueprintCallable)
	void Reset();

	// Returns (-1, -1, -1) if location could not be determined. i.e. the match contains no blocks.
	UFUNCTION(BlueprintCallable)
	FVector GetWorldLocation();
};

