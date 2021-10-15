// Copyright Epic Games, Inc. All Rights Reserved.

#include "MMPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "../MixMatch.h"


AMMPlayerController::AMMPlayerController()
	: Super()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableTouchEvents = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;

	GoodsInventory = CreateDefaultSubobject<UInventoryActorComponent>(TEXT("GoodsInventory"));
	if (GoodsInventory) {
		AddOwnedComponent(GoodsInventory);
	}

	RecipeManager = CreateDefaultSubobject<URecipeManagerComponent>(TEXT("RecipeManager"));
	if (RecipeManager) {
		AddOwnedComponent(RecipeManager);
	}
}


float AMMPlayerController::GetBlockMoveSpeedMultiplier_Implementation()
{
	return 1.f;
}


void AMMPlayerController::SetCurrentGrid(AMMPlayGrid* NewCurrentGrid)
{
	if (IsValid(NewCurrentGrid)) 
	{
		CurrentGrid = NewCurrentGrid;
		OnCurrentGridChanged.Broadcast(CurrentGrid);
	}
}


void AMMPlayerController::ClearCurrentGrid()
{
	CurrentGrid = nullptr;
	OnCurrentGridChanged.Broadcast(nullptr);
}


AMMPlayGrid* AMMPlayerController::GetCurrentGrid()
{
	return CurrentGrid;
}


AMMPlayGrid* AMMPlayerController::GetCurrentGridValid(bool& bIsValid)
{
	if (IsValid(GetCurrentGrid())) {
		bIsValid = true;
	} 
	else {
		bIsValid = false;
	}
	return GetCurrentGrid();
}


bool AMMPlayerController::CraftRecipe(const FCraftingRecipe& Recipe)
{
	if (GoodsInventory->AddSubtractGoodsArray(Recipe.CraftingInputs, true, true))
	{
		TArray<FGoodsQuantity> CraftedGoods;
		if (RecipeManager->GetGoodsForRecipe(Recipe, CraftedGoods))
		{
			GoodsInventory->AddSubtractGoodsArray(CraftedGoods, false, true);
			RecipeManager->IncrementRecipeCraftingCount(Recipe.Name);
			OnRecipeCrafted.Broadcast(Recipe, 1);
			return true;
		}
	}
	return false;
}


int32 AMMPlayerController::GetRecipeLevel(const FName RecipeName)
{
	return RecipeManager->GetRecipeLevel(RecipeName);
}


void AMMPlayerController::SetRecipeLevel(const FName RecipeName, const int32 NewLevel)
{
	RecipeManager->SetRecipeLevel(RecipeName, NewLevel);
}


void AMMPlayerController::OnPlayGrid_Implementation()
{
	GetCurrentGrid()->StartPlayGrid();
	OnPlayGridStarted.Broadcast(GetCurrentGrid());
}