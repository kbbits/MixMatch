#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MMPlayGridCell.generated.h"

/** A cell of our play grid */
UCLASS(minimalapi)
class AMMPlayGridCell : public AActor
{
	GENERATED_BODY()

	/** Scene root component */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class USceneComponent* SceneRoot;

	/** StaticMesh component for the clickable block */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* CellMesh;

public:
	AMMPlayGridCell();

	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
	int32 X;

	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
	int32 Y;

	/** Are we currently active? */
	bool bIsActive;

	/** Pointer to base material used on the cell */
	UPROPERTY(EditDefaultsOnly)
	class UMaterialInterface* BaseMaterial;

	/** Pointer to material used on the focused cell */
	UPROPERTY(EditDefaultsOnly)
	class UMaterialInterface* AltMaterial;
		
	/** Grid that owns us */
	UPROPERTY(BlueprintReadOnly)
	class AMMPlayGrid* OwningGrid;

	UPROPERTY()
	class AMMBlock* CurrentBlock;

	

public:
	/** Returns Root subobject **/
	FORCEINLINE class USceneComponent* GetSceneRoot() const { return SceneRoot; }
	/** Returns BlockMesh subobject **/
	FORCEINLINE class UStaticMeshComponent* GetCellMesh() const { return CellMesh; }

	/** Handle the cell being clicked */
	UFUNCTION()
	void CellClicked(UPrimitiveComponent* ClickedComp, FKey ButtonClicked);

	/** Handle the cell being touched  */
	UFUNCTION()
	void OnFingerPressedCell(ETouchIndex::Type FingerIndex, UPrimitiveComponent* TouchedComponent);

	UFUNCTION(BlueprintCallable)
	void Highlight(bool bOn);

	UFUNCTION(BlueprintCallable)
	FIntPoint GetCoords() const;

	/* Get the world location for this cell's block */
	UFUNCTION(BlueprintCallable)
	FVector GetBlockWorldLocation();

	/* Get the local location for this cell's block, relative to grid */
	UFUNCTION(BlueprintCallable)
	FVector GetBlockLocalLocation();

	void DestroyCell();
};



