#pragma once

#include "Engine/DataTable.h"
#include "GameEffect/GameEffect.h"
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
	TArray<TSubclassOf<UGameEffect>> GameEffects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Block)
	TSoftObjectPtr<USoundBase> UseSound;

};