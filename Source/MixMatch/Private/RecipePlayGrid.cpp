
#include "RecipePlayGrid.h"
#include "Kismet/GameplayStatics.h"
#include "..\MixMatch.h"
#include "MMGameMode.h"
#include "MMPlayerController.h"
#include "Goods\GoodsFunctionLibrary.h"
#include "CraftingRecipe.h"
#include "RecipeManagerComponent.h"

ARecipePlayGrid::ARecipePlayGrid()
{
}


void ARecipePlayGrid::SetRecipeManager(URecipeManagerComponent* NewRecipeManager)
{
	RecipeManager = NewRecipeManager;
}

URecipeManagerComponent* ARecipePlayGrid::GetRecipeManager()
{
	if (IsValid(RecipeManager)) { return RecipeManager; }
	AMMPlayerController* PController = Cast<AMMPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (PController) {
		RecipeManager = PController->RecipeManager;
	}
	return RecipeManager;
}


void ARecipePlayGrid::SetRecipe(FCraftingRecipe& Recipe)
{
	CurrentRecipe = Recipe;
}


bool ARecipePlayGrid::SetRecipeByName(const FName& RecipeName)
{
	if (!IsValid(GetRecipeManager())) {
		UE_LOG(LogMMGame, Error, TEXT("RecipePlayGrid::SetRecipeByName - RecipeManager is null"));
		return false;
	}
	bool bFound;
	CurrentRecipe = GetRecipeManager()->GetRecipe(RecipeName, bFound);
	return bFound;
};


const FCraftingRecipe& ARecipePlayGrid::GetRecipe()
{
	return CurrentRecipe;
}


bool ARecipePlayGrid::GetRandomBlockTypeNameForCell_Implementation(const AMMPlayGridCell* Cell, FName& FoundBlockTypeName)
{
	AMMGameMode* GameMode = Cast<AMMGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode == nullptr) {
		UE_LOG(LogMMGame, Error, TEXT("RecipePlayGrid::GetRandomBlockTypeNameForCell - Cannot get game mode"));
		return false;
	}
	FoundBlockTypeName = NAME_None;
	TArray<FGoodsQuantity> GoodsOdds;
	for (FGoodsQuantity IngredientGoods : GetRecipe().CraftingInputs)
	{
		bool bFound;
		FCraftingRecipe IngredientRecipe = GetRecipeManager()->GetRecipeForGoodsName(IngredientGoods.Name, bFound);
		if (bFound)
		{
			TArray<FGoodsQuantity> RecipeGoods;
			if (GetRecipeManager()->GetGoodsForRecipe(IngredientRecipe, RecipeGoods, 0.5, true))
			{
				float ProductionRatio = IngredientGoods.Quantity / UGoodsFunctionLibrary::CountInGoodsQuantityArray(IngredientGoods.Name, RecipeGoods, bFound);
				GoodsOdds.Add(FGoodsQuantity(IngredientGoods.Name, FMath::Max(1.f, ProductionRatio)));
			}
		}
	}
	if (FMath::FRand() < 1 / FMath::Max(1.f, (float)(TargetBlockTypes - GoodsOdds.Num())))
	{
		float TotalWeight = 0.f;
		for (FGoodsQuantity IngredientGoods : GoodsOdds)
		{
			TotalWeight += IngredientGoods.Quantity;
		}
		float WeightSum = 0.f;
		float PickedWeight = FMath::RandRange(0.f, TotalWeight);
		for (FGoodsQuantity IngredientGoods : GoodsOdds)
		{
			WeightSum += IngredientGoods.Quantity;
			if (WeightSum >= PickedWeight)
			{
				FoundBlockTypeName = IngredientGoods.Name;
				break;
			}
		}
		UE_LOG(LogMMGame, Log, TEXT("RecipePlayGrid::GetRandomBlockTypeNameForCell - Got block type from recipe: %s"), *FoundBlockTypeName.ToString());
	}
	else
	{
		SetBlockTypeSetName(FName(FString::Printf(TEXT("Default_%d"), TargetBlockTypes - GoodsOdds.Num())));
		GameMode->GetRandomBlockTypeNameForCell(Cell, FoundBlockTypeName);
		UE_LOG(LogMMGame, Log, TEXT("RecipePlayGrid::GetRandomBlockTypeNameForCell - Got block type: %s from BlockTypeSet %s"), *FoundBlockTypeName.ToString(),*FString::Printf(TEXT("Default_%d"), TargetBlockTypes - GoodsOdds.Num()));
	}
	return !FoundBlockTypeName.IsNone();
	//return Super::GetRandomBlockTypeNameForCell_Implementation(Cell, FoundBlockTypeName);
}