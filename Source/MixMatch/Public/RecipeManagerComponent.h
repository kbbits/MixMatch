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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UDataTable* CraftingRecipesTable;


protected:
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (TitleProperty = "Name"))
	TMap<FName, FCraftingRecipe> AllRecipeData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, int32> RecipeLevels;

	// Map of goods name to recipe name of recipe producing that goods type.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<FName, FName> GoodsToRecipeMap;

public:

	UFUNCTION(BlueprintPure)
	FCraftingRecipe GetRecipe(const FName& RecipeName, bool& bFound);

	UFUNCTION(BlueprintCallable)
	FCraftingRecipe GetRecipeForGoodsName(const FName& GoodsName, bool& bFound);

	/** Gets BaseGoods as the total goods tagged with "Resource" needed to produce all preceding ingredient goods. 
	 *	Returns true if recipe was found, false otherwise. */
	UFUNCTION(BlueprintPure)
	bool GetBaseIngredientsForRecipe(const FName& RecipeName, TArray<FGoodsQuantity>& BaseGoods);

	/** Gets the output goods for crafting the given recipe once. 
	 *  Returns true if the recipe was found, false otherwise. */
	UFUNCTION(BlueprintPure)
	bool GetGoodsForRecipeName(const FName& RecipeName, TArray<FGoodsQuantity>& OutputGoods, const float QuantityScale = -1.f, const bool bExcludeBonusGoods = false);

	/** Gets the output goods for crafting the given recipe once.
	 *  Returns true if the recipe was found, false otherwise. */
	UFUNCTION(BlueprintPure)
	bool GetGoodsForRecipe(const FCraftingRecipe& RecipeName, TArray<FGoodsQuantity>& OutputGoods, const float QuantityScale = -1.f, const bool bExcludeBonusGoods = false);

protected:

	// Called when the game starts
	virtual void BeginPlay() override;

	void InitCraftingRecipes(bool bForceRefresh = false);

};
