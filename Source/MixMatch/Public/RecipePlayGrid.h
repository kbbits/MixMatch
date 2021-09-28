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

	virtual bool GetRandomBlockTypeNameForCell_Implementation(const AMMPlayGridCell* Cell, FName& FoundBlockTypeName) override;
};

