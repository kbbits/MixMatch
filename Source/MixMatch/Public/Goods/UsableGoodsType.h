#pragma once

#include "Engine/DataTable.h"
#include "GameEffect/GameEffect.h"
#include "Goods/GoodsType.h"
#include "UsableGoodsType.generated.h"

/*
* Contains the full details for goods types that are "usable" by the player.
* Each record row name and Name must match a GoodsType.
*/
USTRUCT(BlueprintType)
struct FUsableGoodsType : public FTableRowBase
{
	GENERATED_BODY()

public:
	
	// Internal name of this UsableGoodsType. Must match a single GoodsType name.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	TArray<FGameEffectContext> GameEffects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	TSoftObjectPtr<USoundBase> UseSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FText UseDescription;

};


UCLASS(BlueprintType)
class MIXMATCH_API UUsableGoods : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	FGoodsType GoodsType;

	UPROPERTY(BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	FUsableGoodsType UsableGoodsType;

public:

	UUsableGoods();

	UFUNCTION(BlueprintPure)
	bool IsUsable();

	UFUNCTION(BlueprintPure)
	FName GetName();
};