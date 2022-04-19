
#include "RecipeManagerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "../MixMatch.h"
#include "MMGameMode.h"
#include "SimpleNamedTypes.h"
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


TMap<FName, int32> URecipeManagerComponent::GetRecipeLevels()
{
	return RecipeLevels;
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


int32 URecipeManagerComponent::CraftingsRequiredForLevel_Implementation(const int32 RecipeLevel)
{
	return (int32)FMath::Pow(RecipeLevel - 1, 2.0f) + 1.0f;
}


float URecipeManagerComponent::GetRecipeLevelUpProgress(const FName& RecipeName)
{
	float RecipeLevel = (float)GetRecipeLevel(RecipeName);
	if (RecipeLevel <= 0) {
		return 0.f;
	}
	float CraftingsNeeded = (float) (CraftingsRequiredForLevel(RecipeLevel + 1) - CraftingsRequiredForLevel(RecipeLevel));
	if (CraftingsNeeded <= 0.0f) {
		return 1.0f;
	}
	return 1.0f - ((float)GetCraftingsRemainingForLevelUp(RecipeName) / CraftingsNeeded);
}


int32 URecipeManagerComponent::GetCraftingsRemainingForLevelUp(const FName& RecipeName)
{
	float RecipeLevel = GetRecipeLevel(RecipeName);
	if (RecipeLevel <= 0) {
		return 1;
	}
	return CraftingsRequiredForLevel(RecipeLevel + 1) - GetRecipeCraftingCount(RecipeName);
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

TMap<FName, int32> URecipeManagerComponent::GetRecipeCraftingCounts()
{
	return RecipeCraftings;
}

int32 URecipeManagerComponent::GetTargetBlockTypeCount(const FCraftingRecipe& Recipe)
{
	if (!Recipe.Name.IsNone()) {
		return Recipe.TargetBlockTypeCount;
	}
	return 4;
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


float URecipeManagerComponent::GetValueForGoods(const FName& GoodsName)
{
	return CalculateValueForGoods(GoodsName, GoodsName);
}


float URecipeManagerComponent::GetValueForRecipe(const FName& RecipeName)
{
	bool bFound;
	FCraftingRecipe Recipe = GetRecipe(RecipeName, bFound);
	if (bFound)
	{
		float TotalValue = 0.0f;
		TArray<FGoodsQuantity> RecipeGoods;		
		GetGoodsForRecipe(Recipe, RecipeGoods, 0.5f, true);
		for (FGoodsQuantity Goods : RecipeGoods) {
			TotalValue += GetValueForGoods(Goods.Name) * Goods.Quantity;
		}
		return TotalValue;
	}
	else {
		return 0.0f;
	}
}


float URecipeManagerComponent::CalculateValueForGoods(const FName& GoodsName, const FName& TopGoodsToCheck)
{
	bool bFound;
	AMMGameMode* GameMode = Cast<AMMGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GameMode)
	{
		UE_LOG(LogMMGame, Error, TEXT("RecipeManager::CalculateValueForGoods - Could not get GameMode"));
		return 0.0f;
	}
	FGoodsType GoodsType = GameMode->GetGoodsData(GoodsName, bFound);
	if (!bFound)
	{
		UE_LOG(LogMMGame, Error, TEXT("RecipeManager::CalculateValueForGoods - Goods type not found %s"), *GoodsName.ToString());
		return 0.0f;
	}
	if (GoodsType.OverrideValue > 0.0f) {
		return GoodsType.OverrideValue;
	}
	FCraftingRecipe ProducingRecipe = GetRecipeForGoodsName(GoodsType.Name, bFound);
	TMap<FName, float> IngredientValueScaleMap;
	// Handle the ScaleInputValue directive in recipe category field.
	for (FName RecipeTag : ProducingRecipe.RecipeCategories) 
	{
		FString TagString = RecipeTag.ToString();
		if (TagString.StartsWith(FString(TEXT("ScaleInputValue"))))
		{
			TArray<FString> params;
			TagString.ParseIntoArray(params, *FString(TEXT(":")), true);
			if (params.Num() == 3)
			{
				float scale = FCString::Atof(*params[2]);
				IngredientValueScaleMap.Add(FName(params[1]), scale);
			}
		}
	}
	// Handle ValueIngoreProducedFrom directive in goods tags field.
	for (FName RecipeTag : GoodsType.GoodsTags) 
	{
		FString TagString = RecipeTag.ToString();
	    // ValueIgnoreProducedFrom:BleachingCake2Cleanse
		if (TagString.StartsWith(FString(TEXT("ValueIgnoreProducedFrom"))))
		{
			TArray<FString> params;
			TagString.ParseIntoArray(params, *FString(TEXT(":")), true);
			if (params.Num() == 2) {
				IngredientValueScaleMap.Add(FName(params[1]), 0.0f);
			}
		}
	}
	TArray<FGoodsQuantity> GoodsProducedByProducingRecipe;
	GetGoodsForRecipe(ProducingRecipe, GoodsProducedByProducingRecipe, 0.5f, true);
	for (FGoodsQuantity ProducedGoods : GoodsProducedByProducingRecipe)
	{
		// Find the quantity of goods we're looking for produced by the producing recipe
		if (ProducedGoods.Name == GoodsType.Name && ProducedGoods.Quantity > 0.f)
		{
			float TotalValue = 0.0f;
			for (FGoodsQuantity IngredientGoods : ProducingRecipe.CraftingInputs)
			{
				float IngredientValue = CalculateValueForGoods(IngredientGoods.Name, TopGoodsToCheck) * IngredientGoods.Quantity;
				if (IngredientValueScaleMap.Contains(IngredientGoods.Name)) {
					IngredientValue *= IngredientValueScaleMap[IngredientGoods.Name];
				}
				TotalValue += IngredientValue;
				if (IngredientGoods.Name == TopGoodsToCheck) {
					UE_LOG(LogMMGame, Warning, TEXT("RecipeManagerComponent::CalculateValueForGoods - Found circular recipe chain with %s"), *TopGoodsToCheck.ToString());
				}
			}
			return (TotalValue * GameMode->ValueTierMultiplier) / ProducedGoods.Quantity;
		}
	}
	UE_LOG(LogMMGame, Warning, TEXT("RecipeManagerComponent::CalculateValueForGoods - Found no recipe producing %s"), *GoodsName.ToString());
	return 0.0f;
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
	float TotalExperience = 0.f;
	bool bFound;
	for (FGoodsQuantity IngredientRequired : Recipe.CraftingInputs)
	{
		FCraftingRecipe IngredientRequiredRecipe = GetRecipeForGoodsName(IngredientRequired.Name, bFound);
		if (IngredientRequiredRecipe.Name == TopRecipeToCheck) {
			UE_LOG(LogMMGame, Warning, TEXT("RecipeManagerComponent::CalculateExperienceForRecipe - Found circular recipe chain with %s"), *TopRecipeToCheck.ToString());
			return 0.0f;
		}
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
	AMMGameMode* GameMode = Cast<AMMGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GameMode)
	{
		UE_LOG(LogMMGame, Error, TEXT("RecipeManager::CalculateExperienceForRecipe - Could not get GameMode"));
		return FMath::TruncToInt(TotalExperience);
	}
	return FMath::TruncToInt(TotalExperience * GameMode->ExperienceTierMultiplier);
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


bool URecipeManagerComponent::GetSaveData(FPlayerSaveData& SaveData)
{
	IntMapToNamedArray(GetRecipeCraftingCounts(), SaveData.TotalRecipesCrafted);
	IntMapToNamedArray(GetRecipeLevels(), SaveData.RecipeLevels);
	return true;
}


void URecipeManagerComponent::UpdateFromSaveData(const FPlayerSaveData& SaveData)
{
	RecipeCraftings.Empty();
	RecipeCraftings.Append(NamedQuantitiesToCountMap(SaveData.TotalRecipesCrafted));
	RecipeLevels.Empty();
	RecipeLevels.Append(NamedQuantitiesToCountMap(SaveData.RecipeLevels));
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
	TArray<FSoftObjectPath> AssetsToCache;
	// Get recipe data
	AllRecipeData.Empty(CraftingRecipesTable->GetRowMap().Num());
	for (const TPair<FName, uint8*>& It : CraftingRecipesTable->GetRowMap())
	{
		FCraftingRecipe FoundRecipe = *reinterpret_cast<FCraftingRecipe*>(It.Value);
		//if (!FoundRecipe.Name.IsNone() && !RecipeLevels.Contains(FoundRecipe.Name)) {
		//	RecipeLevels.Add(FoundRecipe.Name, 0);
		//}
		AllRecipeData.Add(It.Key, FoundRecipe);
		AssetsToCache.AddUnique(FoundRecipe.Thumbnail.ToSoftObjectPath());
		AssetsToCache.AddUnique(FoundRecipe.CraftSound.ToSoftObjectPath());
		// Fill in the GoodsToRecipeMap
		TArray<FGoodsQuantity> AllResultGoods;
		if (!GetGoodsForRecipe(FoundRecipe, AllResultGoods, 1.f, true)) {
			UE_LOG(LogMMGame, Error, TEXT("RecipeManager::InitCraftingRecipes - Could not get goods for recipe: %s"), *FoundRecipe.Name.ToString());
		}
		for (FGoodsQuantity ResultGoods : AllResultGoods)
		{
			if (GoodsToRecipeMap.Contains(ResultGoods.Name))
			{
				// If we find multiple recipes producing this goods type, use the lowest tier recipe.
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
	GameMode->CacheAssets(AssetsToCache, FName(TEXT("Recipes")));
}

