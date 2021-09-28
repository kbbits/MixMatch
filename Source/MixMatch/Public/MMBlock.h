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

	/** Dummy root component */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Block, meta = (AllowPrivateAccess = "true"))
	class USceneComponent* SceneRoot;

	/** StaticMesh component for the clickable block */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Block, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* BlockMesh;

public:
	AMMBlock();

	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(BlueprintReadWrite, Category = Block)
	EMMGridState BlockState;

	/** Are we currently highlighted? */
	bool bIsHighlighted;

	bool bMovedByPlayer = false;

	bool bMatchedHorizontal;

	bool bMatchedVertical;

	bool bMatchFinished = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Block)
	TAssetPtr<USoundBase> MatchSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Block)
	TAssetPtr<USoundBase> StopMoveSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Block)
	UCurveFloat* MoveCurve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Block)
	UCurveFloat* MoveFailCurve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Block)
	float SettleFallDelay;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Block)
	class UMaterialInterface* BaseMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Block)
	class UMaterialInterface* AltMaterial;
		
	/** Grid cell where we should be located */
	UPROPERTY(BlueprintReadOnly)
	AMMPlayGridCell* OwningGridCell;

	bool bUnsettled = false;

	bool bFallingIntoGrid;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FBlockType BlockType;

	AMMPlayGrid* OwningGrid;

	AMMPlayGridCell* MoveToCell;

	bool bMoveSuccessful;

	float StartMoveDistance;

	FTimerHandle SettleFallDelayHandle;

	UPROPERTY()
	class UMaterialInstanceDynamic* BaseMatDynamic = nullptr;
	UPROPERTY()
	class UMaterialInstanceDynamic* AltMatDynamic = nullptr;

private:

	//UPROPERTY()
	const FBlockMatch* CurrentMatch = nullptr;

public:

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintNativeEvent)
	bool MoveTick(float DeltaSeconds);

	// Base class does nothing but change state to normal and call MatchFinished.
	UFUNCTION(BlueprintNativeEvent)
	bool MatchTick(float DeltaSeconds);

	UFUNCTION(BlueprintNativeEvent)
	bool SettleTick(float DeltaSeconds);

	UFUNCTION(BlueprintCallable)
	void SetBlockType(const FBlockType& NewBlockType);

	UFUNCTION(BlueprintCallable)
	FBlockType& GetBlockType();

	UFUNCTION(BlueprintPure)
	FIntPoint GetCoords();

	UFUNCTION(BlueprintPure)
	AMMPlayGrid* Grid();

	UFUNCTION(BlueprintPure)
	bool Matches(const AMMBlock* OtherBlock);

	UFUNCTION(BlueprintPure)
	bool IsMatched();

	UFUNCTION(BlueprintPure)
	bool CanMove();

	AMMPlayGridCell* FindSettleCell();

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

	void Highlight(bool bOn);

	void DestroyBlock();

	/** Get the goods dropped for a normal, minimum sized match. */
	TArray<FGoodsQuantity> GetMatchGoods(UGoodsDropper* GoodsDropper);
	
	UFUNCTION(BlueprintNativeEvent)
	void OnMove(const AMMPlayGridCell* ToCell);

	UFUNCTION(BlueprintNativeEvent)
	void OnMoveFail(const AMMPlayGridCell* ToCell);

	UFUNCTION(BlueprintNativeEvent)
	void OnMatched(const FBlockMatch& Match);

	void OnMatched_Native(const FBlockMatch* Match);

	UFUNCTION(BlueprintNativeEvent)
	void OnUnsettle();

	UFUNCTION(BlueprintNativeEvent)
	void OnSettle();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void MoveFinished();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void MatchFinished(const FBlockMatch& Match);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void SettleFinished();

protected:

	void UpdateBlockVis();

	virtual void BeginPlay() override;

public:
	/** Returns DummyRoot subobject **/
	FORCEINLINE class USceneComponent* GetSceneRoot() const { return SceneRoot; }
	/** Returns BlockMesh subobject **/
	FORCEINLINE class UStaticMeshComponent* GetBlockMesh() const { return BlockMesh; }



};


