#pragma once

#include "Engine/DataTable.h"
#include "MMEnums.h"
#include "SimpleNamedTypes.h"
#include "Goods/Goods.h"
//#include "AttributeDataSet.h"
#include "PlayerSaveData.generated.h"


USTRUCT(BlueprintType)
struct FPlayerSaveData : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		FGuid PlayerGuid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		FName ProfileName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		float MaxTierCompleted;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		float TotalPlaytime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		int32 ExperienceLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		TArray<FGoodsQuantity> GoodsInventory;
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		TArray<FGoodsQuantity> SnapshotInventory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		TArray<FSimpleNamedInt> RecipeLevels;
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		TArray<FGoodsQuantity> TotalGoodsCollected;
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		TArray<FGoodsQuantity> TotalGoodsCrafted;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		TArray<FSimpleNamedInt> TotalRecipesCrafted;
		
};