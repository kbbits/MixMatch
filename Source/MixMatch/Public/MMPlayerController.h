// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InventoryActorComponent.h"
#include "RecipeManagerComponent.h"
#include "MMPlayGrid.h"
#include "MMPlayerController.generated.h"


// Event dispatcher for when CurrentGrid changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCurrentGridChanged, const AMMPlayGrid*, NewCurrentGrid);

// Event dispatcher for when CurrentGrid changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRecipeCrafted, const FCraftingRecipe&, CraftedRecipe, const int32, QuantityCrafted);


/** PlayerController class used to enable cursor */
UCLASS()
class AMMPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMMPlayerController();

	// Delegate event when current grid has changed.
	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
	FOnCurrentGridChanged OnCurrentGridChanged;

	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
	FOnRecipeCrafted OnRecipeCrafted;

	// Player Goods inventory
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	UInventoryActorComponent* GoodsInventory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	URecipeManagerComponent* RecipeManager;

protected:

	UPROPERTY()
	AMMPlayGrid* CurrentGrid = nullptr;

	// ### FUNCTIONS

public:

	UFUNCTION(BlueprintNativeEvent)
	float GetBlockMoveSpeedMultiplier();

	UFUNCTION(BlueprintCallable)
	void SetCurrentGrid(UPARAM(ref) AMMPlayGrid* NewCurrentGrid);

	UFUNCTION(BlueprintCallable)
	AMMPlayGrid* GetCurrentGrid();

	UFUNCTION(BlueprintCallable)
	bool CraftRecipe(const FCraftingRecipe& Recipe);

	UFUNCTION(BlueprintPure)
	int32 GetRecipeLevel(const FName RecipeName);

	UFUNCTION(BlueprintCallable)
	void SetRecipeLevel(const FName RecipeName, const int32 NewLevel);

};


