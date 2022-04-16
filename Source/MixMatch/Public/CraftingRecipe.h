#pragma once

//#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Goods/GoodsQuantity.h"
#include "Goods/GoodsDropChance.h"
#include "CraftingRecipe.generated.h"


USTRUCT(BlueprintType)
struct FCraftingRecipe : public FTableRowBase
{
	GENERATED_BODY()

public:
	// Unique name for this recipe
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		FName Name;

	// Category information for this recipe. 
	// Indicates what equipment can craft this.
	// Also allows additional specifiers:
	//    ScaleInputValue:<InputGoodsName>:<float>  -- The value for the indicated goods name should be scaled by the given value when calculating auto-values (gold value and xp).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		TArray<FName> RecipeCategories;

	// User friendly name
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		TSoftObjectPtr<UTexture2D> Thumbnail;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		TSoftObjectPtr<USoundBase> CraftSound;

	// Currently unused. Additional requirements (not consumed) in order for the recipe to be crafted.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		TArray<FName> RecipeRequirements;

	// Goods required to craft one of instance of this recipe
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		TArray<FGoodsQuantity> CraftingInputs;

	// Goods produced by this recipe.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		TArray<FGoodsDropChance> CraftingResults;

	// Additional goods (hidden on GUI) also produced by this recipe.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		TArray<FGoodsDropChance> BonusCraftingResults;

	// The tier of this recipe.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		int32 Tier;

	// Currently unused. Seconds required to craft this recipe at normal (100%) crafting speed.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		float CraftingTime;

	// Experience awarded to player for crafting this recipe. Tier 1 recipes need a value here. Others will be auto-calculated unless there is a value here.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		float OverrideCraftingExperience;
};