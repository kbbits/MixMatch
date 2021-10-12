// Copyright Epic Games, Inc. All Rights Reserved.

#include "MMBlock.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "..\MixMatch.h"
#include "MMPlayGrid.h"
#include "MMGameMode.h"
#include "Goods/GoodsDropper.h"
#include "Goods/GoodsFunctionLibrary.h"


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
			, BaseMaterial(TEXT("/Game/MixMatch/Assets/Materials/BlockBase_M.BlockBase_M"))
			, AltMaterial(TEXT("/Game/MixMatch/Assets/Materials/BlockBlue_MI.BlockBlue_MI"))
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
			// Grid processes settle ticks
			//SettleTick(DeltaSeconds);
			break;
	}
}


bool AMMBlock::MoveTick_Implementation(float DeltaSeconds)
{
	// Do movement
	if (bMoveSuccessful) {
		float TargetDistance = DistanceToCell();
		// Check if we're done settling/moving
		if (TargetDistance <= 1.f)
		{
			SetActorRelativeLocation(Cell()->GetBlockLocalLocation());
			MoveFinished();
		}
		else
		{
			float MoveSpeed = 100;
			AMMGameMode* GameMode = Cast<AMMGameMode>(UGameplayStatics::GetGameMode(this));
			if (GameMode) {
				MoveSpeed = GameMode->GetBlockMoveSpeed();
			}
			if (IsValid(MoveCurve)) {
				MoveSpeed = MoveSpeed * MoveCurve->GetFloatValue(PercentMoveComplete());
			}
			FVector DirVector = Cell()->GetBlockLocalLocation() - GetRelativeLocation();
			DirVector.Normalize(.002);
			SetActorRelativeLocation(GetRelativeLocation() + (FMath::Min(MoveSpeed * DeltaSeconds, TargetDistance) * DirVector));
		}
	}
	else {
		MoveFinished();
	}
	return true;
}


bool AMMBlock::MatchTick_Implementation(float DeltaSeconds)
{
	// Base class does nothing but call MatchFinished on all current matches.
	if (CurrentMatches.Num() > 0) 
	{
		TArray<UBlockMatch*> MatchesToFinish;
		MatchesToFinish.Append(CurrentMatches);
		for (UBlockMatch* CurMatch : MatchesToFinish) {
			MatchFinished(CurMatch);
		}
	}
	else {
		UE_LOG(LogMMGame, Warning, TEXT("MMBlock::MatchTick - Called match tick with no current match."));
	}
	return true;
}


bool AMMBlock::SettleTick_Implementation(float DeltaSeconds)
{
	// Do settling movement
	float TargetDistance = DistanceToCell();
	if (bMoveSuccessful) 
	{
		if (bFallingIntoGrid)
		{
			AMMPlayGridCell* TopCell = Grid()->GetTopCell(Cell()->X);
			check(TopCell);
			// Check if block is low enough to no longer be "falling into grid"
			if (GetRelativeLocation().Z <= (TopCell->GetBlockLocalLocation().Z + FMath::Max(Grid()->NewBlockDropInHeight, 10.f))) 
			{
				// If we are low enough try to occupy our cell if we aren't already. (should be the top cell of the column)
				if (SettleToGridCell) {
					ChangeOwningGridCell(SettleToGridCell);
				}
				bFallingIntoGrid = false;
				Grid()->BlocksFallingIntoGrid[GetCoords().X].Blocks.RemoveSingle(this);
			}
		}
		// Check if we're done settling/moving
		if (TargetDistance <= 1.f)
		{
			// Re-check our settle location
			AMMPlayGridCell* SettleCell = FindSettleCell();
			if (SettleCell != nullptr && SettleCell != OwningGridCell)
			{
				ChangeOwningGridCell(SettleCell);
				TargetDistance = DistanceToCell();
				StartMoveDistance += TargetDistance;
			}
		}
		// If we are still near our settle target, finsh settle movement
		if (TargetDistance <= 1.f)
		{
			SetActorRelativeLocation(Cell()->GetBlockLocalLocation());
			if (SettleToGridCell) 
			{
				if (ChangeOwningGridCell(SettleToGridCell)) {
					SettleFinished();
				}
				else {
					BlockSettleFails++;
				}
			}
			else {
				SettleFinished();
			}
		}
		else
		{
			float MoveSpeed = 100.f;
			AMMGameMode* GameMode = Cast<AMMGameMode>(UGameplayStatics::GetGameMode(this));
			if (GameMode) {
				MoveSpeed = GameMode->GetBlockMoveSpeed();
			}
			if (IsValid(MoveCurve)) {
				MoveSpeed = MoveSpeed * MoveCurve->GetFloatValue(PercentMoveComplete());
			}
			FVector DirVector = Cell()->GetBlockLocalLocation() - GetRelativeLocation();
			DirVector.Normalize(.002);
			SetActorRelativeLocation(GetRelativeLocation() + (FMath::Min(MoveSpeed * DeltaSeconds, TargetDistance) * DirVector));
		}
		// Sanity check. Shouldn't be needed....
		if (BlockSettleFails >= 100)
		{
			UE_LOG(LogMMGame, Error, TEXT("MMBlock::SettleTick - Block %s at %s failed to settle after %d tries"), *GetName(), *Cell()->GetCoords().ToString(), BlockSettleFails);
			Grid()->DebugBlocks("SettleTick Failed");
			SettleFinished();
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


FIntPoint AMMBlock::GetCoords() const
{
	if (Cell() == nullptr) { return FIntPoint::NoneValue; }
	return Cell()->GetCoords();
}


AMMPlayGrid* AMMBlock::Grid() const
{
	if (Cell() != nullptr) {
		return Cell()->OwningGrid;
	}
	return nullptr;
}


AMMPlayGridCell* AMMBlock::Cell() const
{
	if (SettleToGridCell != nullptr) {
		return SettleToGridCell;
	}
	else if (OwningGridCell != nullptr) {
		return OwningGridCell;
	}
	return nullptr;
}


bool AMMBlock::HasCategory(const FName& CategoryName)
{
	return GetBlockType().BlockCategories.Contains(CategoryName);
}


bool AMMBlock::Matches(const AMMBlock* OtherBlock)
{
	if (OtherBlock == nullptr) { return false; }
	//UE_LOG(LogMMGame, Log, TEXT("Block::Matches %s == %s  %s"), *BlockType.MatchCode.ToString(), *OtherBlock->BlockType.MatchCode.ToString(), BlockType == OtherBlock->BlockType ? TEXT("True") : TEXT("False"));
	return BlockType == OtherBlock->BlockType;
}


bool AMMBlock::IsMatched() const
{
	return bMatchedHorizontal || bMatchedVertical;
}


bool AMMBlock::IsMatchFinished(const UBlockMatch* Match) const
{
	return !CurrentMatches.Contains(Match);
}


bool AMMBlock::CanMove() const
{
	return !BlockType.bImmobile;
}


AMMPlayGridCell* AMMBlock::FindSettleCell()
{
	// Determine where to settle to
	int32 AllOpenBelow = 0;
	if (Grid())
	{
		AMMPlayGridCell* NextCell = Grid()->GetCell(GetCoords() + FIntPoint(0, -1));
		if (NextCell && !IsValid(NextCell->CurrentBlock)) {
			return NextCell;
		}
		/*FIntPoint Coords = Cell()->GetCoords();
		AMMPlayGridCell* NextCell;
		for (int32 i = Coords.Y - 1; i >= 0; i--)
		{
			NextCell = Grid()->GetCell(FIntPoint(Coords.X, i));
			if (!IsValid(NextCell->CurrentBlock)) {
				AllOpenBelow++;
			}		
		}
		if (AllOpenBelow > 0) 
		{
			AMMPlayGridCell* FoundCell = Grid()->GetCell(Coords + FIntPoint(0, -AllOpenBelow));
			return FoundCell;
		}	*/
	}	
	return nullptr;
}


bool AMMBlock::ChangeOwningGridCell(AMMPlayGridCell* ToCell)
{
	check(ToCell);
	// Set new cell
	if (IsValid(ToCell->CurrentBlock) && ToCell->CurrentBlock != this)
	{
		if (SettleToGridCell != ToCell)
		{
			// New cell is occupied, so set SettleToGridCell instead
			UE_LOG(LogMMGame, Log, TEXT("MMBlock::ChangeOwningGridCell - Changing block %s at %s to cell %s that already has different block: %s"), *GetName(), *GetCoords().ToString(), *ToCell->GetCoords().ToString(), *ToCell->CurrentBlock->GetName());
			SettleToGridCell = ToCell;
		}
		return false;
	}
	else
	{
		AMMPlayGridCell* OldCell = Cell();
		AMMPlayGridCell* OldOwningCell = OwningGridCell;
		OwningGridCell = ToCell;
		OwningGridCell->CurrentBlock = this;
		SettleToGridCell = nullptr;
		if (OldCell) {
			UE_LOG(LogMMGame, Log, TEXT("MMBlock::ChangeOwningGridCell - Changing block %s at %s to cell %s"), *GetName(), *OldCell->GetCoords().ToString(), *GetCoords().ToString());
		}
		else {
			UE_LOG(LogMMGame, Log, TEXT("MMBlock::ChangeOwningGridCell - Adding new block %s to cell %s"), *GetName(), *GetCoords().ToString());
		}
		// Clear current cell
		if (OldOwningCell && OldOwningCell != OwningGridCell && OldOwningCell->CurrentBlock == this) 
		{
			UE_LOG(LogMMGame, Log, TEXT("                                 Block %s at %s cleared it's old cell %s"), *GetName(), *GetCoords().ToString(), *OldOwningCell->GetCoords().ToString());
			OldOwningCell->CurrentBlock = nullptr;
			Grid()->CellBecameOpen(OldOwningCell);			
		}
		return true;
	}	
	return false;
}


float AMMBlock::DistanceToCell()
{
	// We want the distance to cell that we want to go to.
	return FVector::Distance(GetRelativeLocation(), Grid()->GridCoordsToLocalLocation(Cell()->GetCoords()));
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
	//OwningGrid = nullptr;
	OwningGridCell = nullptr;
	SettleToGridCell = nullptr;
	CurrentMatches.Empty();
	Destroy();
}


TArray<FGoodsQuantity> AMMBlock::GetBaseMatchGoods_Implementation(const UGoodsDropper* GoodsDropper, const float QuantityScale)
{
	// const_cast because goods dropper must be passed as const in order to appear as input pin. GoodsDropper is not originally declared const.
	// UPARAM(ref) does not work since UObjects must be passed as pointers.
	UGoodsDropper* Dropper = const_cast<UGoodsDropper*>(GoodsDropper);
	return Dropper->EvaluateGoodsDropSet(GetBlockType().MatchDropGoods, QuantityScale);
}


TArray<FGoodsQuantity> AMMBlock::GetMatchGoods_Implementation(const UGoodsDropper* GoodsDropper, const UBlockMatch* Match)
{
	// const_cast because goods dropper must be passed as const in order to appear as input pin. GoodsDropper is not originally declared const.
	// UPARAM(ref) does not work since UObjects must be passed as pointers.
	UGoodsDropper* Dropper = const_cast<UGoodsDropper*>(GoodsDropper);
	int32 BonusMatchSize = Match->Blocks.Num() - Grid()->GetMinimumMatchSize();
	if (BonusMatchSize == 0 || GetBlockType().BonusMatchGoodsMultiplier == 0.f) {
		return GetBaseMatchGoods(Dropper);
	}
	else {
		return UGoodsFunctionLibrary::MultiplyGoodsQuantities(
			GetBaseMatchGoods(Dropper),
			(BonusMatchSize * GetBlockType().BonusMatchGoodsMultiplier) + 1.f
		);
	}
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


void AMMBlock::OnMatched_Implementation(UBlockMatch* Match)
{
	BlockState = EMMGridState::Matching;
	if (GetBlockMesh()) {
		GetBlockMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	CurrentMatches.AddUnique(Match);
}


void AMMBlock::OnUnsettle_Implementation()
{
	AMMPlayGridCell* SettleCell = nullptr;
	if (bUnsettled)
	{
		//if (!bFallingIntoGrid)
		//{
		//	SettleCell = FindSettleCell();
		//	if (SettleCell)	{
		//		bMoveSuccessful = true;
		//	}
		//}
		UE_LOG(LogMMGame, Log, TEXT("Unsettling block %s at %s that was already unsettled"), *GetName(), *GetCoords().ToString())
		//BlockState = EMMGridState::Settling;
		return;
	}
	if (CanMove() && !IsMatched()) {
		bUnsettled = true;
		UE_LOG(LogMMGame, Log, TEXT("Unsettling block %s at %s"), *GetName(), *GetCoords().ToString())
		if (SettleFallDelay > 0) {
			GetWorldTimerManager().SetTimer(SettleFallDelayHandle, this, &AMMBlock::OnSettle, SettleFallDelay, false);
		}
		else {
			OnSettle();
		}
	}
	else 
	{
		bMoveSuccessful = false;
		BlockState = EMMGridState::Settling;
		UE_LOG(LogMMGame, Log, TEXT("Block %s at %s does not need to be unsettled"), *GetName(), *GetCoords().ToString());
		SettleFinished();
	}
}


void AMMBlock::OnSettle_Implementation()
{
	if (BlockState == EMMGridState::Settling) {
		UE_LOG(LogMMGame, Warning, TEXT("SettleBlock - Block %s at %s is already settling"), *GetName(), *GetCoords().ToString());
		return;
	}
	GetWorldTimerManager().ClearTimer(SettleFallDelayHandle);
	BlockSettleFails = 0;
	AMMPlayGridCell* SettleCell = nullptr;
	
	UE_LOG(LogMMGame, Log, TEXT("Settling block %s from %s"), *GetName(), *GetCoords().ToString());
	if (bFallingIntoGrid)
	{
		bMoveSuccessful = true;
		if (SettleToGridCell != nullptr)
		{
			// Immediately queue the next falling block to unsettle
			Grid()->UnsettleBlockAboveCell(Cell());
			ChangeOwningGridCell(SettleToGridCell);
		}
		//SettleCell = FindSettleCell();		
		//if (SettleCell != nullptr)
		//{
		//	ChangeOwningGridCell(SettleCell);
		//}
	}
	else 
	{
		// Check settle cell
		bMoveSuccessful = false;
		if (!IsMatched() && CanMove())
		{
			if (SettleToGridCell != nullptr)
			{
				UE_LOG(LogMMGame, Warning, TEXT("   Block %s at %s that was not falling into grid had SettleToGridCell populated"), *GetName(), *GetCoords().ToString());
				bMoveSuccessful = true;
				Grid()->UnsettleBlockAboveCell(Cell());
				ChangeOwningGridCell(SettleToGridCell);
			}
			SettleCell = FindSettleCell();
			if (SettleCell != nullptr) 
			{	
				bMoveSuccessful = true;
				FIntPoint OrigCoords = GetCoords();
				if (!ChangeOwningGridCell(SettleCell)) {
					UE_LOG(LogMMGame, Warning, TEXT("   Block %s at %s that was not falling into grid changed cell to %s occupied by %s"), *GetName(), *OrigCoords.ToString(), *GetCoords().ToString(), *SettleCell->CurrentBlock->GetName());
					if (OwningGridCell != nullptr) {
						Grid()->UnsettleBlockAboveCell(OwningGridCell);
					}
				}
			}						
		}
		if(!bMoveSuccessful)
		{				
			//bUnsettled = false;
			UE_LOG(LogMMGame, Log, TEXT("   Unsettled block %s at %s does not need settling"), *GetName(), *GetCoords().ToString());
			BlockState = EMMGridState::Settling;
			SettleFinished();
			return;				
		}
	}
	UE_LOG(LogMMGame, Log, TEXT("   Settling block %s to %s"), *GetName(), *GetCoords().ToString());
	StartMoveDistance = DistanceToCell();
	BlockState = EMMGridState::Settling;
}


void AMMBlock::MoveFinished_Implementation()
{
	bMovedByPlayer = false;
	bUnsettled = false;
	if (SettleToGridCell != nullptr) {
		if (!ChangeOwningGridCell(SettleToGridCell)) {
			UE_LOG(LogMMGame, Warning, TEXT("MMBlock::MoveFinished - Could not change block owning cell to final settle cell"));
		}
	}
	BlockState = EMMGridState::Normal;
	if (Grid()) {
		Grid()->BlockFinishedMoving(this, bMoveSuccessful);
	}
}


void AMMBlock::MatchFinished(UBlockMatch* Match)
{
	bool bWasCurrentMatch = CurrentMatches.RemoveSingle(Match) == 1;
	if (CurrentMatches.Num() == 0) {
		BlockState = EMMGridState::Normal;
	}
	if (bWasCurrentMatch && Grid())	{
		Grid()->BlockFinishedMatch(this, Match);
	}	
}


void AMMBlock::SettleFinished_Implementation()
{
	bUnsettled = false;
	//bFallingIntoGrid = false;
	//if (SettleToGridCell != nullptr) {
	//	if (!ChangeOwningGridCell(SettleToGridCell)) {
	//		UE_LOG(LogMMGame, Warning, TEXT("MMBlock::SettleFinished - Could not change block owning cell to final settle cell"));
	//	}
	//}
	UE_LOG(LogMMGame, Log, TEXT("Finish settling block %s at %s. Move success: %s"), *GetName(), *GetCoords().ToString(), bMoveSuccessful ? TEXT("true") : TEXT("false"));
	if (BlockState == EMMGridState::Settling)
	{
		BlockState = EMMGridState::Normal;
		if (Grid()) {
			Grid()->BlockFinishedMoving(this, bMoveSuccessful);
		}
	}
	else {
		BlockState = EMMGridState::Normal;
		UE_LOG(LogMMGame, Warning, TEXT("MMBlock::SettleFinished - Called when block state is not Settling"));
	}
}


FVector AMMBlock::GetRelativeLocation()
{
	if (Grid() == nullptr) {
		return FVector::ZeroVector;
	}
	return Grid()->GetTransform().InverseTransformPosition(GetActorLocation());
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
		if (!IsValid(BaseMatDynamic)) 
		{
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
		if (!IsValid(BaseMatDynamic)) 
		{
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