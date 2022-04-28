#pragma once

#include "CoreMinimal.h"
#include "Goods/GoodsType.h"
#include "Goods/GoodsQuantity.h"
#include "GoodsTypeQuantity.generated.h"

/**
 */
UCLASS(BlueprintType, Blueprintable)
class MIXMATCH_API UGoodsTypeQuantity : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true))
		FGoodsType GoodsType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true))
		float Quantity;

public:
	
	// Constructor
	UGoodsTypeQuantity();


	UFUNCTION(BlueprintPure)
		FGoodsQuantity GetGoodsQuantity();
};