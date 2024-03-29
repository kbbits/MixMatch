// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MMEnums.h"
#include "BlockMatch.h"
#include "BlockType.h"
#include "MMPlayGridCell.h"
#include "MMBlock.generated.h"

class UGoodsDropper;

/** A block that can be clicked */
UCLASS(minimalapi)
class AMMBlock : public AActor
{
	GENERATED_BODY()

	/** Root component */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Block, meta = (AllowPrivateAccess = "true"))
	class USceneComponent* SceneRoot;

	/** StaticMesh component for the clickable block */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Block, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* BlockMesh;

public:
	AMMBlock();

	virtual void OnConstruction(const FTransform& Transform) override;
	
	/** Current state of this block */
	UPROPERTY(BlueprintReadOnly, Category = Block)
	EMMGridState BlockState;

	/** Are we currently highlighted? */
	UPROPERTY(BlueprintReadOnly, Category = Block)
	bool bIsHighlighted = false;

	/** Was this block moved by the player in the last player move action? 
	 *  This is reset to false when the block's movement finishes. */
	UPROPERTY(BlueprintReadOnly, Category = Block)
	bool bMovedByPlayer = false;

	/** Is this block in a horizontal match? */
	UPROPERTY(BlueprintReadOnly, Category = Block)
	bool bMatchedHorizontal = false;;

	/** Is this block in a vertical match? */
	UPROPERTY(BlueprintReadOnly, Category = Block)
	bool bMatchedVertical = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Block)
	TSoftObjectPtr<USoundBase> MatchSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Block)
	TSoftObjectPtr<USoundBase> StopMoveSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Block)
	TSoftObjectPtr<USoundBase> DestroySound;

	/** Optional curve modifying block's movement speed along it's travels. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Block)
	UCurveFloat* MoveCurve;

	/** Optional curve modifying block's movement speed along it's travels during a failed move. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Block)
	UCurveFloat* MoveFailCurve;

	/** Brief delay when a block becomes unsettled until it's settling movement starts */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Block)
	float SettleFallDelay;

	/** Normal material for the block's mesh */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Block)
	class UMaterialInterface* BaseMaterial;

	/** Material to apply when the mouse is hovering over the block */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Block)
	class UMaterialInterface* AltMaterial;
		
	/** Grid cell where block is located */
	UPROPERTY(BlueprintReadOnly, Category = Block)
	AMMPlayGridCell* OwningGridCell;

	/** Grid cell where block should be located */
	UPROPERTY(BlueprintReadOnly, Category = Block)
	AMMPlayGridCell* SettleToGridCell;

	/** Has this block been unsettled. This remains true while the block is settling. */
	bool bUnsettled = false;

	/** Is this block currently falling from above the grid into the grid play space? */
	bool bFallingIntoGrid;

protected:

	/* BlockType containing most of the properties describing this block.
	 * Comes from data object. (DataTable) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Block)
	FBlockType BlockType;

	UPROPERTY(BlueprintReadOnly, Category = Block)
	int32 CurrentHealth;
	
	bool bMoveSuccessful;

	/** The calculated distance at the start of the block's movement.
	 * This is typically the distance between two cells since blocks settle cell to adjacent cell. */
	float StartMoveDistance;

	/** Our timer handle for the delay between block unsettled to block begins settling. */
	FTimerHandle SettleFallDelayHandle;

	UPROPERTY(BlueprintReadWrite)
	class UMaterialInstanceDynamic* BaseMatDynamic = nullptr;
	UPROPERTY(BlueprintReadWrite)
	class UMaterialInstanceDynamic* AltMatDynamic = nullptr;

	/** Some basic protection for errors when attempting to settle this block */
	int32 BlockSettleFails = 0;

	/** Output verbose logging for blocks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Block)
	bool bDebugLog = true;

private:

	/** The list of matches this block currently belongs two. 
	 * There will be 0, 1 or 2 itens in this list since a block can belong to only one horizontal and one vertical match.*/
	UPROPERTY(BlueprintReadOnly, Category = Block, meta = (AllowPrivateAccess = "true"))
	TArray<UBlockMatch*> CurrentMatches;


	//#######  FUNCTIONS  #######
public:

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintNativeEvent)
	bool MoveTick(float DeltaSeconds);

	/** Base class does nothing but call MatchFinished on all current matches that this block is in. 
	 *  Subclasses should override this with meaningful implementation. (see BlockBase blueprint)*/
	UFUNCTION(BlueprintNativeEvent)
	bool MatchTick(float DeltaSeconds);

	/** Called by the grid during each of it's settle ticks. */
	UFUNCTION(BlueprintNativeEvent)
	bool SettleTick(float DeltaSeconds);

	/** Should only be called once. Currently changing an existing block's type is not handled.
	 *  (And is probably never necessary.) */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void SetBlockType(const FBlockType& NewBlockType);

	UFUNCTION(BlueprintPure)
	const FBlockType& GetBlockType() const;

	/** Get this block's current grid coordinates. */
	UFUNCTION(BlueprintPure)
	FIntPoint GetCoords() const;

	/** Accessor to get the grid this block belongs to. */
	UFUNCTION(BlueprintPure)
	AMMPlayGrid* Grid() const;

	/** The cell this block is currently in. This will either be OwningGridCell or SettleToCell. */
	UFUNCTION(BlueprintPure)
	AMMPlayGridCell* Cell() const;

	/** Does this block have the given category? */
	UFUNCTION(BlueprintPure)
	bool HasCategory(const FName& CategoryName);

	/** Does this block match the other block? */
	UFUNCTION(BlueprintPure)
	bool Matches(const AMMBlock* OtherBlock);

	/** Is this block currently part of a match? */
	UFUNCTION(BlueprintPure)
	bool IsMatched() const;

	/** Is the given match one of this block's current matches? If not, then this block has finished the match. */
	UFUNCTION(BlueprintPure)
	bool IsMatchFinished(const UBlockMatch* Match) const;

	UFUNCTION(BlueprintPure)
	bool CanMove() const;

	UFUNCTION(BlueprintPure)
	bool IsIndestructible() const;

	AMMPlayGridCell* FindSettleCell();

	/** Move this block to the given cell.If the cell is not occupied this will set OwningGridCell, otherwise this will set SettleToCell. */
	UFUNCTION(BlueprintCallable)
	bool ChangeOwningGridCell(AMMPlayGridCell* ToCell);

	/** The local distance from the block's current location to the cell it is moving/setting to. */
	UFUNCTION(BlueprintPure)
	float DistanceToCell();

	UFUNCTION(BlueprintPure)
	float PercentMoveComplete();

	/** Handle the block being clicked */
	UFUNCTION()
	void BlockClicked(UPrimitiveComponent* ClickedComp, FKey ButtonClicked);

	/** Handle the block being touched  */
	UFUNCTION()
	void OnFingerPressedBlock(ETouchIndex::Type FingerIndex, UPrimitiveComponent* TouchedComponent);

	void HandleClicked();

	UFUNCTION(BlueprintCallable)
	void Highlight(bool bOn);

	/** Destroys block actor. Only cleans up itself. i.e. grid, etc. must clean their own refs, etc. */
	void DestroyBlock();

	/** Get the goods dropped for a "normal", minimum size, block match. */
	UFUNCTION(BlueprintNativeEvent)
	TArray<FGoodsQuantity> GetBaseMatchGoods(const UGoodsDropper* GoodsDropper, const float QuantityScale = -1.f) const;

	/** Get the goods dropped for the given match. */
	UFUNCTION(BlueprintNativeEvent)
	TArray<FGoodsQuantity> GetMatchGoods(const UGoodsDropper* GoodsDropper, const UBlockMatch* Match);
	
	/** Notifications from the grid to this block */

	/** Grid calls when block is moved */
	UFUNCTION(BlueprintNativeEvent)
	void OnMove(const AMMPlayGridCell* ToCell);

	/** Grid calls when block is moved in an invalid move. */
	UFUNCTION(BlueprintNativeEvent)
	void OnMoveFail(const AMMPlayGridCell* ToCell);

	/** Grid calls when block is matched */
	UFUNCTION(BlueprintNativeEvent)
	void OnMatched(UBlockMatch* Match);
	virtual void OnMatched_Implementation(UBlockMatch* Match);

	/** Block has been destroyed outside of a match */
	UFUNCTION(BlueprintNativeEvent)
	void OnBlockDestroyed();

	/** Grid calls when block is unsettled. */
	UFUNCTION(BlueprintNativeEvent)
	void OnUnsettle();

	/** Grid calls when block begins settling. Our small settle delay begins after this is called. */
	UFUNCTION(BlueprintNativeEvent)
	void OnSettle();

	/** If this block takes damage, this will reduce current health by the DamageAmount and returns new current health.
	 *  If this results in the block's current health reaching <= 0, block will call MMGrid::BlockDestroyedByDamage(). 
	 *  If this block does not take damage, no changes are made and Max(current health, 1) is returned. 
	 *  BP can override to apply FX, but MUST call parent to apply damage .*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	int32 TakeDamage(const int32 DamageAmount);

	/** Override AActor TakeDamage() to call MMBlock::TakeDamage(int32)
	 *  If BP overrides this, call parent as well. */
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	/** This block finshing an action/

	/** This block has finished it's player-initiated movement. Notifies grid. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void MoveFinished();

	/** This block has finished it's matching. Notifies grid. */
	UFUNCTION(BlueprintCallable)
	void MatchFinished(UPARAM(ref) UBlockMatch* Match);
	
	/** This block has finished settling. Notifies grid. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void SettleFinished();

	/** Location stuff */

	/** Gets this block's location relative to the grid. */
	FVector GetRelativeLocation();

protected:
	
	UFUNCTION(BlueprintNativeEvent)
	void UpdateBlockVis();

	virtual void BeginPlay() override;

public:
	/** Returns Root scene subobject **/
	FORCEINLINE class USceneComponent* GetSceneRoot() const { return SceneRoot; }
	/** Returns BlockMesh subobject **/
	FORCEINLINE class UStaticMeshComponent* GetBlockMesh() const { return BlockMesh; }

};



/** Simple container for Blocks */
USTRUCT(BlueprintType)
struct FBlockSet
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	TArray<AMMBlock*> Blocks;
};


