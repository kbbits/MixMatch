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

	/** The "ideal" number of different block types that this grid should generate. 
	 *  Default is 4. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TargetBlockTypes = 4;

	/** When generating blocks, blocks that represent ingredients for the current recipe will only be generated if they 
	 *  exist in this grid's inventory. Also, when these ingredient blocks are spawned, their respective goods will be 
	 *  removed from the grid's inventory. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIngredientsFromInventory = false;

protected:

	/** A reference to the active recipe manager. Currently this would be a ref to the recipe manager on the player controller. */
	URecipeManagerComponent* RecipeManager = nullptr;

	/** The recipe currently associated with this grid */
	UPROPERTY()
	FCraftingRecipe CurrentRecipe;

public:

	ARecipePlayGrid();

	/** Set the grid's reference the recipe manager for it to use. */
	UFUNCTION(BlueprintCallable)
	void SetRecipeManager(UPARAM(ref) URecipeManagerComponent* NewRecipeManager);

	/** Get the grid's current recipe manager. */
	UFUNCTION(BlueprintPure)
	URecipeManagerComponent* GetRecipeManager();

	/** Set the grid's current recipe. */
	UFUNCTION(BlueprintCallable)
	void SetRecipe(UPARAM(ref) FCraftingRecipe& Recipe);

	/** Returns true if recipe was found. */
	UFUNCTION(BlueprintCallable)
	bool SetRecipeByName(const FName& RecipeName);

	/** The recipe this grid is currently producing */
	UFUNCTION(BlueprintPure)
	const FCraftingRecipe& GetRecipe();

	/** Helper to get the current recipe's level from player */
	UFUNCTION(BlueprintPure)
	int32 GetRecipeLevel();

	/** The percent chance (0.0-1.0) that a dropped-in block will be a recipe ingredient block. 
	 * This is based on recipe level. */
	UFUNCTION(BlueprintNativeEvent)
	float GetChanceForIngredientBlock();

	/** Override the base class's logic for which blocks will drop into the grid. */
	virtual bool GetRandomBlockTypeNameForCellEx_Implementation(const AMMPlayGridCell* Cell, FName& FoundBlockTypeName, const TArray<FName>& ExcludedBlockNames) override;

	/** Override base class so we can deduct ingredient goods from inventory if relevant. */
	AMMBlock* AddRandomBlockInCellEx(AMMPlayGridCell* Cell, const TArray<FName>& ExcludedBlockNames, const float OffsetAboveCell, const bool bAllowUnsettle, const bool bPreventMatches) override;
};

