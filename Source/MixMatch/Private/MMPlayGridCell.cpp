// Copyright Epic Games, Inc. All Rights Reserved.

#include "MMPlayGridCell.h"
#include "MMBlock.h"
#include "MMPlayGrid.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

AMMPlayGridCell::AMMPlayGridCell()
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UStaticMesh> PlaneMesh;
		ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> BaseMaterial;
		ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> AltMaterial;
		FConstructorStatics()
			: PlaneMesh(TEXT("/Game/Puzzle/Meshes/PuzzleCube.PuzzleCube"))
			, BaseMaterial(TEXT("/Game/MixMatch/Materials/CellBase_M.CellBase_M"))	
			, AltMaterial(TEXT("/Game/MixMatch/Materials/CellHighlight_MI.CellHighlight_MI"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	// Create root scene component
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// Create static mesh component
	CellMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CellMesh"));
	CellMesh->SetStaticMesh(ConstructorStatics.PlaneMesh.Get());
	CellMesh->SetRelativeScale3D(FVector(0.38f, 0.1f, 0.38f));
	CellMesh->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
	CellMesh->SetMaterial(0, ConstructorStatics.BaseMaterial.Get());
	CellMesh->SetupAttachment(SceneRoot);
	CellMesh->OnClicked.AddDynamic(this, &AMMPlayGridCell::CellClicked);
	CellMesh->OnInputTouchBegin.AddDynamic(this, &AMMPlayGridCell::OnFingerPressedCell);

	// Save a pointer to the orange material
	BaseMaterial = ConstructorStatics.BaseMaterial.Get();
	AltMaterial = ConstructorStatics.AltMaterial.Get();
	CurrentBlock = nullptr;
}


void AMMPlayGridCell::CellClicked(UPrimitiveComponent* ClickedComp, FKey ButtonClicked)
{
	check(OwningGrid);
	OwningGrid->CellClicked(this);
}


void AMMPlayGridCell::OnFingerPressedCell(ETouchIndex::Type FingerIndex, UPrimitiveComponent* TouchedComponent)
{
	check(OwningGrid);
	OwningGrid->CellClicked(this);
}


void AMMPlayGridCell::Highlight(bool bOn)
{
	if (bOn) {
		CellMesh->SetMaterial(0, AltMaterial);
	}
	else {
		CellMesh->SetMaterial(0, BaseMaterial);
	}
}


FIntPoint AMMPlayGridCell::GetCoords() const
{
	return FIntPoint(X, Y);
}


FVector AMMPlayGridCell::GetBlockWorldLocation()
{
	if (OwningGrid) {
		return OwningGrid->GridCoordsToWorldLocation(GetCoords());
	}
	else {
		return FVector::ZeroVector;
	}
}


FVector AMMPlayGridCell::GetBlockLocalLocation()
{
	if (OwningGrid) {
		return OwningGrid->GridCoordsToLocalLocation(GetCoords());
	}
	else {
		return FVector::ZeroVector;
	}
}


void AMMPlayGridCell::DestroyCell()
{
	if (IsValid(CurrentBlock))
	{
		CurrentBlock->DestroyBlock();
		CurrentBlock = nullptr;
	}
	Destroy();
}