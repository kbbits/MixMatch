
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
	UE_LOG(LogMMGame, Error, TEXT("RecipeManager::GetRecipe - Recipe name not found: %s"), *RecipeName.ToString());
	bFound = false;
	return FCraftingRecipe();
}


FCraftingRecipe URecipeManagerComponent::GetRecipeForGoodsName(const FName& GoodsName, bool& bFound)
{
	FName* ProducingRecipeName = GoodsToRecipeMap.Find(GoodsName);
	if (ProducingRecipeName != nullptr) 
	{
		return GetRecipe(*ProducingRecipeName, bFound);
	}
	UE_LOG(LogMMGame, Error, TEXT("RecipeManager::GetRecipeForGoodsName - No producing recipe found for goods name: %s"), *GoodsName.ToString());
	bFound = false;
	return FCraftingRecipe();
}


int32 URecipeManagerComponent::GetRecipeLevel(const FName& RecipeName)
{
	if (RecipeLevels.Contains(RecipeName)) {
		return RecipeLevels[RecipeName];
	}
	return 0;
}


void URecipeManagerComponent::SetRecipeLevel(const FName& RecipeName, const int32 NewLevel)
{
	if (RecipeLevels.Contains(RecipeName)) {
		RecipeLevels[RecipeName] = NewLevel;
	}
	else {
		RecipeLevels.Add(RecipeName, NewLevel);
	}
}


bool URecipeManagerComponent::GetBaseIngredientsForRecipe(const FName& RecipeName, TArray<FGoodsQuantity>& BaseGoods)
{
	AMMGameMode* GameMode = Cast<AMMGameMode>(UGameplayStatics::GetGameMode(this));
	TArray<FGoodsQuantity> TmpBaseGoods;
	FCraftingRecipe Recipe;
	bool bFound;
	BaseGoods.Empty();
	if (!GameMode) {
		UE_LOG(LogMMGame, Error, TEXT("RecipeManager::GetBaseIngredientsForRecipe - Could not get GameMode"));
		return false;
	}
	InitCraftingRecipes();
	Recipe = GetRecipe(RecipeName, bFound);
	if (!bFound)
	{
		UE_LOG(LogMMGame, Error, TEXT("RecipeManager::GetBaseIngredientsForRecipe - Could not find recipe: %s"), *RecipeName.ToString());
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
					if (GetBaseIngredientsForRecipe(*ProducingRecipeName, ProducingRecipeGoods))
					{
						TmpBaseGoods.Append(UGoodsFunctionLibrary::MultiplyGoodsQuantities(ProducingRecipeGoods, GoodsInput.Quantity));
					}					
				}
				else {
					UE_LOG(LogMMGame, Error, TEXT("RecipeManager::GetBaseIngredientsForRecipe - Found no producing recipe for goods: %s"), *InputGoodsData.Name.ToString());
				}
			}
		}
		else {
			UE_LOG(LogMMGame, Error, TEXT("RecipeManager::GetBaseIngredientsForRecipe - Input goods %s in recipe %s not found"), *GoodsInput.Name.ToString(), *Recipe.Name.ToString());
		}
	}
	BaseGoods = UGoodsFunctionLibrary::AddGoodsQuantities(TArray<FGoodsQuantity>(), TmpBaseGoods);
	return true;
}


bool URecipeManagerComponent::GetGoodsForRecipeName(const FName& RecipeName, TArray<FGoodsQuantity>& OutputGoods, const float QuantityScale, const bool bExcludeBonusGoods)
{
	AMMGameMode* GameMode = Cast<AMMGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GameMode) {
		UE_LOG(LogMMGame, Error, TEXT("RecipeManager::GetAverageGoodsFromRecipeName - Could not get GameMode"));
		return false;
	}
	bool bFound;
	FCraftingRecipe Recipe = GetRecipe(RecipeName, bFound);
	if (!bFound) { 
		UE_LOG(LogMMGame, Error, TEXT("RecipeManager::GetAverageGoodsFromRecipeName - Recipe %s not found"), *RecipeName.ToString());
		return false; 
	}
	return GetGoodsForRecipe(Recipe, OutputGoods, QuantityScale, bExcludeBonusGoods);
}

bool URecipeManagerComponent::GetGoodsForRecipe(const FCraftingRecipe& Recipe, TArray<FGoodsQuantity>& OutputGoods, const float QuantityScale, const bool bExcludeBonusGoods)
{
	AMMGameMode* GameMode = Cast<AMMGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GameMode) {
		UE_LOG(LogMMGame, Error, TEXT("RecipeManager::GetAverageGoodsFromRecipe - Could not get GameMode"));
		return false;
	}
	OutputGoods.Empty();
	FGoodsDropSet CraftingResults;
	CraftingResults.GoodsChances = Recipe.CraftingResults;
	FGoodsDropSet BonusCraftingResults;
	if (!bExcludeBonusGoods && Recipe.BonusCraftingResults.Num() > 0.f)
	{
		BonusCraftingResults.GoodsChances = Recipe.BonusCraftingResults;
		OutputGoods.Append(UGoodsFunctionLibrary::AddGoodsQuantities(
			GameMode->GetGoodsDropper()->EvaluateGoodsDropSet(CraftingResults, QuantityScale),
			GameMode->GetGoodsDropper()->EvaluateGoodsDropSet(BonusCraftingResults, QuantityScale)
		));
	}
	else
	{
		OutputGoods.Append(GameMode->GetGoodsDropper()->EvaluateGoodsDropSet(CraftingResults, QuantityScale));
	}
	return true;
}


int32 URecipeManagerComponent::CraftableCountForGoods(const TArray<FGoodsQuantity>& GoodsQuantities, const FCraftingRecipe& Recipe)
{
	int32 MinCraftings = 0;
	int32 Quantity;
	bool bFound;
	for (FGoodsQuantity Ingredient : Recipe.CraftingInputs)
	{
		Quantity = UGoodsFunctionLibrary::CountInGoodsQuantityArray(Ingredient.Name, GoodsQuantities, bFound);
		if (Quantity == 0 || Quantity < Ingredient.Quantity) {
			return 0;
		}
		if (MinCraftings == 0 || (Quantity / Ingredient.Quantity) < MinCraftings) {
			MinCraftings = Quantity / Ingredient.Quantity;
		}
	}
	return MinCraftings;
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
		//TArray<FGoodsQuantity> AllResultGoods = GameMode->GetGoodsDropper()->EvaluateGoodsDropChancePercent(ResultsDrops, 1.f);
		TArray<FGoodsQuantity> AllResultGoods;
		if (!GetGoodsForRecipe(FoundRecipe, AllResultGoods, 1.f, true)) {
			UE_LOG(LogMMGame, Error, TEXT("RecipeManager::InitCraftingRecipes - Could not get goods for recipe: %s"), *FoundRecipe.Name.ToString());
		}
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

