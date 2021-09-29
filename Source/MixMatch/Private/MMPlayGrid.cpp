// Copyright Epic Games, Inc. All Rights Reserved.

#include "MMPlayGrid.h"
#include "Engine/World.h"
#include "Components/TextRenderComponent.h"
#include "Components/BillboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "..\MixMatch.h"
#include "MMMath.h"
#include "MMPlayGridCell.h"
#include "MMBlock.h"
#include "MMGameMode.h"
#include "MMPlayerController.h"
#include "Goods/GoodsQuantity.h"


#define LOCTEXT_NAMESPACE "PuzzleBlockGrid"

AMMPlayGrid::AMMPlayGrid()
{
	// Create root scene component
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// Create score text component
	ScoreText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ScoreText"));
	ScoreText->SetRelativeLocation(FVector(0.f,0.f,-30.f));
	ScoreText->SetRelativeRotation(FRotator(0.f,90.f,0.f));
	ScoreText->SetText(FText::Format(LOCTEXT("ScoreFmt", "Score: {0}"), FText::AsNumber(0)));
	ScoreText->SetupAttachment(SceneRoot);

#if WITH_EDITORONLY_DATA
	// Billboard
	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	Billboard->SetupAttachment(SceneRoot);
#endif

	// Set defaults
	CellClass = AMMPlayGridCell::StaticClass();
	GridState = EMMGridState::Normal;
	SizeX = 5;
	SizeY = 7;
	BlockTypeSetName = FName("Default");
	bScaleBlocks = false;
	BlockSize = FVector(100.f, 100.f, 100.f);
	BlockMargin = 4.f;
	CellBackgroundMargin = 0.f;
	CellBackgroundOffset = (BlockSize.Z / 2.f) + 1.f;// 30.f;
}

void AMMPlayGrid::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	PlaySounds(PlaySoundQueue);
	PlaySoundQueue.Empty();
	switch (GridState) {
	case EMMGridState::Moving:
		//MoveTick(DeltaSeconds);
		if (UnsettledBlocks.Num() == 0) 
		{
			if (BlockMatches.Num() > 0)	{
				ResolveMatches();
			}
			else if (BlocksToCheck.Num() > 0) {
				CheckFlaggedForMatches();
			}
			else {
				GridState = EMMGridState::Normal;
			}
		}		
		break;
	case EMMGridState::Matching:
		//MatchTick(DeltaSeconds);
		if (BlockMatches.Num() == 0)
		{
			if (UnsettledBlocks.Num() > 0) {
				SettleBlocks();
			}
			else if(BlocksToCheck.Num() > 0) {
				CheckFlaggedForMatches();
			}
			else {
				GridState = EMMGridState::Normal;
			}
		}
		else if (bAllMatchesFinished) {
			AllMatchesFinished();
		}
		break;
	case EMMGridState::Settling:
		//SettleTick(DeltaSeconds);
		if (UnsettledBlocks.Num() == 0) 
		{
			if (BlocksToCheck.Num() > 0) {
				CheckFlaggedForMatches();
			}
			else {
				GridState = EMMGridState::Normal;
			}
		}
		break;
	}
}


void AMMPlayGrid::BeginPlay()
{
	Super::BeginPlay();

	SpawnGrid();
	FillGridBlocks();
}


int32 AMMPlayGrid::GetMinimumMatchSize()
{
	return 3;
}


bool AMMPlayGrid::SetBlockTypeSetName(const FName& NewBlockTypeSetName)
{
	BlockTypeSetName = NewBlockTypeSetName; 
	// TODO: Check block type set name is valid. i.e. it is in GameMode's block type sets table.
	return true;
}


FName AMMPlayGrid::GetBlockTypeSetName()
{
	return BlockTypeSetName;
}


void AMMPlayGrid::SpawnGrid()
{
	// Destroy any existing grid
	DestroyGrid();

	// Number of blocks
	const int32 NumCells = SizeX * SizeY;
	Cells.Empty(NumCells);
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;

	// Loop to spawn each block
	for (int32 Index = 0; Index < NumCells; Index++)
	{
		const float X = (Index % SizeX);
		const float Y = (Index / SizeX);
		FVector Rescale = FVector(1.f, 1.f, 1.f);

		// Make position vector, offset from Grid location
		const FVector CellLocation = GridCoordsToLocalLocation(FIntPoint(X, Y)) + FVector(0.f, -CellBackgroundOffset, 0.f);
		// Spawn a block
		AMMPlayGridCell* NewCell = GetWorld()->SpawnActor<AMMPlayGridCell>(CellClass, FVector::ZeroVector, GetActorRotation(), SpawnParams);
		// Set up cell properties
		if (NewCell != nullptr)
		{
			NewCell->X = X;
			NewCell->Y = Y;
			NewCell->OwningGrid = this;
			if (bScaleBlocks)
			{
				FVector CellOrigin;
				FVector CellBoxExtent;
				float CellSphereRadius;
				UKismetSystemLibrary::GetComponentBounds(NewCell->GetCellMesh(), CellOrigin, CellBoxExtent, CellSphereRadius);
				if (CellBoxExtent.X > 0.f) {
					Rescale.X = BlockSize.X / (CellBoxExtent.X * 2);
				}
				if (CellBoxExtent.Y > 0.f) {
					Rescale.Y = BlockSize.Y / (CellBoxExtent.Y * 2);
					// World Y is our grid depth.
					// Only scale by Y if scaling down. i.e. don't inflate to fill Y depth.
					if (Rescale.Y > 1.f) {
						Rescale.Y = 1.f;
					}
				}
				if (CellBoxExtent.Z > 0.f) {
					Rescale.Z = BlockSize.Z / (CellBoxExtent.Z * 2);
				}
				//Rescale = Rescale.GetAbsMin() * FVector::OneVector;
				// Scale by smallest X or Z, ignore Y
				Rescale = Rescale.X < Rescale.Z ? Rescale.X * FVector::OneVector : Rescale.Z * FVector::OneVector;
				NewCell->GetCellMesh()->SetWorldScale3D(NewCell->GetCellMesh()->GetRelativeScale3D() * Rescale);
			}
			NewCell->AttachToActor(this, FAttachmentTransformRules::SnapToTargetIncludingScale);
			NewCell->SetActorRelativeLocation(CellLocation, false, nullptr, ETeleportType::ResetPhysics);
			// Add it to our cell array
			Cells.Add(NewCell);
		}
	}
}


void AMMPlayGrid::DestroyGrid()
{
	for (AMMPlayGridCell* Cell : Cells)
	{
		Cell->DestroyCell();
	}
	UnsettledBlocks.Empty();
	BlocksToCheck.Empty();
	BlockMatches.Empty();
	Blocks.Empty();
	Cells.Empty();
}


void AMMPlayGrid::DestroyBlocks()
{
	for (AMMPlayGridCell* Cell : Cells)
	{
		if (Cell->CurrentBlock) {
			Cell->CurrentBlock->DestroyBlock();
		}
	}
	UnsettledBlocks.Empty();
	BlocksToCheck.Empty();
	BlockMatches.Empty();
	Blocks.Empty();	
}


bool AMMPlayGrid::GetRandomBlockTypeNameForCell_Implementation(const AMMPlayGridCell* Cell, FName& FoundBlockTypeName)
{
	AMMGameMode* GameMode = Cast<AMMGameMode>(UGameplayStatics::GetGameMode(this));
	if (Cell == nullptr) {
		UE_LOG(LogMMGame, Error, TEXT("MMPlayGrid::GetRandomBlockTypeNameForCell - Called with null cell."));
		return false;
	}
	if (GameMode == nullptr) {
		UE_LOG(LogMMGame, Error, TEXT("MMPlayGrid::GetRandomBlockTypeNameForCell - Cannot get game mode"));
		return false;
	}
	return GameMode->GetRandomBlockTypeNameForCell(Cell, FoundBlockTypeName);
}


bool AMMPlayGrid::AddBlockInCell(AMMPlayGridCell* Cell, const FName& BlockTypeName, const float OffsetAboveCell, const bool bAllowUnsettle)
{
	AMMGameMode* GameMode = Cast<AMMGameMode>(UGameplayStatics::GetGameMode(this));
	if (Cell == nullptr) {
		UE_LOG(LogMMGame, Error, TEXT("MMPlayGrid::AddBlockInCell - Called with null cell."));
		return false;
	}
	if (GameMode == nullptr) {
		UE_LOG(LogMMGame, Error, TEXT("MMPlayGrid::AddBlockInCell - Cannot get game mode"));
		return false;
	}
	FVector BlockLocation = Cell->GetBlockLocalLocation() + FVector(0.f, 0.f, OffsetAboveCell);
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;
	FBlockType BlockType;
	FVector Rescale = FVector::OneVector;
	if (GameMode->GetBlockTypeByName(BlockTypeName, BlockType))
	{
		TSubclassOf<AMMBlock> BlockClass = BlockType.BlockClass.LoadSynchronous();
		AMMBlock* NewBlock = GetWorld()->SpawnActor<AMMBlock>(
			BlockClass, 
			FVector::ZeroVector, 
			GetActorRotation(), 
			SpawnParams
		);
		if (NewBlock)
		{
			NewBlock->SetBlockType(BlockType);
			NewBlock->OwningGridCell = Cell;
			NewBlock->bFallingIntoGrid = (Cell->GetCoords().Y == SizeY - 1) && OffsetAboveCell > 0.f;
			NewBlock->Grid();
			Cell->CurrentBlock = NewBlock;
			if (bScaleBlocks)
			{
				FVector BlockOrigin;
				FVector BlockBoxExtent;
				float BlockSphereRadius;
				UKismetSystemLibrary::GetComponentBounds(NewBlock->GetBlockMesh(), BlockOrigin, BlockBoxExtent, BlockSphereRadius);
				if (BlockBoxExtent.X > 0.f) {
					Rescale.X = (BlockSize.X - (BlockMargin * 2)) / (BlockBoxExtent.X * 2);
				}
				//if (BlockBoxExtent.Y > 0.f) {
				//	// Y axis, the depth away from camera, has no margin applied.
				//	Rescale.Y = BlockSize.Y / (BlockBoxExtent.Y * 2);
				//	// World Y is our grid depth.
				//	// Only scale by Y if scaling down. i.e. don't inflate to fill Y depth.
				//	if (Rescale.Y > 1.f) {
				//		Rescale.Y = 1.f;
				//	}
				//}
				if (BlockBoxExtent.Z > 0.f) {
					Rescale.Z = (BlockSize.Z - (BlockMargin * 2)) / (BlockBoxExtent.Z * 2);
				}
				// Scale by smallest X or Z, ignore Y
				Rescale = Rescale.X < Rescale.Z ? Rescale.X * FVector::OneVector : Rescale.Z * FVector::OneVector;
				NewBlock->GetBlockMesh()->SetWorldScale3D(NewBlock->GetBlockMesh()->GetRelativeScale3D() * Rescale);
			}
			NewBlock->AttachToActor(this, FAttachmentTransformRules::SnapToTargetIncludingScale);
			NewBlock->SetActorRelativeLocation(BlockLocation, false, nullptr, ETeleportType::ResetPhysics);
			Blocks.Add(NewBlock);
			BlocksToCheck.AddUnique(NewBlock);
			if (bAllowUnsettle) {
				UnsettleBlock(NewBlock);
			}
			return true;
		}
		else {
			UE_LOG(LogMMGame, Error, TEXT("MMPlayGrid::AddBlockInCell - Error spawning new block at coords %s"), *Cell->GetCoords().ToString());
			return false;
		}
	}
	return false;
}


void AMMPlayGrid::AddRandomBlockInCell(AMMPlayGridCell* Cell, const float OffsetAboveCell, const bool bAllowUnsettle, const bool bPreventMatches)
{
	if (Cell == nullptr || Cell->CurrentBlock != nullptr) { return; }
	FName BlockTypeName;
	bool bTryAgain = true;
	const int32 MaxTries = 50;
	int32 Tries = 0;
	while (bTryAgain && Tries < MaxTries)
	{
		Tries++;
		bTryAgain = false;
		if (GetRandomBlockTypeNameForCell(Cell, BlockTypeName))
		{
			// AddBlockInCell should not allow unsettling if we are preventing matches. If we are preventing matches, we will unsettle the cell
			// ourselves if the block was added successfully without matching.
			if (AddBlockInCell(Cell, BlockTypeName, OffsetAboveCell, bAllowUnsettle && !bPreventMatches))
			{
				if (Cell->CurrentBlock)
				{
					if (bPreventMatches)
					{
						//UE_LOG(LogMMGame, Log, TEXT("Checking new block for matches %s"), *Cell->CurrentBlock->GetCoords().ToString());
						FBlockMatch BlockMatchHoriz;
						FBlockMatch BlockMatchVert;
						if (CheckForMatches(Cell->CurrentBlock, BlockMatchHoriz, BlockMatchVert, false))
						{
							//UE_LOG(LogMMGame, Log, TEXT("   New block matches"));
							Blocks.RemoveSingle(Cell->CurrentBlock);
							BlocksToCheck.RemoveSingle(Cell->CurrentBlock);
							UnsettledBlocks.RemoveSingle(Cell->CurrentBlock);
							Cell->CurrentBlock->DestroyBlock();
							bTryAgain = true;
							if (Tries == MaxTries) {
								UE_LOG(LogMMGame, Log, TEXT("MMPlayGrid::AddRandomBlockInCell - Exceeded max tries to find an unmatching block type in cell %s"), *Cell->GetCoords().ToString());
							}
						}
						else {
							// Block was added with no match. Then unsettle the block now if unsettling is allowed.
							if (bAllowUnsettle) {
								UnsettleBlock(Cell->CurrentBlock);
							}
						}
					}
				}
			}
		}
	}
}


void AMMPlayGrid::FillGridBlocks()
{
	DestroyBlocks();
	AMMPlayGridCell* Cell = nullptr;
	for (int32 i = 0; i < Cells.Num(); i++)
	{
		AddRandomBlockInCell(Cells[i], 0.f, false, true);
	}
	UE_LOG(LogMMGame, Log, TEXT("MMPlayGrid::FillGridBlocks - Added %d blocks to grid"), Blocks.Num());
}


AMMPlayGridCell* AMMPlayGrid::GetCell(const FIntPoint& Coords)
{
	if (Coords.X < 0 || Coords.X >= SizeX) { return nullptr; }
	if (Coords.Y < 0 || Coords.Y >= SizeY) { return nullptr; }
	int32 Index = (Coords.Y * SizeX) + Coords.X;
	if (Index >= Cells.Num()) { return nullptr; }
	return Cells[Index];
}


AMMBlock* AMMPlayGrid::GetBlock(const FIntPoint& Coords)
{
	AMMPlayGridCell* Cell = GetCell(Coords);
	if (Cell != nullptr)
	{
		return Cell->CurrentBlock;
	}
	return nullptr;
}


FVector AMMPlayGrid::GridCoordsToWorldLocation(const FIntPoint& GridCoords)
{
	float BlockSpacingX = BlockSize.X + CellBackgroundMargin;
	float BlockSpacingY = BlockSize.Z + CellBackgroundMargin; // the grid's Y axis is actually world Z;
	float XOffset = (GridCoords.X * BlockSpacingX) + (BlockSpacingX / 2.f);
	float YOffset = (GridCoords.Y * BlockSpacingY) + (BlockSpacingY / 2.f);
	return GetActorTransform().InverseTransformPosition(FVector(XOffset, 0.f, YOffset)); 
}

FVector AMMPlayGrid::GridCoordsToLocalLocation(const FIntPoint& GridCoords)
{
	float BlockSpacingX = BlockSize.X + CellBackgroundMargin;
	float BlockSpacingY = BlockSize.Z + CellBackgroundMargin; // the grid's Y axis is actually local Z;
	float XOffset = (GridCoords.X * BlockSpacingX) + (BlockSpacingX / 2.f);
	float YOffset = (GridCoords.Y * BlockSpacingY) + (BlockSpacingY / 2.f);
	return FVector(XOffset, 0.f, YOffset); 
}

FVector AMMPlayGrid::GridFloatCoordsToWorldLocation(const FVector2D& GridCoords)
{
	if (FMath::Floor(GridCoords.X) == GridCoords.X && FMath::Floor(GridCoords.Y) == GridCoords.Y) {
		return GridCoordsToWorldLocation(FIntPoint((int32)GridCoords.X, (int32)GridCoords.Y));
	}
	else
	{
		FVector LocOne = GridCoordsToWorldLocation(FIntPoint(FMath::FloorToInt(GridCoords.X), FMath::FloorToInt(GridCoords.Y)));
		FVector LocTwo = GridCoordsToWorldLocation(FIntPoint(FMath::CeilToInt(GridCoords.X), FMath::CeilToInt(GridCoords.Y)));
		return (LocOne + LocTwo) / 2.f;
	}
}


void AMMPlayGrid::BlockClicked(AMMBlock* Block)
{
	if (GridState != EMMGridState::Normal) {
		return; 
	}
	if (SelectedBlock == nullptr) 
	{
		SelectedBlock = Block;
		SelectedBlock->OwningGridCell->Highlight(true);
	}
	else 
	{
		if (SelectedBlock == Block)
		{
			SelectedBlock->OwningGridCell->Highlight(false);
			SelectedBlock = nullptr;
			return;
		}
		if (UMMMath::CoordsAdjacent(Block->OwningGridCell->GetCoords(), SelectedBlock->OwningGridCell->GetCoords()))
		{
			AMMPlayGridCell* SelectedCell = SelectedBlock->OwningGridCell;
			if (MoveBlock(SelectedBlock, Block->OwningGridCell))
			{
				Block->OwningGridCell->Highlight(false);
				SelectedBlock->OwningGridCell->Highlight(false);
				SelectedBlock = nullptr;
			}
			else 
			{
				SelectedBlock = SelectedCell->CurrentBlock;
			}
		}
		else
		{
			SelectedBlock->OwningGridCell->Highlight(false);
			SelectedBlock = Block;
			SelectedBlock->OwningGridCell->Highlight(true);
		}
	}
}


bool AMMPlayGrid::MoveBlock(AMMBlock* MovingBlock, AMMPlayGridCell* ToCell)
{
	if (MovingBlock == nullptr || ToCell == nullptr || MovingBlock->OwningGridCell == nullptr) {
		return false;
	}
	//UE_LOG(LogMMGame, Log, TEXT("MoveBlock from %s to %s"), *(MovingBlock->OwningGridCell->GetCoords()).ToString(), *ToCell->GetCoords().ToString());
	TArray<FBlockMatch> CurrentMatches;
	AMMPlayGridCell* FromCell = MovingBlock->OwningGridCell;
	AMMBlock* SwappingBlock = ToCell->CurrentBlock;
	if (!MovingBlock->CanMove() || (SwappingBlock != nullptr && !SwappingBlock->CanMove())) {
		return false;
	}
	if (!UMMMath::CoordsAdjacent(FromCell->GetCoords(), ToCell->GetCoords())) {
		return false;
	}
	//if (SwappingBlock) {
	//	UE_LOG(LogMMGame, Log, TEXT("MoveBlock swapping blocks"));
	//}
	//else {
	//	UE_LOG(LogMMGame, Log, TEXT("MoveBlock moving block"));
	//}
	// Set grid state
	GridState = EMMGridState::Moving;
	// Swap the blocks
	ToCell->CurrentBlock = MovingBlock;
	MovingBlock->OwningGridCell = ToCell;
	FromCell->CurrentBlock = nullptr;
	if (SwappingBlock)
	{
		FromCell->CurrentBlock = SwappingBlock;
		SwappingBlock->OwningGridCell = FromCell;
	}
	// Check for matches on moved block
	FBlockMatch BlockMatchHoriz;
	FBlockMatch BlockMatchVert;
	CheckForMatches(MovingBlock, BlockMatchHoriz, BlockMatchVert);
	if (BlockMatchHoriz.Blocks.Num() > 0) {
		CurrentMatches.Add(BlockMatchHoriz);
	}
	if (BlockMatchVert.Blocks.Num() > 0) {
		CurrentMatches.Add(BlockMatchVert);
	}
	// Check for matches on swapped block
	if (SwappingBlock && !(SwappingBlock->bMatchedHorizontal && SwappingBlock->bMatchedVertical))
	{
		FBlockMatch SwapBlockMatchHoriz;
		FBlockMatch SwapBlockMatchVert;
		CheckForMatches(SwappingBlock, SwapBlockMatchHoriz, SwapBlockMatchVert);
		if (SwapBlockMatchHoriz.Blocks.Num() > 0) {
			CurrentMatches.Add(SwapBlockMatchHoriz);
		}
		if (SwapBlockMatchVert.Blocks.Num() > 0) {
			CurrentMatches.Add(SwapBlockMatchVert);
		}
	}
	if (CurrentMatches.Num() == 0)
	{
		// No matches, swap the blocks back
		PlaySoundQueue.AddUnique(MoveFailSound.Get());
		FromCell->CurrentBlock = MovingBlock;
		MovingBlock->OwningGridCell = FromCell;
		MovingBlock->OnMoveFail(ToCell);
		if (SwappingBlock)
		{
			ToCell->CurrentBlock = SwappingBlock;
			SwappingBlock->OwningGridCell = ToCell;
			SwappingBlock->OnMoveFail(FromCell);
		}
		return false;
	}
	else
	{
		MovingBlock->bMovedByPlayer = true;
		UnsettledBlocks.AddUnique(MovingBlock);
		MovingBlock->OnMove(ToCell);
		if (SwappingBlock) {
			UnsettledBlocks.AddUnique(SwappingBlock);
			SwappingBlock->OnMove(FromCell);
		}
		BlockMatches.Append(CurrentMatches);
		bAllMatchesFinished = false;
	}
	return true;
}


bool AMMPlayGrid::CheckFlaggedForMatches()
{
	GridState = EMMGridState::Matching;
	TArray<FBlockMatch> CurrentMatches;
	for (AMMBlock* Block : BlocksToCheck)
	{
		FBlockMatch BlockMatchHoriz;
		FBlockMatch BlockMatchVert;
		CheckForMatches(Block, BlockMatchHoriz, BlockMatchVert);
		if (BlockMatchHoriz.Blocks.Num() > 0) {
			CurrentMatches.Add(BlockMatchHoriz);
		}
		if (BlockMatchVert.Blocks.Num() > 0) {
			CurrentMatches.Add(BlockMatchVert);
		}
	}
	BlocksToCheck.Empty();
	if (CurrentMatches.Num() > 0) {
		BlockMatches.Append(CurrentMatches);
	}
	if (BlockMatches.Num() > 0) 
	{
		bAllMatchesFinished = false;
		ResolveMatches();
		return true;
	}
	else 
	{
		//GridState = EMMGridState::Normal;
		return false;
	}
}


bool AMMPlayGrid::CheckForMatches(AMMBlock* CheckBlock, FBlockMatch& HorizMatches, FBlockMatch& VertMatches, const bool bMarkBlocks)
{
	if (CheckBlock == nullptr) {
		return false;
	}
	if (!CheckBlock->bMatchedHorizontal) {
		MatchHorizontal(CheckBlock, HorizMatches, bMarkBlocks);
	}
	if (!CheckBlock->bMatchedVertical) {
		MatchVertical(CheckBlock, VertMatches, bMarkBlocks);
	}
	return HorizMatches.Blocks.Num() > 0 || VertMatches.Blocks.Num() > 0;
}


int32 AMMPlayGrid::MatchHorizontal(UPARAM(ref) AMMBlock* StartBlock, FBlockMatch& Match, const bool bMarkBlocks)
{
	Match.Blocks.Empty();
	MatchDirection(StartBlock, EMMDirection::West, Match, true);
	MatchDirection(StartBlock, EMMDirection::East, Match, true);
	if (Match.Blocks.Num() >= GetMinimumMatchSize())
	{
		if (bMarkBlocks) {
			for (AMMBlock* CurBlock : Match.Blocks) {
				CurBlock->bMatchedHorizontal = true;
			}
		}
		Match.Orientation = EMMOrientation::Horizontal;
		SortMatch(Match);
	}
	else
	{
		Match.Blocks.Empty();
	}
	return Match.Blocks.Num();
}


int32 AMMPlayGrid::MatchVertical(UPARAM(ref) AMMBlock* StartBlock, FBlockMatch& Match, const bool bMarkBlocks)
{
	Match.Blocks.Empty();
	MatchDirection(StartBlock, EMMDirection::South, Match, true);
	MatchDirection(StartBlock, EMMDirection::North, Match, true);
	if (Match.Blocks.Num() >= GetMinimumMatchSize())
	{
		if (bMarkBlocks) {
			for (AMMBlock* CurBlock : Match.Blocks)	{
				CurBlock->bMatchedVertical = true;
			}
		}
		Match.Orientation = EMMOrientation::Vertical;
		SortMatch(Match);
	}
	else
	{
		Match.Blocks.Empty();
	}
	return Match.Blocks.Num();
}


int32 AMMPlayGrid::MatchDirection(AMMBlock* StartBlock, const EMMDirection Direction, FBlockMatch& Matches, const bool bRecurse)
{
	if (StartBlock == nullptr) { return false; }
	if (StartBlock->OwningGridCell)
	{
		//UE_LOG(LogMMGame, Log, TEXT("MMPlayGrid::MatchDirection - Matches: %d direction coords %s"), Matches.Blocks.Num(), *UMMMath::DirectionToOffset(Direction).ToString());
		//UE_LOG(LogMMGame, Log, TEXT("MMPlayGrid::MatchDirection - Start coords: %s"), *(StartBlock->OwningGridCell->GetCoords()).ToString());
		FIntPoint NeighborCoords = StartBlock->OwningGridCell->GetCoords() + UMMMath::DirectionToOffset(Direction);
		//UE_LOG(LogMMGame, Log, TEXT("MMPlayGrid::MatchDirection - Neighbor coords: %s"), *NeighborCoords.ToString());
		AMMPlayGridCell* NeighborCell = GetCell(NeighborCoords);
		if (NeighborCell && NeighborCell->CurrentBlock)
		{
			//UE_LOG(LogMMGame, Log, TEXT("MMPlayGrid::MatchDirection - Neighbor cell coords: %s"), *(NeighborCell->GetCoords()).ToString());
			if (UMMMath::DirectionIsHorizontal(Direction)) 
			{
				Matches.Orientation = EMMOrientation::Horizontal;
				if (NeighborCell->CurrentBlock->bMatchedHorizontal) {
					return Matches.Blocks.Num();
				}
			}
			if (UMMMath::DirectionIsVertical(Direction))
			{
				Matches.Orientation = EMMOrientation::Vertical;
				if (NeighborCell->CurrentBlock->bMatchedVertical) {
					return Matches.Blocks.Num();
				}
			}
			if (StartBlock->Matches(NeighborCell->CurrentBlock)) // && !Matches.Contains(NeighborCell->CurrentBlock))
			{
				if (Matches.Blocks.Num() == 0) { Matches.Blocks.Add(StartBlock); }
				Matches.Blocks.Add(NeighborCell->CurrentBlock);
				if (bRecurse && Matches.Blocks.Num() <= 20) {
					MatchDirection(NeighborCell->CurrentBlock, Direction, Matches, bRecurse);
				}
			}
		}
	}
	return Matches.Blocks.Num();
}


void AMMPlayGrid::SortMatch(FBlockMatch& Match, bool bForceSort)
{
	if (Match.bSorted && !bForceSort) { return; }
	FIntPoint MinX = FIntPoint(-1, -1);
	FIntPoint MinY = FIntPoint(-1, -1);
	int32 MatchSize = Match.Blocks.Num();
	TArray<AMMBlock*> TmpBlocks;
	int32 StartIndex = -1;
	
	for (AMMBlock* Block : Match.Blocks)
	{
		FIntPoint Coords = Block->GetCoords();
		if (Match.Orientation == EMMOrientation::Unknown && MinX.X >= 0 && MinY.Y >= 0)
		{
			Match.Orientation = Coords.Y != MinY.Y ? EMMOrientation::Vertical : EMMOrientation::Horizontal;
		}
		if (MinX.X < 0 || Coords.X < MinX.X) {
			MinX = Coords;
		}
		if (MinY.Y < 0 || Coords.Y < MinY.Y) {
			MinY = Coords;
		}
	}
	Match.StartCoords = Match.Orientation == EMMOrientation::Horizontal ? MinX : MinY;
	Match.EndCoords = Match.Orientation == EMMOrientation::Horizontal ? Match.StartCoords + FIntPoint(MatchSize - 1, 0) : Match.StartCoords + FIntPoint(0, MatchSize - 1);
	TmpBlocks.Empty();
	TmpBlocks.SetNum(Match.Blocks.Num());
	for (int32 i = 0; i < Match.Blocks.Num(); i++)
	{
		if (Match.Orientation == EMMOrientation::Horizontal) {
			TmpBlocks[Match.Blocks[i]->GetCoords().X - MinX.X] = Match.Blocks[i];
		}
		else {
			TmpBlocks[Match.Blocks[i]->GetCoords().Y - MinY.Y] = Match.Blocks[i];
		}

	}
	Match.Blocks = TmpBlocks;
	Match.bSorted = true;
}


bool AMMPlayGrid::ResolveMatches()
{
	GridState = EMMGridState::Matching;
	AMMGameMode* GameMode = Cast<AMMGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GameMode) {
		return false;
	}
	int32 TotalScoreToAdd = 0;
	TArray<FGoodsQuantity> TotalGoods;
	for (int32 MatchIndex = 0; MatchIndex < BlockMatches.Num(); MatchIndex++)
	{
		FBlockMatch& CurMatch = BlockMatches[MatchIndex];
		FGoodsQuantitySet MatchGoods;
		GameMode->GetGoodsForMatch(CurMatch, MatchGoods);
		if (MatchGoods.Goods.Num() > 0) {
			TotalGoods.Append(MatchGoods.Goods);
		}
		TotalScoreToAdd += GameMode->GetScoreForMatch(CurMatch);
		for (AMMBlock* CurBlock : CurMatch.Blocks)
		{
			if (CurBlock->OwningGridCell && CurBlock->OwningGridCell->CurrentBlock == CurBlock) {
				CurBlock->OwningGridCell->CurrentBlock = nullptr;
			}
			CurBlock->OnMatched_Native(&CurMatch);
			PlaySoundQueue.AddUnique(CurBlock->MatchSound.Get());			
		}		
		//PerformActionsForMatch(CurMatch);
		//for (AMMBlock* CurBlock : CurMatch.Blocks) {
		//	CurBlock->OnMatched(CurMatch);
		//}
	}
	AddScore(TotalScoreToAdd);
	if (TotalGoods.Num() > 0)
	{
		AMMPlayerController* PC = Cast<AMMPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
		TArray<FGoodsQuantity> NewGoodsTotals;
		PC->GoodsInventory->AddSubtractGoodsArray(TotalGoods, false, NewGoodsTotals, true);
	}
	return true;
}


bool AMMPlayGrid::PerformActionsForMatch(FBlockMatch& Match)
{
	bool bAnyPerformed = false;
	bool bAnyPerformedForBlock = false;
	int32 MatchSize = Match.Blocks.Num();
	if (MatchSize == 0) {
		return false;
	}
	if (!Match.Blocks.IsValidIndex(0) || Match.Blocks[0]->Grid() == nullptr) {
		return false;
	}
	TArray<FName> ProcessedBlockTypeNames;
	int32 BonusMatchSize = MatchSize - Match.Blocks[0]->Grid()->GetMinimumMatchSize();
	for (AMMBlock* Block : Match.Blocks)
	{
		bAnyPerformedForBlock = false;
		// Process each block type actions only once per block type
		if (!ProcessedBlockTypeNames.Contains(Block->GetBlockType().Name))
		{
			for (FMatchActionTrigger Trigger : Block->GetBlockType().MatchActions)
			{
				if (BonusMatchSize >= Trigger.MinBonusSizeToTrigger && BonusMatchSize <= Trigger.MaxBonusSizeToTrigger) 
				{
					if (PerformActionType(Trigger.ActionType, Match)) {
						bAnyPerformedForBlock = bAnyPerformed = true;
					}
				}
				//if (Trigger.ActionType.ActionQuantityType == EMMBlockQuantity::PerMatch)
				//{
				//	// PerMatch do it only once for this match
				//	if (!ProcessedBlockTypeNames.Contains(Block->GetBlockType().Name))
				//	{
				//		ProcessedBlockTypeNames.Add(Block->GetBlockType().Name);
				//		if (PerformActionType(Trigger.ActionType, Match)) {
				//			bAnyPerformedForBlock = bAnyPerformed = true;
				//		}
				//	}
				//}
				//else if (Trigger.ActionType.ActionQuantityType == EMMBlockQuantity::PerBlock)
				//{
				//	// PerBlock do it for each block
				//	if (PerformActionType(Trigger.ActionType, Match)) {
				//		bAnyPerformedForBlock = bAnyPerformed = true;
				//	}
				//}
				//else if (Trigger.ActionType.ActionQuantityType == EMMBlockQuantity::PerBlockOverMin)
				//{
				//	// PerBlockOverMin do it once for each block over min match size. i.e. BonusMatchSize
				//	if (!ProcessedBlockTypeNames.Contains(Block->GetBlockType().Name))
				//	{
				//		ProcessedBlockTypeNames.Add(Block->GetBlockType().Name);
				//		// Call it once for each BonusMatchSize
				//		for (int32 i = 1; i <= BonusMatchSize; i++)
				//		{
				//			if (PerformActionType(Trigger.ActionType, Match)) {
				//				bAnyPerformedForBlock = bAnyPerformed = true;
				//			}
				//		}
				//	}
				//}
			}
			if (bAnyPerformedForBlock) {
				ProcessedBlockTypeNames.Add(Block->GetBlockType().Name);
			}
		}		
	}
	return bAnyPerformed;
}


bool AMMPlayGrid::PerformActionType(const FMatchActionType& MatchActionType, const FBlockMatch& Match)
{
	bool bSuccess = false;
	UMatchAction* MatchAction = NewObject<UMatchAction>(this, MatchActionType.MatchActionClass);
	if (MatchAction) {
		bSuccess = MatchAction->Perform(Match, MatchActionType);
	}
	MatchAction = nullptr;
	return bSuccess;
}


void AMMPlayGrid::SettleBlocks()
{
	GridState = EMMGridState::Settling;
	// Unsettle all blocks that were queued for unsettling
	for (AMMBlock* CurBlock : UnsettledBlocks)
	{
		CurBlock->OnUnsettle();
	}
}


void AMMPlayGrid::UnsettleBlock(AMMBlock* Block)
{
	if (Block == nullptr) {
		return;
	}
	// If it is a movable unmatched block, grab it to be unsettled.
	if (Block->CanMove() && !Block->IsMatched()) {
		UnsettledBlocks.AddUnique(Block);
		if (GridState == EMMGridState::Settling) {
			Block->OnUnsettle();
		}
	}
}

void AMMPlayGrid::UnsettleBlockAboveCell(AMMPlayGridCell* Cell)
{
	if (Cell == nullptr) {
		UE_LOG(LogMMGame, Error, TEXT("MMPlayGrid::UnsettleBlockAboveCell - Called with nullptr cell"));
		return;
	}
	// Get the cell and block above it
	AMMPlayGridCell* AboveCell = GetCell(Cell->GetCoords() + UMMMath::DirectionToOffset(EMMDirection::North));
	AMMBlock* AboveBlock = (AboveCell == nullptr) ? nullptr : AboveCell->CurrentBlock;
	if (AboveBlock != nullptr) {
		UnsettleBlock(AboveBlock);		
	}
	else if (!AboveCell && (Cell->GetCoords().Y == (SizeY - 1)))
	{
		// Unsettling cell above top row then drop new block into our soon-to-be-empty top cell
		AddRandomBlockInCell(Cell, BlockSize.Z + BlockMargin);
		//UnsettleBlock(Cell->CurrentBlock);
		//Cell->CurrentBlock->OnUnsettle();
	}
}


void AMMPlayGrid::AddScore(int32 PointsToAdd)
{
	// Increment score
	Score += PointsToAdd;

	// Update text
	ScoreText->SetText(FText::Format(LOCTEXT("ScoreFmt", "Score: {0}"), FText::AsNumber(Score)));
}



void AMMPlayGrid::BlockFinishedMoving(AMMBlock* Block, bool bBlockMoved)
{
	if (Block == nullptr) { return; }
	//if (BlockMatches.Num() > 0) 
	//{
	//	if (UnsettledBlocks.Num() == 0) { 
	//		ResolveMatches();
	//	}
	//}
	//else 
	//{
		if (bBlockMoved) 
		{
			BlocksToCheck.AddUnique(Block);
			if (Block->StopMoveSound) {
				PlaySoundQueue.AddUnique(Block->StopMoveSound.Get());
			}
		}
		//if (UnsettledBlocks.Num() == 0) {
		//	CheckFlaggedForMatches();
		//}
	//}
	UnsettledBlocks.RemoveSingle(Block);
}

void AMMPlayGrid::BlockFinishedMatch(AMMBlock* Block, const FBlockMatch& Match)
{
	int32 BlockIndex;
	bool bWholeMatchFinished;
	bool bTmpAllMatchesFinished = true;;
	//UnsettledBlocks.RemoveSingleSwap(Block);
	for (int32 i = 0; i < BlockMatches.Num(); i++)
	{ 
		bWholeMatchFinished = true;
		if (BlockMatches[i].Blocks.Find(Block, BlockIndex)) 
		{
			if (BlockMatches[i].Blocks[BlockIndex]->OwningGridCell && BlockMatches[i].Blocks[BlockIndex]->OwningGridCell->CurrentBlock == BlockMatches[i].Blocks[BlockIndex]) {
				BlockMatches[i].Blocks[BlockIndex]->OwningGridCell->CurrentBlock = nullptr;
			}
			for (AMMBlock* CurBlock : BlockMatches[i].Blocks)
			{
				if (!CurBlock->bMatchFinished) 
				{
					bWholeMatchFinished = false;
					break;
				}
			}
			if (bWholeMatchFinished) 
			{
				PerformActionsForMatch(BlockMatches[i]);
				BlockMatches[i].bMatchFinished = true;
			}			
		}
	}
	// See if all matches have finished
	for (int32 i = 0; i < BlockMatches.Num(); i++)
	{
		if (!BlockMatches[i].bMatchFinished)
		{
			bTmpAllMatchesFinished = false;
			break;
		}
	}
	if (bTmpAllMatchesFinished)
	{
		bAllMatchesFinished = true;
		// AllMatchesFinished();
	}
}


void AMMPlayGrid::AllMatchesFinished()
{
	// When all matches finished, destroy blocks in all the matches.
	// Also unsettle any blocks above each destroyed block if the cell is still empty.
	for (int32 i = 0; i < BlockMatches.Num(); i++)
	{
		int32 BlockMatchSize = BlockMatches[i].Blocks.Num();
		for (int32 BIndex = 0; BIndex < BlockMatchSize; BIndex++)
		{
			AMMBlock* CurBlock = BlockMatches[i].Blocks[BIndex];
			AMMPlayGridCell* UnsettleAboveCell = nullptr;
			if (CurBlock->OwningGridCell != nullptr && (CurBlock->OwningGridCell->CurrentBlock == nullptr || CurBlock->OwningGridCell->CurrentBlock == CurBlock)) {
				UnsettleAboveCell = CurBlock->OwningGridCell;
			}
			Blocks.RemoveSingle(CurBlock);
			CurBlock->DestroyBlock();
			if (UnsettleAboveCell) {
				UnsettleBlockAboveCell(UnsettleAboveCell);
			}
		}
		BlockMatches[i].Blocks.Empty();
	}
	BlockMatches.Empty();
	bAllMatchesFinished = false;
	//if (UnsettledBlocks.Num() > 0) {
	//	SettleBlocks();
	//}
	//else {
	//	CheckFlaggedForMatches();
	//}
}


void AMMPlayGrid::PlaySounds(const TArray<USoundBase*> Sounds)
{
	for (USoundBase* Sound : Sounds) {
		UGameplayStatics::PlaySound2D(GetWorld(), Sound, 1.f, 1.f, 0.f);
	}
}

#undef LOCTEXT_NAMESPACE
