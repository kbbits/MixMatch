#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MMEnums.h"
#include "MMBlock.h"
#include "BlockMatch.h"
#include "MatchAction.h"
#include "MMPlayGridCell.h"
#include "MMPlayGrid.generated.h"

// Event dispatcher for when grid gives award for matches
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchAwards, const TArray<UBlockMatch*>&, Matches);

// Event dispatcher for when grid gives award for blocks destroyed outside of matches
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBlockDestroyedAwards, const AMMBlock*, Block, const TArray<FGoodsQuantity>&, Goods);

// Event dispatcher for when grid gives award for matches
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGridLocked, const AMMPlayGrid*, LockedGrid);

// Event dispatcher for when player makes a move on the grid
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerMoved, const AMMPlayGrid*, Grid);

// Event dispatcher for when grid reaches max player moves
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMaxPlayerMoves, const AMMPlayGrid*, Grid);

/** A match grid containing cells and blocks. */
UCLASS(minimalapi)
class AMMPlayGrid : public AActor
{
	GENERATED_BODY()

public:

	// Delegate event when grid gives awards for matches.
	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
	FOnMatchAwards OnMatchAwards;

	// Delegate event when grid gives awards for blocks destroyed outside of matches.
	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
	FOnBlockDestroyedAwards OnBlockDestroyedAwards;

	// Delegate event called when grid becomes locked.
	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
	FOnGridLocked OnGridLocked;

	// Delegate event called when player moves a block on the grid.
	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
	FOnPlayerMoved OnPlayerMoved;

	// Delegate event called when max player moves is reached.
	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
	FOnMaxPlayerMoves OnMaxPlayerMoves;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	EMMGridState GridState;

	/** Current Score */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Score;

	/** Number of blocks along X dimension of grid */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 SizeX;

	/** Number of blocks along Y dimension of grid */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 SizeY;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<AMMPlayGridCell> CellClass;

	/** The block type set this grid is currently using. Determine which block will be generated. */
	UPROPERTY(EditAnywhere)
	FName BlockTypeSetName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float GridMarginTop = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float GridMarginBottom = 100.0f;

	/** If true, blocks will be scaled to fill BlockSize - BlockMargin in it's cell. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bScaleBlocks;

	/** Size of the block in local dimensions */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector BlockSize;

	/** Spacing inside of BlockSize.*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float BlockMargin;

	/** Spacing between the cells. i.e. also the spacing between the background cell meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float CellBackgroundMargin;

	/** The -Y offset behind the block to place the background cell meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float CellBackgroundOffset;

	/** New blocks spawn BlockSize.Z + NewBlockDropHeight above the grid */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float NewBlockDropInHeight;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TAssetPtr<USoundBase> MoveFailSound;

	/** List of matches being processed */
	UPROPERTY()
	TArray<UBlockMatch*> BlockMatches;

	/** Currently selected block */
	UPROPERTY()
	AMMBlock* SelectedBlock;

	/** Number of moves player has made during current grid game */
	UPROPERTY(BlueprintReadOnly)
	int32 PlayerMovesCount;

	/** If > 0, this is the maximum number of player moves allwed during grid game. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MaxPlayerMovesCount = 0;

	/** Stop the new blocks from falling into the grid when other blocks are matched. 
	 *  Currently used for dev/debug only. */
	UPROPERTY(BlueprintReadWrite)
	bool bPauseNewBlocks;

	/** List of blocks, by column, that are currently marked as bFallingIntoGrid. i.e. the blocks "above" the grid. */
	UPROPERTY(BlueprintReadOnly)
	TMap<int32, FBlockSet> BlocksFallingIntoGrid;

protected:

	/** Minimum match size. Default = 3 */
	UPROPERTY(EditAnywhere)
	int32 MinimumMatchSize = 3;

	/** The percent chance that the grid will add duplicate block type prevention logic for each block dropped in. Default = 0.5*/
	UPROPERTY(EditAnywhere)
	float DuplicateSpawnPreventionFactor = 0.5;

	/** All of this grid's cells */
	UPROPERTY()
	TArray<AMMPlayGridCell*> Cells;

	/** All of the blocks this grid has spawned. (that still exist) */
	UPROPERTY()
	TArray<AMMBlock*> Blocks;

	/** List of blocks that have been unsettled but not yet settled. */
	UPROPERTY(BlueprintReadOnly)
	TArray<AMMBlock*> UnsettledBlocks;

	/** Queue of blocks to be unsettled during the next settle tick. */
	UPROPERTY(BlueprintReadOnly)
	TArray<AMMBlock*> ToBeUnsettledBlocks;

	/** List of blocks to check for adjacent matches. */
	UPROPERTY(BlueprintReadOnly)
	TArray<AMMBlock*> BlocksToCheck;

	/** Blocks to be destroyed during AllMatchesFinished() */
	UPROPERTY()
	TArray<AMMBlock*> BlocksToDestroy;

	/** Simple queue of sounds to play. Sounds are played and queue is emptied on each tick. */
	UPROPERTY()
	TArray<USoundBase*> PlaySoundQueue;

	UPROPERTY()
	TArray<class AGameEffectPreviewActor*> EffectPreviewActors;

	/** Have all current matches finished? */
	bool bAllMatchesFinished = false;

	/** Has the grid been checked for locked state? And if so, is the grid locked? i.e. no valid moves available. (excluding special powers/actions) */
	EMMGridLockState GridLockedState = EMMGridLockState::Unchecked;

	/** The last coords that were checked for locked grid status. 
	 *  This allows us to check for locked state over the span of many ticks. */
	FIntPoint GridLockCheckCoords = FIntPoint::ZeroValue;

	/** How many potential move matches should be checked each tick.  */
	int32 GridLockChecksPerTick = 5;

	/** Inventory for this grid */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	class UInventoryActorComponent* GoodsInventory;

	/** Output verbose debug logging for this grid. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDebugLog = true;

	/** Include match logic in debug logging. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDebugLogMatches = false;

private:

	/** Scene root component */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class USceneComponent* SceneRoot;

	/** Text component for the score */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UTextRenderComponent* ScoreText;

	/** StaticMesh component for the clickable blocks toggle control */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* BlockToggleControlMesh;

#if WITH_EDITORONLY_DATA
	/** Actor billboard */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UBillboardComponent* Billboard;
#endif


//###########  Functions ###### 

public:
	AMMPlayGrid();

	virtual void Tick(float DeltaSeconds) override;

	void SettleTick(float DeltaSeconds);

	/** Called when player begins play on this grid.
	 *  If overridden in BP, parent should be called. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void StartPlayGrid();

	/** This gives all the grid's goods inventory to the player. */
	UFUNCTION(BlueprintNativeEvent)
	void StopPlayGrid();

	/** Minimum number of matching blocks to qualify as a match */
	UFUNCTION(BlueprintPure)
	int32 GetMinimumMatchSize();

	UFUNCTION(BlueprintCallable)
	void SetMinimumMatchSize(const int32 NewMinimumMatchSize);

	/** Set the name of the BlockTypeSet that is used by the grid to determine which blocks will be generated. */
	UFUNCTION(BlueprintCallable)
	bool SetBlockTypeSetName(const FName& NewBlockTypeSetName);

	/** Get the name of the curent BlockTypeSet the grid is using. */
	UFUNCTION(BlueprintPure)
	FName GetBlockTypeSetName();

	/** Add a number of moves to the max allowed for the current grid. */
	UFUNCTION(BlueprintCallable)
	void AddPlayerMaxMoveCount(const int32 MovesToAdd);

	/** Increments the number of move turns the player has made.
	 *  Returns true if more move turns remain. */
	UFUNCTION(BlueprintCallable)
	bool IncrementPlayerMoveTurn();

	//### Spawn Destroy **/

	/** Spawns the grid's cells (and cell background meshes) */
	UFUNCTION(BlueprintCallable, CallInEditor)
	void SpawnGrid();

	/** Fill the grid with blocks. */
	UFUNCTION(BlueprintCallable, CallInEditor)
	void FillGridBlocks();

	/** Destroy all blocks and cells in the grid. Does not destroy self. */
	UFUNCTION(BlueprintCallable, CallInEditor)
	void DestroyGrid();

	/** Destroys all blocks in the grid. */
	UFUNCTION(BlueprintCallable, CallInEditor)
	void DestroyBlocks();

	//### Add Blocks **/

	// Base class implementation calls GameMode->GetRandomBlockTypeNameForCell
	UFUNCTION(BlueprintNativeEvent)
	bool GetRandomBlockTypeNameForCell(FName& FoundBlockTypeName, const FAddBlockContext& BlockContext);

	/** Add a new block of the given block type into the given cell. */
	UFUNCTION(BlueprintCallable)
	AMMBlock* AddBlockInCell(const FName& BlockType, const FAddBlockContext& BlockContext);

	/** Add a new block of a random block type to the given cell. */
	UFUNCTION(BlueprintCallable)
	virtual AMMBlock* AddRandomBlockInCell(const FAddBlockContext& BlockContext);
		
	/** Drop a random block from above grid, to fall into given cell.*/
	UFUNCTION(BlueprintCallable)
	AMMBlock* DropRandomBlockInColumn(UPARAM(ref) AMMPlayGridCell* Cell);

	/** Called when the given cell has become unoccupied. */
	void CellBecameOpen(AMMPlayGridCell* Cell);

	//### Get Cells & Blocks **/

	// Get the total number of cells in the grid.
	UFUNCTION(BlueprintPure)
	int32 GetCellCount();

	/** Get cell at given grid coordinates. */
	UFUNCTION(BlueprintPure)
	AMMPlayGridCell* GetCell(const FIntPoint& Coords);

	/** Get the cell at the given index. Cells are numbered from bottom left to top right 0 to numCells - 1 */
	UFUNCTION(BlueprintPure)
	AMMPlayGridCell* GetCellByNumber(const int32 CellNumber);

	/** Get cell that are adjacent to the given cell. */
	UFUNCTION(BlueprintPure)
	TArray<AMMPlayGridCell*> GetCellNeighbors(const AMMPlayGridCell* Cell, const bool bIncludeDiagonal = false);

	/** Get the cell at the top of the column. This is the top cell in the grid. i.e. it does not include blocks that are falling into 
	 * the grid but are not yet in any grid cell. */
	UFUNCTION(BlueprintPure)
	AMMPlayGridCell* GetTopCell(const int32 Column);

	/** Get the block at the given grid coordinates. */
	UFUNCTION(BlueprintPure)
	AMMBlock* GetBlock(const FIntPoint& Coords);

	//## Grid & Cell Locations **/

	/** Translates grid coordinates to world coordinates */
	UFUNCTION(BlueprintPure)
	FVector GridCoordsToWorldLocation(const FIntPoint& GridCoords);

	/** Returns coordinates of grid cell in local space (relative to the grid ). Note: grid coord Y axis is "up" the grid, i.e. local Z axis */
	UFUNCTION(BlueprintPure)
	FVector GridCoordsToLocalLocation(const FIntPoint& GridCoords);

	/** Translates grid coordinates to world coordinates. Float coordinates allow specifying a location between grid cells. */
	UFUNCTION(BlueprintPure)
	FVector GridFloatCoordsToWorldLocation(const FVector2D& GridCoords);

	/** Gets the world-space bounds of the portion of the grid intended to be visible to the player. */
	UFUNCTION(BlueprintPure)
	FBoxSphereBounds GetVisibleGridBounds();

	//### Actions 

	void BlockClicked(AMMBlock* Block);

	void CellClicked(AMMPlayGridCell* Cell);

	/** Apply selection preview FX for our grid effects. */
	UFUNCTION()
	void PreviewUsableGoodsSelection(UUsableGoodsContext* UsableGoodsContext);

	UFUNCTION(BlueprintNativeEvent)
	void ShowEffectPreviewForCoords(const UGameEffect* GameEffect, const FIntPoint& Coords);

	UFUNCTION(BlueprintCallable)
	void ClearUsableGoodsPreviews();

	UFUNCTION()
	void ToggleBlocksClicked(UPrimitiveComponent* ClickedComp, FKey ButtonClicked);

	UFUNCTION()
	void OnFingerPressedToggleBlocks(ETouchIndex::Type FingerIndex, UPrimitiveComponent* TouchedComponent);

	/* Move a block from one cell to given adjacent cell.
	 * If target cell is occupied by a moveable block, the blocks will be swapped. 
	 * Returns true if a match was found and the move was successful. */
	UFUNCTION(BlueprintCallable)
	bool MoveBlock(UPARAM(ref) AMMBlock* MovingBlock, UPARAM(ref) AMMPlayGridCell* ToCell);

	/** Checks a single block for a match if the block were moved in the given direction (swapping with neighboring block, if any.).
	 *  Note: North is increasing Y axis (up the gri), South is decreasing Y axis (down the grid). West is decreasing on X axis, East increasing X axis. */
	bool BlockMoveHasMatch(const AMMBlock* CheckBlock, const EMMDirection DirectionToCheck);

	/** Checks a single block to see if it has any moves that would result in a match.
	 * @returns True if a matching move is found, false if not.
	 * If true, also returns the first adjacent destination cell to move the block to that results in a match. */
	bool BlockHasMatchingMove(const AMMBlock* CheckBlock, AMMPlayGridCell** MatchMoveDestination = nullptr);

	//### Matching

	UFUNCTION(BlueprintCallable)
	bool CheckFlaggedForMatches();

	//UFUNCTION(BlueprintCallable)
	//bool CheckForMatches(UPARAM(ref) AMMBlock* CheckBlock, UPARAM(ref) UBlockMatch** HorizMatchPtr, UBlockMatch** VertMatchPtr, const bool bMarkBlocks = true);
	bool CheckForMatches(AMMBlock* CheckBlock, UBlockMatch** HorizMatchPtr, UBlockMatch** VertMatchPtr, const bool bMarkBlocks = true);

	//UFUNCTION(BlueprintCallable)
	//int32 MatchHorizontal(UPARAM(ref) AMMBlock* StartBlock, UPARAM(ref) UBlockMatch** MatchPtr, const bool bMarkBlocks = true);
	int32 MatchHorizontal(AMMBlock* StartBlock, UBlockMatch** MatchPtr, const bool bMarkBlocks = true);

	//UFUNCTION(BlueprintCallable)
	//int32 MatchVertical(UPARAM(ref) AMMBlock* StartBlock, UPARAM(ref) UBlockMatch** MatchPtr, const bool bMarkBlocks = true);
	int32 MatchVertical(AMMBlock* StartBlock, UBlockMatch** MatchPtr, const bool bMarkBlocks = true);

	//UFUNCTION(BlueprintCallable)
	//int32 MatchDirection(UPARAM(ref) AMMBlock* StartBlock, const EMMDirection Direction, UBlockMatch** MatchPtr, const bool bRecurse = true);
	int32 MatchDirection(AMMBlock* StartBlock, const EMMDirection Direction, UBlockMatch** MatchPtr, const bool bRecurse = true);

	UFUNCTION(BlueprintCallable)
	void SortMatches(/*const bool bForceSort = false*/);

	UFUNCTION()
	bool ResolveMatches();

	UFUNCTION()
	bool PerformActionsForMatch(UPARAM(ref) UBlockMatch* Match, const bool bDestroyOnly /*only perform destroy actions OR non-destroy actions*/);

	UFUNCTION()
	bool PerformActionType(const FMatchActionType& MatchActionType, const UBlockMatch* Match, const AMMBlock* TriggeringBlock, const bool bDestroyOnly);

	//### Check for Locked Grid

	/** Check part of the grid to see if grid is locked.
	 * i.e. check for potential moves that would result in a match. */
	UFUNCTION()
	EMMGridLockState CheckGridIsLocked();


	//### Settling

	UFUNCTION()
	void SettleBlocks();

	void UnsettleBlock(AMMBlock* Block);

	void UnsettleBlockAboveCell(AMMPlayGridCell* Cell);

	void AddScore(int32 PointsToAdd);
		
	void BlockFinishedMoving(AMMBlock* Block, bool bBlockMoved = true);
	
	void BlockFinishedMatch(AMMBlock* Block, UBlockMatch* Match);

	void AllMatchesFinished();

	/** Handles destruction of a block via damage. */
	void BlockDestroyedByDamage(AMMBlock* Block);

	/** Handles destruction of a block due to a game effect. */
	void BlockDestroyedByGameEffect(AMMBlock* Block);

	/** Handles destruction of a block via a match action. */
	void BlockDestroyedByMatchAction(AMMBlock* Block);

	//### Misc.

	/** Play all the sounds in the sound queue */
	UFUNCTION(BlueprintCallable)
	void PlaySounds(const TArray<USoundBase*> Sounds);

	/** Returns Root scene subobject **/
	FORCEINLINE class USceneComponent* GetSceneRoot() const { return SceneRoot; }
	/** Returns ScoreText subobject **/
	FORCEINLINE class UTextRenderComponent* GetScoreText() const { return ScoreText; }

	bool DebugBlocks(const FString& ContextString = FString());

protected:
	// Begin AActor interface
	virtual void BeginPlay() override;
	// End AActor interface

	/** Sets up the BlocksFallingIntoGrid map based on grid size. */
	void InitBlocksFallingIntoGrid();

};



