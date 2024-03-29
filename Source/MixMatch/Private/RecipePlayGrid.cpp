
#include "RecipePlayGrid.h"
#include "Kismet/GameplayStatics.h"
#include "..\MixMatch.h"
#include "BlockType.h"
#include "MMGameMode.h"
#include "MMPlayerController.h"
#include "Goods\Goods.h"
#include "Goods\GoodsDropper.h"
#include "CraftingRecipe.h"
#include "RecipeManagerComponent.h"

ARecipePlayGrid::ARecipePlayGrid()
{
	NonIngredientBlockTypeSetBaseName = FString(TEXT("Default"));
	// Input Inventory
	InputGoodsInventory = CreateDefaultSubobject<UInventoryActorComponent>(TEXT("InputGoodsInventory"));
	if (InputGoodsInventory) {
		AddOwnedComponent(InputGoodsInventory);
	}
}


void  ARecipePlayGrid::StartPlayGrid_Implementation()
{
	InitIngredientBlockDropOdds(true);
	Super::StartPlayGrid_Implementation();
}


void ARecipePlayGrid::StopPlayGrid_Implementation()
{
	// Don't do base class stuff. Insead:
	// Give back unused ingredient inventory.
	AMMPlayerController* PC = Cast<AMMPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (PC)
	{
		TArray<FGoodsQuantity> RemainingIngredients;
		InputGoodsInventory->GetAllGoods(RemainingIngredients);
		PC->GoodsInventory->AddSubtractGoodsArray(RemainingIngredients, false);
		InputGoodsInventory->ClearAllInventory();
		GoodsInventory->ClearAllInventory();
	}
	// Tell game mode to end all effects since grid play has stopped.
	AMMGameMode* GameMode = Cast<AMMGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode) {
		GameMode->EndAllActiveGameEffects();
	}
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
	NonIngredientBlockTypeSetBaseName = Recipe.BlockTypeSetBase;
	if (GetRecipeManager()) {
		TargetBlockTypes = GetRecipeManager()->GetTargetBlockTypeCount(CurrentRecipe);
	}
	InitIngredientBlockDropOdds(true);
}


bool ARecipePlayGrid::SetRecipeByName(const FName& RecipeName)
{
	if (!IsValid(GetRecipeManager())) {
		UE_LOG(LogMMGame, Error, TEXT("RecipePlayGrid::SetRecipeByName - RecipeManager is null"));
		return false;
	}
	bool bFound;
	FCraftingRecipe FoundRecipe = GetRecipeManager()->GetRecipe(RecipeName, bFound);
	if (bFound) {
		SetRecipe(FoundRecipe);
	}
	return bFound;
};


const FCraftingRecipe& ARecipePlayGrid::GetRecipe()
{
	return CurrentRecipe;
}


int32 ARecipePlayGrid::GetRecipeLevel()
{
	AMMPlayerController* PC = Cast<AMMPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (PC) {
		return PC->GetRecipeLevel(GetRecipe().Name);
	}
	return 0;
}


bool ARecipePlayGrid::CraftRecipe(const FCraftingRecipe& Recipe)
{
	AMMPlayerController* PC = Cast<AMMPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (PC && GoodsInventory->AddSubtractGoodsArray(Recipe.CraftingInputs, true, true))
	{
		TArray<FGoodsQuantity> CraftedGoods;
		if (RecipeManager->GetGoodsForRecipe(Recipe, CraftedGoods))
		{
			PC->GoodsInventory->AddSubtractGoodsArray(CraftedGoods, false, true);
			PC->RecipeManager->IncrementRecipeCraftingCount(Recipe.Name);
			PC->OnRecipeCrafted.Broadcast(Recipe, 1);
			return true;
		}
	}
	return false;
}


float ARecipePlayGrid::GetChanceForIngredientBlock_Implementation()
{
	if (bIngredientsFromInventory && (IngredientBlockDropOdds.Num() == 0 || InputGoodsInventory->IsEmpty())) return 0.f;
	float NormalPercent = ((float)GetRecipe().CraftingInputs.Num()) / FMath::Max(1.f, (float)(TargetBlockTypes));
	NormalPercent *= IngredientDropFactor;
	float MaxPercent = NormalPercent + ((1 - NormalPercent) * 0.5f);
	return FMath::Min(((float)GetRecipeLevel() * 0.005f) + NormalPercent, MaxPercent);
}


float ARecipePlayGrid::GetIngredientBlockRelativeChance(const FName& GoodsName)
{
	InitIngredientBlockDropOdds();
	float TotalWeight = 0.0f;
	float GoodsWeight = 0.0f;
	for (FGoodsQuantity GoodsOdds : IngredientBlockDropOdds)
	{
		TotalWeight += GoodsOdds.Quantity;
		if (GoodsOdds.Name == GoodsName) {
			GoodsWeight = GoodsOdds.Quantity;
		}
	}
	if (TotalWeight <= 0.0f) {
		return 0.0f;
	}
	return GoodsWeight / TotalWeight;
}


// Note: if bIngredientsFromInventory == true, ingredients produced by the chosen block type 
// will be deducted from inventory when the block is added to the grid.
//  @see ARecipePlayGrid::AddRandomBlockInCellEx
bool ARecipePlayGrid::GetRandomBlockTypeNameForCell_Implementation(FName& FoundBlockTypeName, const FAddBlockContext& BlockContext)
{
	AMMGameMode* GameMode = Cast<AMMGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode == nullptr) {
		UE_LOG(LogMMGame, Error, TEXT("RecipePlayGrid::GetRandomBlockTypeNameForCell - Cannot get game mode"));
		return false;
	}
	InitIngredientBlockDropOdds();
	FoundBlockTypeName = NAME_None;
	bool bUseExclusionList = BlockContext.ExcludedBlockNames.Num() > 0 && FMath::FRandRange(0.f, 1.f) < DuplicateSpawnPreventionFactor;
	if (FMath::FRand() < GetChanceForIngredientBlock())
	{
		// Determine which ingredient goods are not excluded
		TArray<FGoodsQuantity> AllowedInputs;
		// Iterate through the cached block drop odds for the grid's recipe
		for (int32 i = 0; i < IngredientBlockDropOdds.Num(); i++)
		{
			if (!bUseExclusionList || !BlockContext.ExcludedBlockNames.Contains(IngredientBlockDropOdds[i].Name)) {
				AllowedInputs.Add(IngredientBlockDropOdds[i]);
			}
		}
		// Only do this if we're not using inventory goods or we have at least one that we have inventory for
		if (!bIngredientsFromInventory || IngredientBlockDropOdds.Num() > 0)
		{
			// If we ended up with no allowed input ingredients, then pick a random one from our list of ingredients we have inventory for.
			if (AllowedInputs.Num() == 0) {
				AllowedInputs.Add(IngredientBlockDropOdds[FMath::RandRange(0, IngredientBlockDropOdds.Num() - 1)]);
			}

			float TotalWeight = 0.f;
			if (AllowedInputs.Num() == 1) {
				FoundBlockTypeName = AllowedInputs[0].Name;
			}
			else {
				// Pick from our weighted list of GoodsOods
				for (FGoodsQuantity IngredientGoods : AllowedInputs)
				{
					TotalWeight += IngredientGoods.Quantity;
				}
				float WeightSum = 0.f;
				float PickedWeight = FMath::RandRange(0.f, TotalWeight);
				for (FGoodsQuantity IngredientGoods : AllowedInputs)
				{
					WeightSum += IngredientGoods.Quantity;
					if (WeightSum >= PickedWeight)
					{
						FoundBlockTypeName = IngredientGoods.Name;
						break;
					}
				}
			}
			if (FoundBlockTypeName.IsNone())
			{
				UE_LOG(LogMMGame, Error, TEXT("RecipePlayGrid::GetRandomBlockTypeNameForCell - Unable to pick a recipe from weighted list. Number of list entries: %d  Weighted sum: %d"), IngredientBlockDropOdds.Num(), TotalWeight);
			}
			UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("RecipePlayGrid::GetRandomBlockTypeNameForCell - Got block type % from recipe: %s"), *FoundBlockTypeName.ToString(), *GetRecipe().Name.ToString());
		}
	}
	// If we haven't found a block name yet, do it via game mode.
	if(FoundBlockTypeName.IsNone())
	{
		// TODO: Improve the table name determination logic.
		// Currently we will set the BlockTypeSetName to a block type set that has no ingredient blocks and has only the number of block types 
		// normally available to a recipe with a given number of ingredients.
		// i.e. TargetBlockTypes - CraftingInputs.Num()
		//FName UseBlockTypeSetName = FName(FString::Printf(TEXT("%s_%d"), *NonIngredientBlockTypeSetBaseName, FMath::Max(0, TargetBlockTypes - GetRecipe().CraftingInputs.Num())));
		SetBlockTypeSetName(FName(NonIngredientBlockTypeSetBaseName));
		if (bUseExclusionList) {
			GameMode->GetRandomBlockTypeNameForCell(FoundBlockTypeName, BlockContext);
		}
		else {
			FAddBlockContext TmpContext = BlockContext;
			TmpContext.ExcludedBlockNames.Empty();
			GameMode->GetRandomBlockTypeNameForCell(FoundBlockTypeName, TmpContext);
		}
		UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("RecipePlayGrid::GetRandomBlockTypeNameForCell - Got block type: %s from BlockTypeSet %s"), *FoundBlockTypeName.ToString(),*GetBlockTypeSetName().ToString());
	}
	return !FoundBlockTypeName.IsNone();
}

/** Override the base class so we can drop "ingredient blocks" based on the grid's recipe. */
AMMBlock* ARecipePlayGrid::AddRandomBlockInCell(const FAddBlockContext& BlockContext)
{
	// Call base class first
	AMMBlock* NewBlock = Super::AddRandomBlockInCell(BlockContext);

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
					TArray<FGoodsQuantity> BlockAwardGoods = NewBlock->GetBaseMatchGoods(GameMode->GetGoodsDropper(), 0.5f);
					TArray<FGoodsQuantity> IngredientAwardGoods = UGoodsFunctionLibrary::CountsInGoodsQuantities(GetRecipe().CraftingInputs, BlockAwardGoods);
					// Subtract block's ingredient match goods from grid's inventory.
					if (!InputGoodsInventory->AddSubtractGoodsArray(IngredientAwardGoods, true)) {
						UE_LOG(LogMMGame, Warning, TEXT("RecipePlayGrid::AddRandomBlockInCell - cannot deduct goods from grid input inventory for block %s with block type %s"), *NewBlock->GetName(), *NewBlock->GetBlockType().Name.ToString());
					}
					int32 Index = IngredientBlockDropOdds.IndexOfByKey(BlockTypeName);
					if (Index != INDEX_NONE && !InputGoodsInventory->HasAllGoods(IngredientAwardGoods)) {
						IngredientBlockDropOdds.RemoveAt(Index);
					}
					if (InputGoodsInventory->IsEmpty() || IngredientBlockDropOdds.Num() == 0) {
						bPauseNewBlocks = true;
						IngredientBlockDropOdds.Empty();
					}
					break;
				}
			}
		}
	}
	return NewBlock;
}


void ARecipePlayGrid::InitIngredientBlockDropOdds(const bool bForceRefresh)
{
	if ((IngredientBlockDropOdds.Num() > 0 && !bForceRefresh) || GetRecipe().Name.IsNone()) {
		return;
	}
	AMMGameMode* GameMode = Cast<AMMGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode == nullptr) {
		UE_LOG(LogMMGame, Error, TEXT("RecipePlayGrid::InitIngredientGoodsDropOdds - Cannot get game mode"));
		return;
	}
	TArray<FGoodsQuantity> CraftingInputs = GetRecipe().CraftingInputs;
	IngredientBlockDropOdds.Empty();
	UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("RecipePlayGrid::InitIngredientGoodsDropOdds - Init for recipe: %s with %d ingredients"), *GetRecipe().Name.ToString(), CraftingInputs.Num());
	// Determine the odds of each ingredient type that could be picked
	for (int32 i = 0; i < CraftingInputs.Num(); i++)
	{
		bool bFound;
		FBlockType BlockType;
		FGoodsQuantity IngredientGoods = CraftingInputs[i];
		if (!GameMode->GetBlockTypeByName(IngredientGoods.Name, BlockType)) {
			UE_LOG(LogMMGame, Error, TEXT("RecipePlayGrid::InitIngredientGoodsDropOdds - Cannot get block type name for goods %s"), *IngredientGoods.Name.ToString());
			continue;
		}
		TArray<FGoodsQuantity> BlockAwardGoods = GameMode->GetGoodsDropper()->EvaluateGoodsDropSet(BlockType.MatchDropGoods, 0.5f);
		if (!bIngredientsFromInventory || InputGoodsInventory->HasAllGoods(BlockAwardGoods))
		{
			float IngredientBlockAward = UGoodsFunctionLibrary::CountInGoodsQuantityArray(IngredientGoods.Name, BlockAwardGoods, bFound);
			float ProductionRatio = 1.f;
			if (bFound)
			{
				ProductionRatio = IngredientGoods.Quantity / IngredientBlockAward;
			}
			// The IngredientGoods.Name is going to be the BlockType name picked to drop.
			// Just using a FGoodsQuantity as odds entries because it's convenient -- it has a name and float.
			IngredientBlockDropOdds.Add(FGoodsQuantity(IngredientGoods.Name, FMath::Max(0.5f, ProductionRatio)));
			
			// Get the recipe that produces the ingredient goods
			//FCraftingRecipe IngredientRecipe = GetRecipeManager()->GetRecipeForGoodsName(IngredientGoods.Name, bFound);
			//if (bFound)
			//{
			//	TArray<FGoodsQuantity> RecipeGoods;
			//	if (GetRecipeManager()->GetGoodsForRecipe(IngredientRecipe, RecipeGoods, 0.5, true))
			//	{
			//		// Determine the ratio of ingredient goods produced by it's producing recipe to the quantity required by the
			//		// recipe being crafted. This ratio will be the ingredient's weight in our weighted list pick.
			//		// i.e. The more ingredients this recipe needs relative to how many the producing recipe yeilds, 
			//		// the higher the chance the ingredient will be picked.
			//		// TODO: Rework this rule
			//		float ProducingRecipeQuantity = UGoodsFunctionLibrary::CountInGoodsQuantityArray(IngredientGoods.Name, RecipeGoods, bFound);
			//		float ProductionRatio = 1.f;
			//		if (ProducingRecipeQuantity > 0) {
			//			ProductionRatio = IngredientGoods.Quantity / ProducingRecipeQuantity;
			//		}
			//		// The IngredientGoods.Name is going to be the BlockType name picked to drop.
			//		// Just using a FGoodsQuantity as odds entries because it's convenient -- it has a name and float.
			//		IngredientBlockDropOdds.Add(FGoodsQuantity(IngredientGoods.Name, FMath::Max(0.5f, ProductionRatio)));
			//	}
			//}
		}
		else {
			UE_CLOG(bDebugLog, LogMMGame, Warning, TEXT("RecipePlayGrid::InitIngredientGoodsDropOdds - Grid does not have inventory for recipe %s"), *GetRecipe().Name.ToString());
			for (FGoodsQuantity AwardGQ : BlockAwardGoods) {
				UE_CLOG(bDebugLog, LogMMGame, Warning, TEXT("                                              Need %0.f %s.  Have %0.f"), AwardGQ.Quantity, *AwardGQ.Name.ToString(), InputGoodsInventory->GetGoodsCount(AwardGQ.Name));
			}
		}
	}
	if (IngredientBlockDropOdds.Num() == 0)	{
		UE_LOG(LogMMGame, Warning, TEXT("RecipePlayGrid::InitIngredientBlockDropOdds - Found no weighted ingredient odds."));
	}
}