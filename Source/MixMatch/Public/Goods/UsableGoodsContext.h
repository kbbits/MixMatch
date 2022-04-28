#pragma once

//#include "Goods/UsableGoodsType.h"
#include "Goods/UsableGoodsType.h"
#include "GameEffect/GameEffect.h"
#include "UsableGoodsContext.generated.h"

//class UUsableGoods;
//class UGameEffect;

UCLASS(BlueprintType)
class MIXMATCH_API UUsableGoodsContext : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite)
	TArray<FIntPoint> SelectedCoords;

protected:

	UPROPERTY()
	TArray<class UGameEffect*> GameEffectsCache;

private:

	UPROPERTY()
	UUsableGoods* UsableGoods;

public:

	UUsableGoodsContext();

	UFUNCTION(BlueprintPure)
	UUsableGoods* GetUsableGoods() const;

	UFUNCTION(BlueprintCallable)
	void SetUsableGoods(const UUsableGoods* NewUsableGoods);

	UFUNCTION(BlueprintCallable)
	void GetGameEffects(TArray<UGameEffect*>& GameEffects);

	UFUNCTION(BlueprintCallable)
	bool RequiresSelection();
	
	/** Cleans out external references and internal caches */
	UFUNCTION(BlueprintCallable)
	void Cleanup();
};