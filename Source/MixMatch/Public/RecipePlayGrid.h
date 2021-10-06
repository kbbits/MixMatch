#pragma once

#include "MMPlayGrid.h"
#include "CraftingRecipe.h"
#include "RecipePlayGrid.generated.h"

class URecipeManagerComponent;


/** Grid class for crafting a recipe */
UCLASS(minimalapi)
class ARecipePlayGrid : public AMMPlayGrid
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TargetBlockTypes = 4;

protected:

	URecipeManagerComponent* RecipeManager = nullptr;
	UPROPERTY()
	FCraftingRecipe CurrentRecipe;

public:

	ARecipePlayGrid();

	UFUNCTION(BlueprintCallable)
	void SetRecipeManager(UPARAM(ref) URecipeManagerComponent* NewRecipeManager);

	UFUNCTION(BlueprintPure)
	URecipeManagerComponent* GetRecipeManager();

	UFUNCTION(BlueprintCallable)
	void SetRecipe(UPARAM(ref) FCraftingRecipe& Recipe);

	/** Returns true if recipe was found. */
	UFUNCTION(BlueprintCallable)
	bool SetRecipeByName(const FName& RecipeName);

	UFUNCTION(BlueprintPure)
	const FCraftingRecipe& GetRecipe();

	/** Helper to get the recipe level from player */
	int32 GetRecipeLevel();

	/** The percent chance (0.0-1.0) that a dropped-in block will be a recipe ingredient block. 
	 * This is based on recipe level. */
	UFUNCTION(BlueprintNativeEvent)
	float GetChanceForIngredientBlock();

	virtual bool GetRandomBlockTypeNameForCellEx_Implementation(const AMMPlayGridCell* Cell, FName& FoundBlockTypeName, const TArray<FName>& ExcludedBlockNames) override;
};

