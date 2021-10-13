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

	/** When caclulating experience for crafting a recipe higher than Tier 1:
	 *  total experience = ExperienceTierMultiplier * (the total experience for crafting recipes needed for this recipe's ingredients) 
	 *  Default = 1.1 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float ExperienceTierMultiplier;


protected:
	
	/** Cache map of recipe data loaded from data source. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (TitleProperty = "Name"))
	TMap<FName, FCraftingRecipe> AllRecipeData;

	/** Map of recipe names to the number of total times the player has crafted that recipe. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TMap<FName, int32> RecipeCraftings;

	/** The current level of each recipe. 
	 *  A recipe level <= 0 means the recipe is locked for the player. */
	UPROPERTY(EditAnywhere)
	TMap<FName, int32> RecipeLevels;

	// Map of goods name to recipe name of recipe producing that goods type.
	UPROPERTY()
	TMap<FName, FName> GoodsToRecipeMap;

public:

	/** Get recipe data for given recipe name. */
	UFUNCTION(BlueprintPure)
	FCraftingRecipe GetRecipe(const FName& RecipeName, bool& bFound);
	FCraftingRecipe GetRecipe(const FName& RecipeName);

	/** Get the recipe that produces the given goods type. */
	UFUNCTION(BlueprintCallable)
	FCraftingRecipe GetRecipeForGoodsName(const FName& GoodsName, bool& bFound);

	/** Get the recipes that have the given category */
	UFUNCTION(BlueprintCallable)
	TArray<FCraftingRecipe> GetRecipesWithCategory(const FName& Category);

	/** Get the recipes that have any of the given categories */
	UFUNCTION(BlueprintCallable)
	TArray<FCraftingRecipe> GetRecipesWithCategories(const TArray<FName>& Categories);

	/** Get the current level of the recipe */
	UFUNCTION(BlueprintPure)
	int32 GetRecipeLevel(const FName& RecipeName);

	/** Increases the recipe's current level by the IncrementAmount. Default = 1 */
	UFUNCTION(BlueprintCallable)
	int32 IncrementRecipeLevel(const FName& RecipeName, const int32 IncrementAmount = 1);

	/** Set the current level of the recipe. 
	 *  Note: Setting recipe level to <= 0 means the recipe is locked (unavailable) for the player.
	 *        Setting it to > 0 means the recipe is unlocked (available) for the player. */
	UFUNCTION(BlueprintCallable)
	void SetRecipeLevel(const FName& RecipeName, const int32 NewLevel);

	/** Gets the percent progress 0.0 - 1.0 for the given recipe to level up to the next level */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure)
	float GetRecipeLevelUpProgress(const FName& RecipeName);

	/** Does the recipe currently qualify for a level up? */
	UFUNCTION(BlueprintPure)
	bool IsRecipeReadyForLevelUp(const FName& RecipeName);

	UFUNCTION(BlueprintPure)
	bool IsRecipeUnlocked(const FName& RecipeName);

	/** Increases count of times the recipe has been crafted by the player by the IncrementAmount specified. 
	 *  @returns The new total of times the recipe has been crafted. */
	UFUNCTION(BlueprintCallable)
	int32 IncrementRecipeCraftingCount(const FName& RecipeName, const int32 IncrementAmount = 1);

	/** @returns The number of times the recipe has been crafted by the player. */
	UFUNCTION(BlueprintPure)
	int32 GetRecipeCraftingCount(const FName& RecipeName);

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

	/** @returns The experience for crafting the given recipe. For Tier 1 recipes, this is the value in OverrideCraftingExperience.
	 *  For Higher tier recipes the total experience is (sum of experience for crafting recipes for it's crafting inputs) * ExperienceTierMultiplier
	 *  Note: these multipliers are cumulative as we go up tiers. */
	UFUNCTION(BlueprintPure)
	int32 GetExperienceForRecipe(const FCraftingRecipe& Recipe);

	/** Returns the number of times the given recipe could be crafted with the submitted goods quantities */
	UFUNCTION(BlueprintPure)
	int32 CraftableCountForGoods(const TArray<FGoodsQuantity>& GoodsQuantities, const FCraftingRecipe& Recipe);

protected:

	// Called when the game starts
	virtual void BeginPlay() override;

	/** Set up our cache of recipe data */
	void InitCraftingRecipes(bool bForceRefresh = false);

private:

	int32 CalculateExperienceForRecipe(const FCraftingRecipe& Recipe, const FName& TopRecipeToCheck);

};
