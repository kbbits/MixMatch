
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


int32 ARecipePlayGrid::GetRecipeLevel()
{
	AMMPlayerController* PC = Cast<AMMPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (PC)
	{
		return PC->GetRecipeLevel(GetRecipe().Name);
	}
	return 0;
}


float ARecipePlayGrid::GetChanceForIngredientBlock_Implementation()
{
	float NormalPercent = 1.f / FMath::Max(1.f, (float)(TargetBlockTypes - GetRecipe().CraftingInputs.Num()));
	float MaxPercent = NormalPercent + ((1 - NormalPercent) * 0.25f);
	return ((float)GetRecipeLevel() * 0.005f) + NormalPercent;
}


bool ARecipePlayGrid::GetRandomBlockTypeNameForCellEx_Implementation(const AMMPlayGridCell* Cell, FName& FoundBlockTypeName, const TArray<FName>& ExcludedBlockNames)
{
	// TODO: Remove this testing logic
	//if (((PlayerMovesCount / 10) + 1) % 2 == 0) { return false; }

	AMMGameMode* GameMode = Cast<AMMGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode == nullptr) {
		UE_LOG(LogMMGame, Error, TEXT("RecipePlayGrid::GetRandomBlockTypeNameForCell - Cannot get game mode"));
		return false;
	}
	FoundBlockTypeName = NAME_None;
	TArray<FGoodsQuantity> CraftingInputs = GetRecipe().CraftingInputs;
	bool bUseExclusionList = ExcludedBlockNames.Num() > 0 && FMath::FRandRange(0.f, 1.f) < DuplicateSpawnPreventionFactor;
	if (FMath::FRand() < GetChanceForIngredientBlock())
	{
		TArray<FGoodsQuantity> GoodsOdds;
		TArray<FGoodsQuantity> AllowedInputs;
		for (int32 i = 0; i < CraftingInputs.Num(); i++)
		{
			if (!bUseExclusionList || !ExcludedBlockNames.Contains(CraftingInputs[i].Name)) {
				AllowedInputs.Add(CraftingInputs[i]);
			}
		}
		// If we ended up with no allowed input ingredients, then pick a random one to add.
		if (AllowedInputs.Num() == 0) {
			AllowedInputs.Add(CraftingInputs[FMath::RandRange(0, CraftingInputs.Num() - 1)]);
		}
		for (int32 i = 0; i < AllowedInputs.Num(); i++)
		{
			FGoodsQuantity IngredientGoods = AllowedInputs[i];
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
		//UE_LOG(LogMMGame, Log, TEXT("RecipePlayGrid::GetRandomBlockTypeNameForCell - Got block type from recipe: %s"), *FoundBlockTypeName.ToString());
	}
	else
	{
		SetBlockTypeSetName(FName(FString::Printf(TEXT("Default_%d"), TargetBlockTypes - CraftingInputs.Num())));
		if (bUseExclusionList) {
			GameMode->GetRandomBlockTypeNameForCell(Cell, FoundBlockTypeName, ExcludedBlockNames);
		}
		else {
			GameMode->GetRandomBlockTypeNameForCell(Cell, FoundBlockTypeName);
		}
		//UE_LOG(LogMMGame, Log, TEXT("RecipePlayGrid::GetRandomBlockTypeNameForCell - Got block type: %s from BlockTypeSet %s"), *FoundBlockTypeName.ToString(),*FString::Printf(TEXT("Default_%d"), TargetBlockTypes - GoodsOdds.Num()));
	}
	return !FoundBlockTypeName.IsNone();
	//return Super::GetRandomBlockTypeNameForCell_Implementation(Cell, FoundBlockTypeName);
}