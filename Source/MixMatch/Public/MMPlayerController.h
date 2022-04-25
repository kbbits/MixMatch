// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PlayerSaveData.h"
#include "MMEnums.h"
#include "PersistentDataComponent.h"
#include "InventoryActorComponent.h"
#include "RecipeManagerComponent.h"
#include "MMPlayGrid.h"
#include "MMPlayerController.generated.h"

class ACraftingToolActor;
class UActionBarItem;

// Event dispatcher for when CurrentGrid changes to different grid
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCurrentGridChanged, const AMMPlayGrid*, NewCurrentGrid);

// Event dispatcher for when player starts play on a grid
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayGridStarted, const AMMPlayGrid*, PlayingGrid);

// Event dispatcher for when player stops play on a grid
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayGridStopped, const AMMPlayGrid*, PlayingGrid);

// Event dispatcher for when goods are collected (from matches on the grid).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGoodsCollected, const TArray<FGoodsQuantity>&, CollectedGoods);

// Event dispatcher for when a recipe is crafted
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRecipeCrafted, const FCraftingRecipe&, CraftedRecipe, const int32, QuantityCrafted);

// Event dispatcher for when slots are added to or removed from action bar
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnActionBarChanged);

// Event dispatcher for when a given action bar slot has changed
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActionBarItemChanged, const int32, SlotNumber, const UActionBarItem*, CurrentActionBarItem);


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

	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
	FOnPlayGridStarted OnPlayGridStopped;

	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
	FOnGoodsCollected OnGoodsCollected;

	// Delegate event when a recipe is crafted
	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
	FOnRecipeCrafted OnRecipeCrafted;

	// When slots are added to or removed from the action bar.
	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
	FOnActionBarChanged OnActionBarChanged;

	// When given action bar slot has changed
	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
	FOnActionBarItemChanged OnActionBarItemChanged;

	// This component handles save/load 
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite)
	UPersistentDataComponent* PersistentDataComponent;

	// Player Goods inventory
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	UInventoryActorComponent* GoodsInventory;

	// Recipe manager component
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	URecipeManagerComponent* RecipeManager;

	// Total goods this player profile has collected.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	TArray<FGoodsQuantity> TotalGoodsCollected;

	// Unique Id of the current player profile (i.e. save game)
	UPROPERTY(BlueprintReadOnly, SaveGame)
	FGuid PlayerGuid;

	// Display name for this player profile (i.e. save game)
	UPROPERTY(BlueprintReadWrite, SaveGame)
	FText PlayerDisplayName;

protected:

	UPROPERTY()
	AMMPlayGrid* CurrentGrid = nullptr;

	UPROPERTY(BlueprintReadWrite)
	ACraftingToolActor* CurrentTool = nullptr;

	UPROPERTY(SaveGame)
	int32 ActionBarSize = 0;

	UPROPERTY()
	TArray<UActionBarItem*> ActionBarItems;

	// Ordered list (stack) of input contexts.
	UPROPERTY(BlueprintReadOnly)
	TArray<EMMInputContext> InputContextStack;

	float LoadedTotalPlaytime;
	float TimeAtLastSave;

	// ### FUNCTIONS

public:

	// Gets player save data from this controller and its components.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool GetPlayerSaveData(FPlayerSaveData& SaveData);

	// Updates the controller and its components from serialized data.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool UpdateFromPlayerSaveData(const FPlayerSaveData& SaveData);

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
	void CollectGoods(const TArray<FGoodsQuantity> CollectedGoods);

	UFUNCTION(BlueprintCallable)
	bool CraftRecipe(const FCraftingRecipe& Recipe);

	UFUNCTION(BlueprintPure)
	int32 GetRecipeLevel(const FName RecipeName);

	UFUNCTION(BlueprintCallable)
	void SetRecipeLevel(const FName RecipeName, const int32 NewLevel);

	/** Returns the number of available slots in action bar.
	 *  Note that slot numbers (indexes) in action bar are 0 based. So max valid slot number = action bar size - 1. */
	UFUNCTION(BlueprintCallable)
	int32 GetActionBarSize();

	/** Set the size of the action bar. This will shrink or grow the ActionBarItems array as needed. */
	UFUNCTION(BlueprintCallable)
	void SetActionBarSize(const int32 NewSize);

	/** Gets the UActionBarItem instance at the given action bar slot index. (Index is 0 based) 
	 *  Returns nullptr if slot does not exist or no item is in slot. */
	UFUNCTION(BlueprintPure)
	UActionBarItem* GetActionBarItem(const int32 SlotIndex);

	/** Sets the action bar slot to use the given usable goods name. (slot index is 0 based)
	 *  Returns true if setting the slot was successful. 
	 *  Returns false if slot number > ActionBarSize. */
	UFUNCTION(BlueprintCallable)
	bool SetActionBarItemByName(const FName& UsableGoodsName, const int32 SlotIndex);

	/** Removes and cleans up any action bar item at the given slot index. 
	 *  If bFireEvents = true then the OnActionBarItemChanged event will be dispatched. */
	UFUNCTION(BlueprintCallable)
	void ClearActionBarItem(const int32 SlotIndex, const bool bFireEvents = true);

	UFUNCTION(BlueprintCallable)
	void HandleToolClicked(UPARAM(ref) ACraftingToolActor* ClickedTool);

	/** Override in BP to set up grid, inventory, etc. for a grid before grid play actually starts. 
	 *  Base class just calls OnPlayGrid. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void OnPrePlayGrid();

	/** Base class calls StartPlayGrid on current grid and fires OnPlayGridStarted event. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void OnPlayGrid();

	/** Call when play on grid has stopped.  Base class calls StopPlayGrid on grid and fires OnPlayGridStopped event. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void OnStopGrid();
		
	UFUNCTION(BlueprintCallable)
	void AddInputContext(const EMMInputContext NewContext);

	UFUNCTION(BlueprintCallable)
	void RemoveInputContext(const EMMInputContext RemoveContext, const bool bRemoveAll = false);

	UFUNCTION(BlueprintCallable)
	void ClearInputContextStack();

	UFUNCTION(BlueprintPure)
	EMMInputContext GetLastInputContext();

	UFUNCTION(BlueprintPure)
	bool HasInputContext(const EMMInputContext CheckContext);

};


