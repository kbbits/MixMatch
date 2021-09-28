
#include "RecipePlayGrid.h"
#include "..\MixMatch.h"
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
	return RecipeManager;
}


void ARecipePlayGrid::SetRecipe(FCraftingRecipe& Recipe)
{
	CurrentRecipe = Recipe;
}


bool ARecipePlayGrid::SetRecipeByName(const FName& RecipeName)
{
	if (!IsValid(RecipeManager)) {
		UE_LOG(LogMMGame, Error, TEXT("RecipePlayGrid::SetRecipeByName - RecipeManager is null"));
		return false;
	}
	bool bFound;
	CurrentRecipe = RecipeManager->GetRecipe(RecipeName, bFound);
	return bFound;
};


const FCraftingRecipe& ARecipePlayGrid::GetRecipe()
{
	return CurrentRecipe;
}


bool ARecipePlayGrid::GetRandomBlockTypeNameForCell_Implementation(const AMMPlayGridCell* Cell, FName& FoundBlockTypeName)
{
	return Super::GetRandomBlockTypeNameForCell_Implementation(Cell, FoundBlockTypeName);
}