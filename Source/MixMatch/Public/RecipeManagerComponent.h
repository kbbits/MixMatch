#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Goods/GoodsQuantity.h"
#include "CraftingRecipe.h"
#include "RecipeManagerComponent.generated.h"

// Event dispatcher for when a recipe level changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnRecipeLevelChanged, const FCraftingRecipe&, ChangedRecipe, const int32, NewLevel, const int32, OldLevel);

/*
* Manages crafting recipes.
*/
UCLASS(Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MIXMATCH_API URecipeManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	URecipeManagerComponent();

	// Delegate event when inventory has changed.
	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
	FOnRecipeLevelChanged OnRecipeLevelChanged;

	/** Recipe data */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UDataTable* CraftingRecipesTable;


protected:
	
	/** Cache map of recipe data loaded from data source. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (TitleProperty = "Name"))
	TMap<FName, FCraftingRecipe> AllRecipeData;

	/** The current level of each recipe. */
	UPROPERTY(EditAnywhere)
	TMap<FName, int32> RecipeLevels;

	// Map of goods name to recipe name of recipe producing that goods type.
	UPROPERTY()
	TMap<FName, FName> GoodsToRecipeMap;

public:

	/** Get recipe data for given recipe name. */
	UFUNCTION(BlueprintPure)
	FCraftingRecipe GetRecipe(const FName& RecipeName, bool& bFound);

	/** Get the recipe that produces the given goods type. */
	UFUNCTION(BlueprintCallable)
	FCraftingRecipe GetRecipeForGoodsName(const FName& GoodsName, bool& bFound);

	/** Get the current level of the recipe */
	UFUNCTION(BlueprintPure)
	int32 GetRecipeLevel(const FName& RecipeName);

	/** Set the current level of the recipe. */
	UFUNCTION(BlueprintCallable)
	void SetRecipeLevel(const FName& RecipeName, const int32 NewLevel);

	/** Gets BaseGoods as the total goods tagged with "Resource" needed to produce all preceding ingredient goods. 
	 *	Returns true if recipe was found, false otherwise. */
	UFUNCTION(BlueprintPure)
	bool GetBaseIngredientsForRecipe(const FName& RecipeName, TArray<FGoodsQuantity>& BaseGoods);

	/** Gets the output goods for crafting the given recipe name once. 
	 *  Returns true if the recipe was found, false otherwise. */
	UFUNCTION(BlueprintPure)
	bool GetGoodsForRecipeName(const FName& RecipeName, TArray<FGoodsQuantity>& OutputGoods, const float QuantityScale = -1.f, const bool bExcludeBonusGoods = false);

	/** Gets the output goods for crafting the given recipe once.
	 *  Returns true if the recipe was found, false otherwise. */
	UFUNCTION(BlueprintPure)
	bool GetGoodsForRecipe(const FCraftingRecipe& Recipe, TArray<FGoodsQuantity>& OutputGoods, const float QuantityScale = -1.f, const bool bExcludeBonusGoods = false);

	/** Returns the number of times the given recipe could be crafted with the submitted goods quantities */
	UFUNCTION(BlueprintPure)
	int32 CraftableCountForGoods(const TArray<FGoodsQuantity>& GoodsQuantities, const FCraftingRecipe& Recipe);

protected:

	// Called when the game starts
	virtual void BeginPlay() override;

	/** Set up our cache of recipe data */
	void InitCraftingRecipes(bool bForceRefresh = false);

};
