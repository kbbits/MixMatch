#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MMEnums.h"
#include "BlockMatch.h"
#include "MatchAction.h"
#include "MMPlayGridCell.h"
#include "MMPlayGrid.generated.h"

class AMMBlock;

/** Class used to spawn grid and manage score */
UCLASS(minimalapi)
class AMMPlayGrid : public AActor
{
	GENERATED_BODY()

public:

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

	/** If true, blocks will be scaled to fill BlockSize - BlockMargin */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bScaleBlocks;

	/** Size of the block in unscaled, local dimensions */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector BlockSize;

	/** Spacing inside of BlockSize.*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float BlockMargin;

	/** Spacing between the background cells meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float CellBackgroundMargin;

	/** The -Y offset behind the cell to place the background cell meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float CellBackgroundOffset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TAssetPtr<USoundBase> MoveFailSound;

	UPROPERTY()
	TArray<FBlockMatch> BlockMatches;

	UPROPERTY()
	AMMBlock* SelectedBlock;

protected:

	UPROPERTY()
	TArray<AMMPlayGridCell*> Cells;

	UPROPERTY()
	TArray<AMMBlock*> Blocks;

	UPROPERTY(BlueprintReadOnly)
	TArray<AMMBlock*> UnsettledBlocks;

	UPROPERTY(BlueprintReadOnly)
	TArray<AMMBlock*> BlocksToCheck;

	UPROPERTY()
	TArray<USoundBase*> PlaySoundQueue;

private:

	/** Scene root component */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class USceneComponent* SceneRoot;

	/** Text component for the score */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UTextRenderComponent* ScoreText;

public:
	AMMPlayGrid();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable)
	int32 GetMinimumMatchSize();

	UFUNCTION(BlueprintCallable, CallInEditor)
	void SpawnGrid();

	UFUNCTION(BlueprintCallable, CallInEditor)
	void DestroyGrid();

	UFUNCTION(BlueprintCallable, CallInEditor)
	void DestroyBlocks();

	// Base class implementation calls GameMode->GetRandomBlockTypeNameForCell
	UFUNCTION(BlueprintNativeEvent)
	bool GetRandomBlockTypeNameForCell(const AMMPlayGridCell* Cell, FName& FoundBlockTypeName);

	UFUNCTION(BlueprintCallable)
	bool AddBlockInCell(UPARAM(ref) AMMPlayGridCell* Cell, const FName& BlockType, const float OffsetAboveCell = 0.f, const bool bAllowUnsettle = true);

	UFUNCTION(BlueprintCallable)
	void AddRandomBlockInCell(UPARAM(ref) AMMPlayGridCell* Cell, const float OffsetAboveCell = 0.f, const bool bAllowUnsettle = true, const bool bPreventMatches = false);

	UFUNCTION(BlueprintCallable, CallInEditor)
	void FillGridBlocks();

	UFUNCTION(BlueprintPure)
	AMMPlayGridCell* GetCell(const FIntPoint& Coords);

	UFUNCTION(BlueprintPure)
	AMMBlock* GetBlock(const FIntPoint& Coords);

	UFUNCTION(BlueprintPure)
	FVector GridCoordsToWorldLocation(const FIntPoint& GridCoords);

	UFUNCTION(BlueprintPure)
	FVector GridFloatCoordsToWorldLocation(const FVector2D& GridCoords);

	void BlockClicked(AMMBlock* Block);

	/* Move a block from one cell to given adjacent cell.
	 * If target cell is occupied by a moveable block, the blocks will be swapped. 
	 * Returns true if the move was successful. */
	UFUNCTION(BlueprintCallable)
	bool MoveBlock(UPARAM(ref) AMMBlock* MovingBlock, UPARAM(ref) AMMPlayGridCell* ToCell);

	UFUNCTION(BlueprintCallable)
	bool CheckFlaggedForMatches();

	UFUNCTION(BlueprintCallable)
	bool CheckForMatches(UPARAM(ref) AMMBlock* CheckBlock, FBlockMatch& HorizMatches, FBlockMatch& VertMatches, const bool bMarkBlocks = true);

	UFUNCTION(BlueprintCallable)
	int32 MatchHorizontal(UPARAM(ref) AMMBlock* StartBlock, FBlockMatch& Match, const bool bMarkBlocks = true);

	UFUNCTION(BlueprintCallable)
	int32 MatchVertical(UPARAM(ref) AMMBlock* StartBlock, FBlockMatch& Match, const bool bMarkBlocks = true);

	UFUNCTION(BlueprintCallable)
	int32 MatchDirection(UPARAM(ref) AMMBlock* StartBlock, const EMMDirection Direction, FBlockMatch& Matches, const bool bRecurse = true);

	UFUNCTION(BlueprintCallable)
	void SortMatch(UPARAM(ref) FBlockMatch& Match, bool bForceSort = false);

	UFUNCTION()
	bool ResolveMatches();

	UFUNCTION()
	bool PerformActionsForMatch(UPARAM(ref) FBlockMatch& Match);

	UFUNCTION()
	bool PerformActionType(const FMatchActionType& MatchActionType, const FBlockMatch& Match);

	UFUNCTION()
	void SettleBlocks();

	void UnsettleBlock(AMMBlock* Block);

	void UnsettleBlockAboveCell(AMMPlayGridCell* Cell);

	void AddScore(int32 PointsToAdd);
		
	void BlockFinishedMoving(AMMBlock* Block, bool bBlockMoved = true);
	
	void BlockFinishedMatch(AMMBlock* Block, const FBlockMatch& Match);
	
	UFUNCTION(BlueprintCallable)
	void PlaySounds(const TArray<USoundBase*> Sounds);

	/** Returns DummyRoot subobject **/
	FORCEINLINE class USceneComponent* GetSceneRoot() const { return SceneRoot; }
	/** Returns ScoreText subobject **/
	FORCEINLINE class UTextRenderComponent* GetScoreText() const { return ScoreText; }

protected:
	// Begin AActor interface
	virtual void BeginPlay() override;
	// End AActor interface

};



