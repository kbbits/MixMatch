#pragma once


#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CraftingRecipe.h"
#include "CraftingToolActor.generated.h"


/** A crafting tool the player can use.  Generally this will spawn a play grid for the crafting of recipes. */
UCLASS(minimalapi)
class ACraftingToolActor : public AActor
{
	GENERATED_BODY()

/*##########  Properties  ###### */

public:

	/** This tool can craft recipes that have at least one of these categories. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FName> CraftableRecipeCategories;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftClassPtr<class AMMPlayGrid> GridClass;

protected:

	/** A reference to the active recipe manager. Currently this would be a ref to the recipe manager on the player controller. */
	class URecipeManagerComponent* RecipeManager = nullptr;

	/** The current recipe this tool will craft */
	UPROPERTY()
	FCraftingRecipe CurrentRecipe;

	/** The grid this tool is working with.
	 *  TODO: Is this needed?  */
	UPROPERTY()
	class AMMPlayGrid* CurrentGrid;

private:

	/** Root component */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class USceneComponent* SceneRoot;

	/** StaticMesh component for the tool actor */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UStaticMeshComponent* ToolMesh;


/*##########  Functions ###### */

public:

	ACraftingToolActor();

	/** Set the reference to the recipe manager this tool is using. */
	UFUNCTION(BlueprintCallable)
	void SetRecipeManager(UPARAM(ref) class URecipeManagerComponent* NewRecipeManager);

	/** Get the current recipe manager. 
	 *  If not set, this will set the recipe manager to the one on the player controller. */
	UFUNCTION(BlueprintPure)
	class URecipeManagerComponent* GetRecipeManager();

	/** Set the recipe this tool will craft. */
	UFUNCTION(BlueprintCallable)
	void SetRecipe(const FName& RecipeName);

	/** Gets the recipe this tool would currently craft. */
	UFUNCTION(BlueprintPure)
	FCraftingRecipe GetRecipe();

	/** If this returns a valid play grid cell class, this will override the grid class's setting for CellClass 
	 *  Base class returns null so grid's CellClass is used. */
	UFUNCTION(BlueprintNativeEvent)
	TSubclassOf<class AMMPlayGridCell> GetCellClass();

	/** The target number of different block types for the tool. 
	 *   Base class just returns 5.  */
	UFUNCTION(BlueprintNativeEvent)
	int32 GetTargetBlockTypeCount();

	/** Base class returns a fixed size grid (GetTargetBlockTypeCount + 2, GetTargetBlockTypeCount + 2) 
	 *  @returns the size of the grid that should be created. */
	UFUNCTION(BlueprintNativeEvent)
	FIntPoint GetNewGridSize();

	/** 
	 *  If this returns a vector with all components > 0, this will override the grid's BlockSize property. 
	 *  Base class returns (-1, -1, -1) so grid's BlockSize will be used. 
	 *  @Returns the world-scale BlockSize that the grid should use. */
	UFUNCTION(BlueprintNativeEvent)
	FVector GetGridBlockSize();

	/** If this returns a value > 0 it will be used to override the spawned grid's NewBlockDropInHeight.
	 *  Base class returns -1, so grid's normal NewBlockDropInHeight will be used.
	 *  @returns The override value for the grid's NewBlockDropInHeight. */
	UFUNCTION(BlueprintNativeEvent)
	float GetNewBlockDropInHeight();

	/** Gets the default, starting BlockTypeSet name that should be set on grids when this tool spawns them. 
	 *  Default = "Default"  */
	UFUNCTION(BlueprintNativeEvent)
	FName GetDefaultBlockTypeSetName();

	/** Spawns the grid actor. Does not tell the grid to spawn it's cells and related background meshes. */
	UFUNCTION(BlueprintCallable)
	void SpawnGrid();

	/** Destroys the spawned grid of this tool, if any. */
	UFUNCTION(BlueprintCallable)
	void DestroyGrid();

protected:

	/** Base class does nothing (for now) */
	UFUNCTION(BlueprintNativeEvent)
	void HandleClick();

	/** Handle the tool being clicked. Calls HandleClick. */
	UFUNCTION()
	void ToolClicked(UPrimitiveComponent* ClickedComp, FKey ButtonClicked);

	/** Handle the tool being touched. Calls HandleClick  */
	UFUNCTION()
	void OnFingerPressedTool(ETouchIndex::Type FingerIndex, UPrimitiveComponent* TouchedComponent);

};
