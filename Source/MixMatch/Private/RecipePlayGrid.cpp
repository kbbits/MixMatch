
#include "RecipePlayGrid.h"
#include "Kismet/GameplayStatics.h"
#include "..\MixMatch.h"
#include "BlockType.h"
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
	// Grab player's recipe manager if one has not been set.
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
	if (bIngredientsFromInventory && GetRecipeManager()->CraftableCountForGoods(GoodsInventory->Inventory, GetRecipe()) <= 0) return 0.f;
	float NormalPercent = 1.f / FMath::Max(1.f, (float)(TargetBlockTypes - GetRecipe().CraftingInputs.Num()));
	float MaxPercent = NormalPercent + ((1 - NormalPercent) * 0.25f);
	return ((float)GetRecipeLevel() * 0.005f) + NormalPercent;
}


// Note: if bIngredientsFromInventory == true, ingredients produced by the chosen block type 
// will be deducted from inventory when the block is added to the grid.
//  @see ARecipePlayGrid::AddRandomBlockInCellEx
bool ARecipePlayGrid::GetRandomBlockTypeNameForCellEx_Implementation(const AMMPlayGridCell* Cell, FName& FoundBlockTypeName, const TArray<FName>& ExcludedBlockNames)
{
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
		// Determine which ingredient goods are not excluded
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
		// Determine the odds of each ingredient type that could be picked
		for (int32 i = 0; i < AllowedInputs.Num(); i++)
		{
			bool bFound;
			FGoodsQuantity IngredientGoods = AllowedInputs[i];
			// Get the recipe that produces the ingredient goods
			FCraftingRecipe IngredientRecipe = GetRecipeManager()->GetRecipeForGoodsName(IngredientGoods.Name, bFound);
			if (bFound)
			{
				TArray<FGoodsQuantity> RecipeGoods;
				if (GetRecipeManager()->GetGoodsForRecipe(IngredientRecipe, RecipeGoods, 0.5, true))
				{
					// Determine the ratio of ingredient goods produced by it's producing recipe to the quantity required by the
					// recipe being crafted. This ratio will be the ingredient's weight in our weighted list pick.
					// i.e. The more ingredients this recipe needs compared to how many the recipe that produces the goods, the higher the chance 
					// the ingredient will be picked.
					// TODO: Rework this rule
					float ProductionRatio = IngredientGoods.Quantity / UGoodsFunctionLibrary::CountInGoodsQuantityArray(IngredientGoods.Name, RecipeGoods, bFound);
					GoodsOdds.Add(FGoodsQuantity(IngredientGoods.Name, FMath::Max(0.5f, ProductionRatio)));
				}
			}
		}
		// Pick from our weighted list of GoodsOods
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
		if (FoundBlockTypeName.IsNone())
		{
			UE_LOG(LogMMGame, Error, TEXT("RecipePlayGrid::GetRandomBlockTypeNameForCell - Unable to pick a recipe from weighted list. Number of list entries: %d  Weighted sum: %d"), GoodsOdds.Num(), TotalWeight);
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


AMMBlock* ARecipePlayGrid::AddRandomBlockInCellEx(AMMPlayGridCell* Cell, const TArray<FName>& ExcludedBlockNames, const float OffsetAboveCell, const bool bAllowUnsettle, const bool bPreventMatches)
{
	// Call base class first
	AMMBlock* NewBlock = Super::AddRandomBlockInCellEx(Cell, ExcludedBlockNames, OffsetAboveCell, bAllowUnsettle, bPreventMatches);

	// TODO: Remove this debug
	if (BlockCategory::Goods == NAME_None) {
		UE_LOG(LogMMGame, Error, TEXT("BlockCategory::Goods is not defined."));
	}

	// Check NewBLock for ingredient logic
	if (NewBlock && bIngredientsFromInventory && NewBlock->HasCategory(BlockCategory::Goods))
	{
		AMMGameMode* GameMode = Cast<AMMGameMode>(UGameplayStatics::GetGameMode(this));
		check(GameMode);
		if (GameMode) 
		{
			FName BlockTypeName = NewBlock->GetBlockType().Name;
			for (FGoodsQuantity GQ : GetRecipe().CraftingInputs)
			{
				if (GQ.Name == BlockTypeName)
				{
					// If this block type's name matches a recipe ingredient name, determine what quantities of ingredient 
					// goods the block will produce on a normal match.
					TArray<FGoodsQuantity> BlockAwardGoods = NewBlock->GetBaseMatchGoods(GameMode->GetGoodsDropper());
					TArray<FGoodsQuantity> IngredientAwardGoods = UGoodsFunctionLibrary::CountsInGoodsQuantities(GetRecipe().CraftingInputs, BlockAwardGoods);
					// Subtract block's ingredient match goods from grid's inventory.
					if (!GoodsInventory->AddSubtractGoodsArray(IngredientAwardGoods, true)) {
						UE_LOG(LogMMGame, Warning, TEXT("RecipePlayGrid::AddRandomBlockInCellEx - cannot deduct goods from grid inventory for block %s with block type %s"), *NewBlock->GetName(), *NewBlock->GetBlockType().Name.ToString());
					}
					break;
				}
			}
		}
	}
	return NewBlock;
}