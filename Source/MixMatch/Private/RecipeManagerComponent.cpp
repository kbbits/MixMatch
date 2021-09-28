
#include "RecipeManagerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "..\MixMatch.h"
#include "MMGameMode.h"
#include "Goods\GoodsFunctionLibrary.h"
#include "Goods\GoodsDropper.h"
#include "GameFramework/PlayerController.h"

// Sets default values for this component's properties
URecipeManagerComponent::URecipeManagerComponent()
{
	SetIsReplicated(false);
	SetIsReplicatedByDefault(false);
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


FCraftingRecipe URecipeManagerComponent::GetRecipe(const FName& RecipeName, bool& bFound)
{
	InitCraftingRecipes();
	if (AllRecipeData.Contains(RecipeName))
	{
		bFound = true;
		return *AllRecipeData.Find(RecipeName);
	}
	bFound = false;
	return FCraftingRecipe();
}


bool URecipeManagerComponent::GetBaseGoodsForRecipe(const FName& RecipeName, TArray<FGoodsQuantity>& BaseGoods)
{
	AMMGameMode* GameMode = Cast<AMMGameMode>(UGameplayStatics::GetGameMode(this));
	TArray<FGoodsQuantity> TmpBaseGoods;
	FCraftingRecipe Recipe;
	bool bFound;
	BaseGoods.Empty();
	if (!GameMode) {
		UE_LOG(LogMMGame, Error, TEXT("RecipeManager::GetBaseGoodsForRecipe - Could not get GameMode"));
		return false;
	}
	InitCraftingRecipes();
	Recipe = GetRecipe(RecipeName, bFound);
	if (!bFound)
	{
		UE_LOG(LogMMGame, Error, TEXT("RecipeManager::GetBaseGoodsForRecipe - Could not find recipe: %s"), *RecipeName.ToString());
		return false;
	}
	for (FGoodsQuantity GoodsInput : Recipe.CraftingInputs)
	{
		FGoodsType InputGoodsData = GameMode->GetGoodsData(GoodsInput.Name, bFound);
		if (bFound)
		{
			if (InputGoodsData.GoodsTags.Contains(FName(TEXT("Resource")))) 
			{
				TmpBaseGoods.Add(GoodsInput);
			}
			else
			{
				FName* ProducingRecipeName = GoodsToRecipeMap.Find(InputGoodsData.Name);
				if (ProducingRecipeName)
				{
					TArray<FGoodsQuantity> ProducingRecipeGoods;
					if (GetBaseGoodsForRecipe(*ProducingRecipeName, ProducingRecipeGoods))
					{
						TmpBaseGoods.Append(UGoodsFunctionLibrary::MultiplyGoodsQuantities(ProducingRecipeGoods, GoodsInput.Quantity));
					}					
				}
				else {
					UE_LOG(LogMMGame, Error, TEXT("RecipeManager::GetBaseGoodsForRecipe - Found no producing recipe for goods: %s"), *InputGoodsData.Name.ToString());
				}
			}
		}
		else {
			UE_LOG(LogMMGame, Error, TEXT("RecipeManager::GetBaseGoodsForRecipe - Input goods %s in recipe %s not found"), *GoodsInput.Name.ToString(), *Recipe.Name.ToString());
		}
	}
	BaseGoods = UGoodsFunctionLibrary::AddGoodsQuantities(TArray<FGoodsQuantity>(), TmpBaseGoods);
	return true;
}


// Called when the game starts
void URecipeManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	InitCraftingRecipes();
}


void URecipeManagerComponent::InitCraftingRecipes(bool bForceRefresh)
{
	if (AllRecipeData.Num() > 0 && !bForceRefresh) { return; }
	AMMGameMode* GameMode = Cast<AMMGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GameMode) {
		UE_LOG(LogMMGame, Error, TEXT("RecipeManager::InitCraftingRecipes - Could not get GameMode"));
		return;
	}
	if (!IsValid(CraftingRecipesTable)) {
		UE_LOG(LogMMGame, Error, TEXT("RecipeManager::InitCraftingRecipes - CraftingRecipesTable is not valid"));
		return;
	}
	bool bFound;
	// Get recipe data
	AllRecipeData.Empty(CraftingRecipesTable->GetRowMap().Num());
	for (const TPair<FName, uint8*>& It : CraftingRecipesTable->GetRowMap())
	{
		FCraftingRecipe FoundRecipe = *reinterpret_cast<FCraftingRecipe*>(It.Value);
		AllRecipeData.Add(It.Key, FoundRecipe);
		// Fill in the GoodsToRecipeMap
		for (FGoodsDropChance ResultsDrops : FoundRecipe.CraftingResults)
		{
			TArray<FGoodsQuantity> AllResultGoods = GameMode->GetGoodsDropper()->EvaluateGoodsDropChancePercent(ResultsDrops, 1.f);
			for (FGoodsQuantity ResultGoods : AllResultGoods)
			{
				if (GoodsToRecipeMap.Contains(ResultGoods.Name))
				{
					FCraftingRecipe ExistingRecipe = GetRecipe(GoodsToRecipeMap[ResultGoods.Name], bFound);
					if (bFound && FoundRecipe.Tier < ExistingRecipe.Tier) {
						GoodsToRecipeMap[ResultGoods.Name] = FoundRecipe.Name;
					}
				}
				else
				{
					GoodsToRecipeMap.Add(ResultGoods.Name, FoundRecipe.Name);
				}
			}
		}
	}
}

