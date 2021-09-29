// Copyright Epic Games, Inc. All Rights Reserved.

#include "MMBlock.h"
#include "..\MixMatch.h"
#include "MMPlayGrid.h"
#include "MMGameMode.h"
#include "Goods/GoodsDropper.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"

AMMBlock::AMMBlock()
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UStaticMesh> PlaneMesh;
		ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> BaseMaterial;
		ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> AltMaterial;
		FConstructorStatics()
			: PlaneMesh(TEXT("/Game/Puzzle/Meshes/PuzzleCube.PuzzleCube"))
			, BaseMaterial(TEXT("/Game/MixMatch/Materials/BlockBase_M.BlockBase_M"))
			, AltMaterial(TEXT("/Game/MixMatch/Materials/BlockBlue_MI.BlockBlue_MI"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// Set defaults
	BaseMaterial = ConstructorStatics.BaseMaterial.Get();
	AltMaterial = ConstructorStatics.AltMaterial.Get();

	// Create root scene component
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// Create static mesh component
	BlockMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BlockMesh"));
	BlockMesh->SetStaticMesh(ConstructorStatics.PlaneMesh.Get());
	//BlockMesh->SetRelativeScale3D(FVector(0.25f, 0.1f, 0.25f));
	BlockMesh->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
	BlockMesh->SetupAttachment(SceneRoot);
	BlockMesh->OnClicked.AddDynamic(this, &AMMBlock::BlockClicked);
	BlockMesh->OnInputTouchBegin.AddDynamic(this, &AMMBlock::OnFingerPressedBlock);
	
	bFallingIntoGrid = false;
	SettleFallDelay = 0.2f;
	BlockState = EMMGridState::Normal;
	bMatchedHorizontal = false;
	bMatchedVertical = false;
}


void AMMBlock::OnConstruction(const FTransform& Transform)
{
	GetBlockMesh()->SetMaterial(0, BaseMaterial);
	UpdateBlockVis();
}


void AMMBlock::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	switch (BlockState) {
		case EMMGridState::Moving :
			MoveTick(DeltaSeconds);
			break;
		case EMMGridState::Matching :
			MatchTick(DeltaSeconds);
			break;
		case EMMGridState::Settling :
			SettleTick(DeltaSeconds);
			break;
	}
}


bool AMMBlock::MoveTick_Implementation(float DeltaSeconds)
{
	// Do movement
	if (bMoveSuccessful) {
		float TargetDistance = DistanceToCell();
		// Check if we're done settling/moving
		if (TargetDistance <= 0.1f)
		{
			SetActorRelativeLocation(OwningGridCell->GetBlockLocalLocation());
			MoveFinished();
		}
		else
		{
			float MoveSpeed = 100;
			AMMGameMode* GameMode = Cast<AMMGameMode>(UGameplayStatics::GetGameMode(this));
			if (GameMode) {
				MoveSpeed = GameMode->BlockMoveSpeed;
			}
			if (IsValid(MoveCurve)) {
				MoveSpeed = MoveSpeed * MoveCurve->GetFloatValue(PercentMoveComplete());
			}
			FVector DirVector = OwningGridCell->GetBlockLocalLocation() - GetRelativeLocation();
			DirVector.Normalize(.002);
			SetActorRelativeLocation(GetRelativeLocation() + (FMath::Min(MoveSpeed * DeltaSeconds, TargetDistance) * DirVector));
		}
	}
	else
	{
		MoveFinished();
	}
	return true;
}


bool AMMBlock::MatchTick_Implementation(float DeltaSeconds)
{
	// Base class does nothing but change state to normal and call MatchFinished.
	if (CurrentMatch) {
		MatchFinished(*CurrentMatch);
	}
	else {
		MatchFinished(FBlockMatch());
	}
	return true;
}


bool AMMBlock::SettleTick_Implementation(float DeltaSeconds)
{
	// Do settling movement
	float TargetDistance = DistanceToCell();
	if (bMoveSuccessful) 
	{
		// Check if we're done settling/moving
		if (TargetDistance <= 1.f)
		{
			AMMPlayGridCell* UnsettleAboveCell = nullptr;
			if (bFallingIntoGrid)
			{
				bFallingIntoGrid = false;
				UnsettleAboveCell = OwningGridCell;
			}
			// Re-check our settle location
			AMMPlayGridCell* SettleCell = FindSettleCell();
			if (SettleCell && SettleCell != OwningGridCell)
			{
				if (OwningGridCell->CurrentBlock == this) {
					OwningGridCell->CurrentBlock = nullptr;
				}
				OwningGridCell = SettleCell;
				OwningGridCell->CurrentBlock = this;
				TargetDistance = DistanceToCell();
				StartMoveDistance += TargetDistance;
				if (UnsettleAboveCell) {
					Grid()->UnsettleBlockAboveCell(UnsettleAboveCell);
				}
			}			
		}
		// If we are still near our settle target, finsh settle movement
		if (TargetDistance <= 1.f)
		{
			SetActorRelativeLocation(OwningGridCell->GetBlockLocalLocation());
			SettleFinished();
		}
		else
		{
			float MoveSpeed = 100.f;
			AMMGameMode* GameMode = Cast<AMMGameMode>(UGameplayStatics::GetGameMode(this));
			if (GameMode) {
				MoveSpeed = GameMode->BlockMoveSpeed;
			}
			if (IsValid(MoveCurve)) {
				MoveSpeed = MoveSpeed * MoveCurve->GetFloatValue(PercentMoveComplete());
			}
			FVector DirVector = OwningGridCell->GetBlockLocalLocation() - GetRelativeLocation();
			DirVector.Normalize(.002);
			SetActorRelativeLocation(GetRelativeLocation() + (FMath::Min(MoveSpeed * DeltaSeconds, TargetDistance) * DirVector));
		}
	}
	else
	{
		// Move not successful
		SettleFinished();
	}
	return true;
}


void AMMBlock::SetBlockType(const FBlockType& NewBlockType)
{
	BlockType = NewBlockType;
	UpdateBlockVis();
}

FBlockType& AMMBlock::GetBlockType()
{
	return BlockType;
}


FIntPoint AMMBlock::GetCoords()
{
	if (OwningGridCell == nullptr) { return FIntPoint::NoneValue; }
	return OwningGridCell->GetCoords();
}


AMMPlayGrid* AMMBlock::Grid()
{
	if (OwningGrid) { return OwningGrid; }
	if (OwningGridCell == nullptr) { return nullptr; }
	OwningGrid = OwningGridCell->OwningGrid;
	return OwningGrid;
}


bool AMMBlock::Matches(const AMMBlock* OtherBlock)
{
	if (OtherBlock == nullptr) { return false; }
	//UE_LOG(LogMMGame, Log, TEXT("Block::Matches %s == %s  %s"), *BlockType.MatchCode.ToString(), *OtherBlock->BlockType.MatchCode.ToString(), BlockType == OtherBlock->BlockType ? TEXT("True") : TEXT("False"));
	return BlockType == OtherBlock->BlockType;
}


bool AMMBlock::IsMatched()
{
	return bMatchedHorizontal || bMatchedVertical;
}


bool AMMBlock::CanMove()
{
	return !BlockType.bImmobile;
}


AMMPlayGridCell* AMMBlock::FindSettleCell()
{
	// Determine where to settle to
	AMMPlayGridCell* CurCell = OwningGridCell;
	bool bFoundBlockBelow = false;
	int32 OpenBelow = 0;
	if (Grid())
	{
		FIntPoint Coords = CurCell->GetCoords();
		AMMPlayGridCell* NextCell;
		for (int32 i = Coords.Y - 1; i >= 0; i--)
		{
			NextCell = Grid()->GetCell(FIntPoint(Coords.X, i));
			if (NextCell->CurrentBlock == nullptr)
			{
				CurCell = NextCell;
				if (!bFoundBlockBelow) { OpenBelow++; }
			}
			else 
			{
				bFoundBlockBelow = true;
			}		
		}
	}
	if (OpenBelow > 0) {
		return Grid()->GetCell(OwningGridCell->GetCoords() + FIntPoint(0, -OpenBelow));
	}
	else if (CurCell != OwningGridCell) {
		return Grid()->GetCell(OwningGridCell->GetCoords() + FIntPoint(0, -1));
	}
	return nullptr;
}


float AMMBlock::DistanceToCell()
{
	return FVector::Distance(GetRelativeLocation(), Grid()->GridCoordsToLocalLocation(OwningGridCell->GetCoords()));
}


float AMMBlock::PercentMoveComplete()
{
	if (StartMoveDistance > 0.f) {
		return (StartMoveDistance - DistanceToCell()) / StartMoveDistance;
	}
	else {
		return 1.f;
	}
}


void AMMBlock::BlockClicked(UPrimitiveComponent* ClickedComp, FKey ButtonClicked)
{
	HandleClicked();
}


void AMMBlock::OnFingerPressedBlock(ETouchIndex::Type FingerIndex, UPrimitiveComponent* TouchedComponent)
{
	HandleClicked();
}


void AMMBlock::HandleClicked()
{
	// Tell the Grid
	if (Grid() != nullptr) {
		Grid()->BlockClicked(this);
	}
}

void AMMBlock::Highlight(bool bOn)
{
	bIsHighlighted = bOn;
	UpdateBlockVis();
}


void AMMBlock::DestroyBlock()
{
	if (OwningGridCell && OwningGridCell->CurrentBlock == this) {
		OwningGridCell->CurrentBlock = nullptr;
	}
	BaseMatDynamic = nullptr;
	AltMatDynamic = nullptr;
	OwningGrid = nullptr;
	OwningGridCell = nullptr;
	CurrentMatch = nullptr;
	Destroy();
}


TArray<FGoodsQuantity> AMMBlock::GetMatchGoods(UGoodsDropper* GoodsDropper)
{
	return GoodsDropper->EvaluateGoodsDropSet(BlockType.MatchDropGoods);
}


void AMMBlock::OnMove_Implementation(const AMMPlayGridCell* ToCell)
{
	BlockState = EMMGridState::Moving;
	StartMoveDistance = DistanceToCell();
	bMoveSuccessful = true;
}


void AMMBlock::OnMoveFail_Implementation(const AMMPlayGridCell* ToCell)
{
	BlockState = EMMGridState::Moving;
	StartMoveDistance = 0.f;
	bMoveSuccessful = false;
}


void AMMBlock::OnMatched_Implementation(const FBlockMatch& Match)
{
	BlockState = EMMGridState::Matching;
}


void AMMBlock::OnMatched_Native(const FBlockMatch* Match)
{
	CurrentMatch = Match;
	if (CurrentMatch) {
		OnMatched(*CurrentMatch);
	}
	else
	{
		UE_LOG(LogMMGame, Error, TEXT("Block::OnMatched_Native - CurrentMatch is invalid"));
		return;
	}
}


void AMMBlock::OnUnsettle_Implementation()
{
	AMMPlayGridCell* SettleCell = nullptr;
	if (bUnsettled)
	{
		if (!bFallingIntoGrid)
		{
			SettleCell = FindSettleCell();
			if (SettleCell)
			{
				//MoveToCell = SettleCell;
				bMoveSuccessful = true;
			}
		}
		return;
	}
	// Determine where to settle to
	if (CanMove()) {
		if (bFallingIntoGrid) {
			SettleCell = OwningGridCell;
		}
		else {
			SettleCell = FindSettleCell();
		}		
	}
	if (SettleCell)
	{
		//MoveToCell = SettleCell;
		bUnsettled = true;
		bMoveSuccessful = true;
		if (SettleFallDelay > 0) {
			GetWorldTimerManager().SetTimer(SettleFallDelayHandle, this, &AMMBlock::OnSettle, SettleFallDelay, false);
		}
		else {
			OnSettle();
		}
	}
	else 
	{
		//MoveToCell = nullptr;
		bMoveSuccessful = false;
		BlockState = EMMGridState::Settling;
	}
}


void AMMBlock::OnSettle_Implementation()
{
	if (BlockState == EMMGridState::Settling) {
		return;
	}
	GetWorldTimerManager().ClearTimer(SettleFallDelayHandle);
	if (bMoveSuccessful) { // && MoveToCell) {
		AMMPlayGridCell* UnsettleAboveCell = OwningGridCell;
		if (!bFallingIntoGrid) 
		{
			// Check settle cell again.
			AMMPlayGridCell* SettleCell = FindSettleCell();
			if (SettleCell) {
				//MoveToCell = SettleCell;
				if (OwningGridCell->CurrentBlock == this) {
					OwningGridCell->CurrentBlock = nullptr;
				}
				OwningGridCell = SettleCell;
				OwningGridCell->CurrentBlock = this;
			}
		}		
		StartMoveDistance = DistanceToCell();
		BlockState = EMMGridState::Settling;
		// Notify grid
		if (!bFallingIntoGrid && UnsettleAboveCell && Grid()) {
			Grid()->UnsettleBlockAboveCell(UnsettleAboveCell);
		}		
	}
	else {
		MoveFinished();
	}
	//MoveToCell = nullptr;
}


void AMMBlock::MoveFinished_Implementation()
{
	BlockState = EMMGridState::Normal;
	if (Grid()) {
		Grid()->BlockFinishedMoving(this, bMoveSuccessful);
	}
}


void AMMBlock::MatchFinished_Implementation(const FBlockMatch& Match)
{
	bMatchFinished = true;
	CurrentMatch = nullptr;
	BlockState = EMMGridState::Normal;
	if (Grid())	{
		Grid()->BlockFinishedMatch(this, Match);
	}	
}


void AMMBlock::SettleFinished_Implementation()
{
	bUnsettled = false;
	BlockState = EMMGridState::Normal;
	if (Grid()) {
		Grid()->BlockFinishedMoving(this, bMoveSuccessful);
	}
}


FVector AMMBlock::GetRelativeLocation()
{
	if (Grid() == nullptr) {
		return FVector::ZeroVector;
	}
	return Grid()->GetTransform().InverseTransformPosition(GetActorLocation());
	//return GetActorLocation()- Grid()->GetActorLocation();
}


void AMMBlock::UpdateBlockVis()
{
	if (bIsHighlighted)
	{
		//if (AltMatDynamic == nullptr) {
		//	AltMatDynamic = GetBlockMesh()->CreateDynamicMaterialInstance(0, AltMaterial);
		//}
		//if (AltMatDynamic) {
		//	AltMatDynamic->SetVectorParameterValue("PrimaryColor", BlockType.AltColor);
		//	//GetBlockMesh()->SetMaterial(0, AltMaterial);
		//	GetBlockMesh()->SetMaterial(0, AltMatDynamic);
		//}
		if (!IsValid(BaseMatDynamic)) {
			BaseMatDynamic = GetBlockMesh()->CreateDynamicMaterialInstance(0, BaseMaterial);
			//GetBlockMesh()->SetMaterial(0, BaseMaterial);
			GetBlockMesh()->SetMaterial(0, BaseMatDynamic);
		}
		if (BaseMatDynamic) {
			BaseMatDynamic->SetVectorParameterValue("PrimaryColor", BlockType.AltColor);
		}
	}
	else
	{
		if (!IsValid(BaseMatDynamic)) {
			BaseMatDynamic = GetBlockMesh()->CreateDynamicMaterialInstance(0, BaseMaterial);
			//GetBlockMesh()->SetMaterial(0, BaseMaterial);
			GetBlockMesh()->SetMaterial(0, BaseMatDynamic);
		}
		if (BaseMatDynamic) {
			BaseMatDynamic->SetVectorParameterValue("PrimaryColor", BlockType.PrimaryColor);
		}		
	}
}


void AMMBlock::BeginPlay()
{
	Super::BeginPlay();
	UpdateBlockVis();
}