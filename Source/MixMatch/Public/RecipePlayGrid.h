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

	/** Inventory to hold ingredient Goods for this grid. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	class UInventoryActorComponent* InputGoodsInventory;
		
	/** The multiplier applied to the odds to drop an ingredient block. 
	 *  chance to drop ingredient = (Number of ingredient block types / TargetBlockTypes) * IngredientDropFactor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float IngredientDropFactor = 0.75f;

	/** Set from Recipe.BlockTypeSetBase when SetRecipe() is called. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FString NonIngredientBlockTypeSetBaseName;

	/** When generating blocks, blocks that represent ingredients for the current recipe will only be generated if they 
	 *  exist in this grid's input inventory. Also, when these ingredient blocks are spawned, their respective goods will be 
	 *  removed from the grid's input inventory. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIngredientsFromInventory = false;

protected:

	/** A reference to the active recipe manager. Currently this would be a ref to the recipe manager on the player controller. */
	URecipeManagerComponent* RecipeManager = nullptr;

	/** The recipe currently associated with this grid */
	UPROPERTY()
	FCraftingRecipe CurrentRecipe;

	/** The default number of different block types that this grid should generate when spawning new blocks.
	 *  Value is set from RecipeManager based on recipe. */
	UPROPERTY(BlueprintReadOnly)
	int32 TargetBlockTypes = 4;

	UPROPERTY()
	TArray<FGoodsQuantity> IngredientBlockDropOdds;

public:

	ARecipePlayGrid();

	virtual void StartPlayGrid_Implementation() override;

	/** This returns all unused ingredients to player inventory. Does not call base class. */
	virtual void StopPlayGrid_Implementation() override;

	/** Set the grid's reference the recipe manager for it to use. */
	UFUNCTION(BlueprintCallable)
	void SetRecipeManager(UPARAM(ref) URecipeManagerComponent* NewRecipeManager);

	/** Get the grid's current recipe manager. 
	 *  If not set, this will set the recipe manager to the one on the player controller and return it. */
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

	/** Crafts a recipe with ingredients from grid's inventory and puts crafted goods in player's inventory. */
	UFUNCTION(BlueprintCallable)
	bool CraftRecipe(const FCraftingRecipe& Recipe);

	/** The percent chance (0.0-1.0) that a dropped-in block will be a recipe ingredient block. 
	 * This is based on recipe level. */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure)
	float GetChanceForIngredientBlock();

	/** The chance of the given ingredient block dropping amongst other ingredient blocks. */
	UFUNCTION(BlueprintPure)
	float GetIngredientBlockRelativeChance(const FName& GoodsName);

	/** Override the base class's logic for which blocks will drop into the grid. */
	virtual bool GetRandomBlockTypeNameForCell_Implementation(FName& FoundBlockTypeName, const FAddBlockContext& BlockContext) override;

	/** Override base class so we can deduct ingredient goods from inventory if relevant. */
	virtual AMMBlock* AddRandomBlockInCell(const FAddBlockContext& BlockContext) override;

protected:

	void InitIngredientBlockDropOdds(const bool bForceRefresh = false);
};

