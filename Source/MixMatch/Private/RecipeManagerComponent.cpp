
#include "RecipeManagerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "../MixMatch.h"
#include "MMGameMode.h"
#include "Goods/Goods.h"
#include "Goods/GoodsDropper.h"


// Sets default values for this component's properties
URecipeManagerComponent::URecipeManagerComponent()
{
	SetIsReplicated(false);
	SetIsReplicatedByDefault(false);
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	//Defaults
	ExperienceTierMultiplier = 1.1f;
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

FCraftingRecipe URecipeManagerComponent::GetRecipe(const FName& RecipeName)
{
	bool bFound;
	return GetRecipe(RecipeName, bFound);
}


FCraftingRecipe URecipeManagerComponent::GetRecipeForGoodsName(const FName& GoodsName, bool& bFound)
{
	FName* ProducingRecipeName = GoodsToRecipeMap.Find(GoodsName);
	if (ProducingRecipeName != nullptr) {
		return GetRecipe(*ProducingRecipeName, bFound);
	}
	UE_LOG(LogMMGame, Error, TEXT("RecipeManager::GetRecipeForGoodsName - No producing recipe found for goods name: %s"), *GoodsName.ToString());
	bFound = false;
	return FCraftingRecipe();
}


TArray<FCraftingRecipe> URecipeManagerComponent::GetRecipesWithCategory(const FName& Category, const bool bUnlockedRecipesOnly)
{
	const TArray<FName> TmpCats = { Category };
	return GetRecipesWithCategories(TmpCats, bUnlockedRecipesOnly);
}


TArray<FCraftingRecipe> URecipeManagerComponent::GetRecipesWithCategories(const TArray<FName>& Categories, const bool bUnlockedRecipesOnly)
{
	InitCraftingRecipes();
	TArray<FCraftingRecipe> FoundRecipes;
	for (TPair<FName, FCraftingRecipe> It : AllRecipeData)
	{
		if (!bUnlockedRecipesOnly || IsRecipeUnlocked(It.Key))
		{
			for (FName CurCategory : Categories)
			{
				if (It.Value.RecipeCategories.Contains(CurCategory))
				{
					FoundRecipes.Add(It.Value);
					break;
				}
			}
		}
	}
	return FoundRecipes;
}


int32 URecipeManagerComponent::GetRecipeLevel(const FName& RecipeName)
{
	if (RecipeLevels.Contains(RecipeName)) {
		return RecipeLevels[RecipeName];
	}
	return 0;
}


int32 URecipeManagerComponent::IncrementRecipeLevel(const FName& RecipeName, const int32 IncrementAmount)
{
	int32 NewLevel = GetRecipeLevel(RecipeName) + IncrementAmount;
	SetRecipeLevel(RecipeName, NewLevel);
	return NewLevel;
}


void URecipeManagerComponent::SetRecipeLevel(const FName& RecipeName, const int32 NewLevel)
{
	int32 OldLevel = 0;
	if (RecipeLevels.Contains(RecipeName)) 
	{
		OldLevel = RecipeLevels[RecipeName];
		if (OldLevel != NewLevel) {
			RecipeLevels[RecipeName] = NewLevel;
		}
	}
	else {
		RecipeLevels.Add(RecipeName, NewLevel);
	}
	OnRecipeLevelChanged.Broadcast(GetRecipe(RecipeName), NewLevel, OldLevel);
}


float URecipeManagerComponent::GetRecipeLevelUpProgress_Implementation(const FName& RecipeName)
{
	float RecipeLevel = (float)GetRecipeLevel(RecipeName);
	if (RecipeLevel <= 0) {
		return 0.f;
	}
	float LastCraftingsNeeded = 0.f;
	float CraftingsNeeded = FMath::Pow(RecipeLevel, 2.f) + 1.f;
	if (RecipeLevel > 1) {
		LastCraftingsNeeded = FMath::Pow(RecipeLevel - 1.f, 2.f) + 1.f;
	}
	return (((float)GetRecipeCraftingCount(RecipeName)) - LastCraftingsNeeded) / (CraftingsNeeded - LastCraftingsNeeded);
}


bool URecipeManagerComponent::IsRecipeReadyForLevelUp(const FName& RecipeName)
{
	return GetRecipeLevelUpProgress(RecipeName) >= 1.f;
}


bool URecipeManagerComponent::IsRecipeUnlocked(const FName& RecipeName)
{
	return GetRecipeLevel(RecipeName) > 0;
}


int32 URecipeManagerComponent::IncrementRecipeCraftingCount(const FName& RecipeName, const int32 IncrementAmount)
{
	int32& CurTotal = RecipeCraftings.FindOrAdd(RecipeName);	
	CurTotal += IncrementAmount;
	while (IsRecipeReadyForLevelUp(RecipeName)) {
		IncrementRecipeLevel(RecipeName);
	}	
	return CurTotal;
}


int32 URecipeManagerComponent::GetRecipeCraftingCount(const FName& RecipeName)
{
	int32* CurTotal = RecipeCraftings.Find(RecipeName);
	if (CurTotal) {
		return *CurTotal;
	}
	return 0;
}


bool URecipeManagerComponent::GetBaseIngredientsForRecipe(const FName& RecipeName, TArray<FGoodsQuantity>& BaseGoods)
{
	AMMGameMode* GameMode = Cast<AMMGameMode>(UGameplayStatics::GetGameMode(this));
	TArray<FGoodsQuantity> TmpBaseGoods;
	FCraftingRecipe Recipe;
	bool bFound;
	BaseGoods.Empty();
	if (!GameMode) 
	{
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
			if (InputGoodsData.GoodsTags.Contains(FGoodsTags::Resource)) 
			{
				// Goods tagged as "Resource" are base goods
				TmpBaseGoods.Add(GoodsInput);
			}
			else
			{
				// Go down the chain, get the goods required to produce this good
				FName* ProducingRecipeName = GoodsToRecipeMap.Find(InputGoodsData.Name);
				if (ProducingRecipeName)
				{
					TArray<FGoodsQuantity> ProducingRecipeGoods;
					// RECURSION
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
	if (!GameMode) 
	{
		UE_LOG(LogMMGame, Error, TEXT("RecipeManager::GetAverageGoodsFromRecipeName - Could not get GameMode"));
		return false;
	}
	bool bFound;
	FCraftingRecipe Recipe = GetRecipe(RecipeName, bFound);
	if (!bFound) 
	{ 
		UE_LOG(LogMMGame, Error, TEXT("RecipeManager::GetAverageGoodsFromRecipeName - Recipe %s not found"), *RecipeName.ToString());
		return false; 
	}
	return GetGoodsForRecipe(Recipe, OutputGoods, QuantityScale, bExcludeBonusGoods);
}

bool URecipeManagerComponent::GetGoodsForRecipe(const FCraftingRecipe& Recipe, TArray<FGoodsQuantity>& OutputGoods, const float QuantityScale, const bool bExcludeBonusGoods)
{
	AMMGameMode* GameMode = Cast<AMMGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GameMode) 
	{
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
	else{
		OutputGoods.Append(GameMode->GetGoodsDropper()->EvaluateGoodsDropSet(CraftingResults, QuantityScale));
	}
	return true;
}


int32 URecipeManagerComponent::GetExperienceForRecipe(const FCraftingRecipe& Recipe)
{
	return CalculateExperienceForRecipe(Recipe, Recipe.Name);
}


int32 URecipeManagerComponent::CalculateExperienceForRecipe(const FCraftingRecipe& Recipe, const FName& TopRecipeToCheck)
{
	if (Recipe.Tier <= 1) {
		return Recipe.OverrideCraftingExperience;
	}
	if (TopRecipeToCheck == Recipe.Name)
	{
		UE_LOG(LogMMGame, Warning, TEXT("RecipeManagerComponent::CalculateExperienceForRecipe - Found circular recipe chain with %s"), *TopRecipeToCheck.ToString());
		return 0;
	}
	float TotalExperience = 0.f;
	bool bFound;
	for (FGoodsQuantity IngredientRequired : Recipe.CraftingInputs)
	{
		FCraftingRecipe IngredientRequiredRecipe = GetRecipeForGoodsName(IngredientRequired.Name, bFound);
		TArray<FGoodsQuantity> GoodsProducedByRequirementRecipe;
		GetGoodsForRecipe(IngredientRequiredRecipe, GoodsProducedByRequirementRecipe, 0.5f, true);
		for (FGoodsQuantity GoodProducedByRequirement : GoodsProducedByRequirementRecipe)
		{
			if (IngredientRequired.Name == GoodProducedByRequirement.Name && GoodProducedByRequirement.Quantity > 0.f)
			{
				float IngredientExperience = CalculateExperienceForRecipe(IngredientRequiredRecipe, TopRecipeToCheck);
				IngredientExperience = IngredientExperience * (IngredientRequired.Quantity / GoodProducedByRequirement.Quantity);
				TotalExperience += IngredientExperience;
				break;
			}
		}
	}
	return FMath::TruncToInt(TotalExperience * ExperienceTierMultiplier);
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
	if (!GameMode) 
	{
		UE_LOG(LogMMGame, Error, TEXT("RecipeManager::InitCraftingRecipes - Could not get GameMode"));
		return;
	}
	if (!IsValid(CraftingRecipesTable)) 
	{
		UE_LOG(LogMMGame, Error, TEXT("RecipeManager::InitCraftingRecipes - CraftingRecipesTable is not valid"));
		return;
	}
	bool bFound;
	// Get recipe data
	AllRecipeData.Empty(CraftingRecipesTable->GetRowMap().Num());
	for (const TPair<FName, uint8*>& It : CraftingRecipesTable->GetRowMap())
	{
		FCraftingRecipe FoundRecipe = *reinterpret_cast<FCraftingRecipe*>(It.Value);
		//if (!FoundRecipe.Name.IsNone() && !RecipeLevels.Contains(FoundRecipe.Name)) {
		//	RecipeLevels.Add(FoundRecipe.Name, 0);
		//}
		AllRecipeData.Add(It.Key, FoundRecipe);
		// Fill in the GoodsToRecipeMap
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
			else {
				GoodsToRecipeMap.Add(ResultGoods.Name, FoundRecipe.Name);
			}
		}
	}
}

