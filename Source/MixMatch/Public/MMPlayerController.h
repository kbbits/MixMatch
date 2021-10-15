// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InventoryActorComponent.h"
#include "RecipeManagerComponent.h"
#include "MMPlayGrid.h"
#include "MMPlayerController.generated.h"


// Event dispatcher for when CurrentGrid changes to different grid
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCurrentGridChanged, const AMMPlayGrid*, NewCurrentGrid);

// Event dispatcher for when player starts play on a grid
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayGridStarted, const AMMPlayGrid*, PlayingGrid);

// Event dispatcher for when a recipe is crafted
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRecipeCrafted, const FCraftingRecipe&, CraftedRecipe, const int32, QuantityCrafted);


/** PlayerController class used to enable cursor */
UCLASS()
class AMMPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMMPlayerController();

	// Delegate event when current grid has changed to differnt grid
	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
	FOnCurrentGridChanged OnCurrentGridChanged;

	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
	FOnPlayGridStarted OnPlayGridStarted;

	// Delegate event when a recipe is crafted
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
	void ClearCurrentGrid();

	UFUNCTION(BlueprintCallable)
	AMMPlayGrid* GetCurrentGrid();

	UFUNCTION(BlueprintCallable, meta = (ExpandBoolAsExecs = bIsValid))
	AMMPlayGrid* GetCurrentGridValid(bool& bIsValid);

	UFUNCTION(BlueprintCallable)
	bool CraftRecipe(const FCraftingRecipe& Recipe);

	UFUNCTION(BlueprintPure)
	int32 GetRecipeLevel(const FName RecipeName);

	UFUNCTION(BlueprintCallable)
	void SetRecipeLevel(const FName RecipeName, const int32 NewLevel);

	/** Override in BP to set up grid, inventory, etc. for a grid before grid play actually starts. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnPrePlayGrid();

	/** Base class calls StartPlayGrid on current grid and fires OnPlayGridStarted event. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void OnPlayGrid();

};


