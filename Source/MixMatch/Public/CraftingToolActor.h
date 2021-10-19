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

	/** The AMMPlayGrid class or sub-class, that this tool will spawn. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftClassPtr<class AMMPlayGrid> GridClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ViewFOV = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ExtraCameraDistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GridMarginH = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GridMarginV = 0.f;

protected:

	/** A reference to the active recipe manager. Currently this would be a ref to the recipe manager on the player controller. */
	UPROPERTY(BlueprintGetter=GetRecipeManager)
	class URecipeManagerComponent* RecipeManager = nullptr;

	/** The current recipe this tool will craft */
	UPROPERTY(BlueprintGetter=GetRecipe)
	FCraftingRecipe CurrentRecipe;

	/** The grid this tool is working with.
	 *  TODO: Is this needed?  */
	UPROPERTY(BlueprintReadWrite)
	class AMMPlayGrid* CurrentGrid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDebugLog = true;

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

	virtual void CalcCamera(float DeltaTime, struct FMinimalViewInfo& OutResult) override;

	/** Returns the location in world space that this till will spawn the grid. 
	 *  Base class returns actor location. */
	UFUNCTION(BlueprintNativeEvent)
	FVector GetGridSpawnLocation();

	/** Set the reference to the recipe manager this tool is using. */
	UFUNCTION(BlueprintCallable)
	void SetRecipeManager(UPARAM(ref) class URecipeManagerComponent* NewRecipeManager);

	/** Get the current recipe manager. 
	 *  If not set, this will set the recipe manager to the one on the player controller and return it. */
	UFUNCTION(BlueprintPure)
	class URecipeManagerComponent* GetRecipeManager();

	UFUNCTION(BlueprintPure)
	TArray<FCraftingRecipe> GetCraftableRecipes();

	/** Set the recipe this tool will craft. */
	UFUNCTION(BlueprintCallable)
	void SetRecipe(const FName& RecipeName);

	/** Gets the recipe this tool would currently craft. */
	UFUNCTION(BlueprintPure)
	FCraftingRecipe GetRecipe();

	/** If this returns > 2, this will override the grid's minimum match size. 
	 *  Base class returns -1, so grid's default will be used. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	int32 GetMinimumMatchSize();

	/** If this returns a valid play grid cell class, this will override the grid class's setting for CellClass 
	 *  Base class returns null so grid's CellClass is used. */
	UFUNCTION(BlueprintNativeEvent)
	TSubclassOf<class AMMPlayGridCell> GetCellClass();

	/** If this returns a value > 0, then this will set the grid's maximum player moves that should be allowed for the next play-through of the grid. 
	 *  Base class just returns 20. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	int32 GetMaxPlayerMoves();

	/** The target number of different block types for the tool. 
	 *   Base class just returns 5.  */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	int32 GetTargetBlockTypeCount();

	/** Base class returns a fixed size grid (GetTargetBlockTypeCount + 2, GetTargetBlockTypeCount + 2) 
	 *  @returns the size of the grid that should be created. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
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
	UFUNCTION(BlueprintCallable, CallInEditor)
	void SpawnGrid();

	UFUNCTION(BlueprintCallable, CallInEditor)
	void SpawnGridBackground();
		
	/** Destroys the spawned grid of this tool, if any. */
	UFUNCTION(BlueprintCallable, CallInEditor)
	void DestroyGrid();

	//### Camera Stuff 

	/** Gets a normalized vector representing the facing of the grid in world space. i.e. the grid's play surface "normal". 
     *  Intended for placing the camera facing along the opposite of this vector. 
	 *  Base class return's this actor's forward vector. */
	UFUNCTION(BlueprintNativeEvent)
	FVector GetGridFacing();

	/** Gets the bounds of the grid's play area in world space.
	 *  Intended for placing the camera (considering it's FOV) so these bounds are visible. */
	UFUNCTION(BlueprintNativeEvent)
	FBoxSphereBounds GetVisibleGridBounds();

protected:

	/** Base class does nothing (for now) */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void HandleClick();

	/** Handle the tool being clicked. Calls HandleClick. */
	UFUNCTION()
	void ToolClicked(UPrimitiveComponent* ClickedComp, FKey ButtonClicked);

	/** Handle the tool being touched. Calls HandleClick  */
	UFUNCTION()
	void OnFingerPressedTool(ETouchIndex::Type FingerIndex, UPrimitiveComponent* TouchedComponent);

};
