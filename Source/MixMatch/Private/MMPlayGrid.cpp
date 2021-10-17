// Copyright Epic Games, Inc. All Rights Reserved.

#include "MMPlayGrid.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Components/TextRenderComponent.h"
#include "Components/BillboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "..\MixMatch.h"
#include "MMMath.h"
#include "MMPlayGridCell.h"
#include "MMBlock.h"
#include "MMGameMode.h"
#include "InventoryActorComponent.h"
#include "MMPlayerController.h"
#include "Goods/GoodsQuantity.h"
#include "Goods/GoodsFunctionLibrary.h"


#define LOCTEXT_NAMESPACE "MMPlayGrid"

AMMPlayGrid::AMMPlayGrid()
{
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UStaticMesh> ToggleBlocksMesh;
		FConstructorStatics()
			: ToggleBlocksMesh(TEXT("/Game/Puzzle/Meshes/PuzzleCube.PuzzleCube"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	// Create root scene component
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// Create score text component
	ScoreText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ScoreText"));
	ScoreText->SetRelativeLocation(FVector(0.f,0.f,-30.f));
	ScoreText->SetRelativeRotation(FRotator(0.f,90.f,0.f));
	ScoreText->SetText(FText::Format(LOCTEXT("ScoreFmt", "Score: {0}"), FText::AsNumber(0)));
	ScoreText->SetupAttachment(SceneRoot);

	BlockToggleControlMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ToggleBlocksControlMesh"));
	BlockToggleControlMesh->SetStaticMesh(ConstructorStatics.ToggleBlocksMesh.Get());
	BlockToggleControlMesh->SetRelativeScale3D(FVector(1.f, 1.f, 1.f));
	BlockToggleControlMesh->SetRelativeLocation(FVector(-60.f, 0.f, -30.f));
	BlockToggleControlMesh->SetupAttachment(SceneRoot);
	BlockToggleControlMesh->OnClicked.AddDynamic(this, &AMMPlayGrid::ToggleBlocksClicked);
	BlockToggleControlMesh->OnInputTouchBegin.AddDynamic(this, &AMMPlayGrid::OnFingerPressedToggleBlocks);

#if WITH_EDITORONLY_DATA
	// Billboard
	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	Billboard->SetupAttachment(SceneRoot);
#endif

	// Inventory
	GoodsInventory = CreateDefaultSubobject<UInventoryActorComponent>(TEXT("GoodsInventory"));
	if (GoodsInventory) {
		AddOwnedComponent(GoodsInventory);
	}

	// Set defaults
	CellClass = AMMPlayGridCell::StaticClass();
	GridState = EMMGridState::Normal;
	SizeX = 5;
	SizeY = 7;
	BlockTypeSetName = FName("Default");
	bScaleBlocks = false;
	BlockSize = FVector(50.f, 50.f, 50.f);
	BlockMargin = 4.f;
	CellBackgroundMargin = 0.f;
	CellBackgroundOffset = (BlockSize.Y / 2.f) + 1.f;// 30.f;
	NewBlockDropInHeight = 25.f;
	PlayerMovesCount = 0;
	bPauseNewBlocks = false;
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
				UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("All Blocks Finished Moving"));
				GridLockedState = EMMGridLockState::Unchecked;
				GridState = EMMGridState::Normal;
			}
		}		
		break;
	case EMMGridState::Matching:
		//MatchTick(DeltaSeconds);
		if (BlockMatches.Num() == 0)
		{
			if (UnsettledBlocks.Num() > 0 || ToBeUnsettledBlocks.Num() > 0) {
				SettleBlocks();
			}
			else if(BlocksToCheck.Num() > 0) {
				CheckFlaggedForMatches();
			}
			else {
				UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("All Blocks Finished Matching"));
				GridLockedState = EMMGridLockState::Unchecked;
				GridState = EMMGridState::Normal;
			}
		}
		else if (bAllMatchesFinished) {
			AllMatchesFinished();
		}
		break;
	case EMMGridState::Settling:
		SettleTick(DeltaSeconds);
		break;
	case EMMGridState::Normal:
		if (GridLockedState == EMMGridLockState::Unchecked || GridLockedState == EMMGridLockState::Checking) 
		{
			if (CheckGridIsLocked() == EMMGridLockState::Locked)
			{
				// Call delegate
				OnGridLocked.Broadcast(this);
			}
		}
		break;
	default:
		break;
	}
}


void AMMPlayGrid::SettleTick(float DeltaSeconds)
{
	// Unsettle all waiting to be unsettled
	for (AMMBlock* CurBlock : ToBeUnsettledBlocks) 
	{
		if (!CurBlock->bUnsettled) {
			UnsettleBlock(CurBlock);
		}
	}
	ToBeUnsettledBlocks.Empty();
	// Iterate blocks in order to settle them. Iterating all blocks in order so settling begins from bottom->up.
	for (int32 i = 0; i < Cells.Num(); i++)
	{
		AMMPlayGridCell* CurCell = Cells[i];
		AMMBlock* Block = CurCell->CurrentBlock;
		if (Block && Block->BlockState == EMMGridState::Settling) {
			Block->SettleTick(DeltaSeconds);
		}
	}
	if (UnsettledBlocks.Num() == 0)
	{
		if (BlocksToCheck.Num() > 0) {
			CheckFlaggedForMatches();
		}
		else {
			// TODO: Remove this debug
			UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("All Blocks Finished Settling"));
			GridLockedState = EMMGridLockState::Unchecked;
			GridState = EMMGridState::Normal;
		}
	}
}


void AMMPlayGrid::StartPlayGrid_Implementation()
{
	PlayerMovesCount = 0;
	FillGridBlocks();
}


void AMMPlayGrid::BeginPlay()
{
	Super::BeginPlay();
	
	//SpawnGrid();
	//FillGridBlocks();
}


int32 AMMPlayGrid::GetMinimumMatchSize()
{
	return MinimumMatchSize;
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
	InitBlocksFallingIntoGrid();

	// Number of blocks
	const int32 NumCells = SizeX * SizeY;
	Cells.Empty();
	Cells.Reserve(NumCells);
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;

	// Loop to spawn each cell
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


void AMMPlayGrid::FillGridBlocks()
{
	if (Cells.Num() == 0) {
		SpawnGrid();
	}
	else {
		DestroyBlocks();
	}
	UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("MMPlayGrid::FillGridBlocks - Filling %d grid blocks [%d x %d]"), Cells.Num(), SizeX, SizeY);
	AMMPlayGridCell* Cell = nullptr;
	FAddBlockContext BlockContext;
	BlockContext.bPreventMatches = true;
	BlockContext.bForInitialFill = true;
	for (int32 i = 0; i < Cells.Num(); i++)	
	{
		BlockContext.AddToCell = Cells[i];
		AddRandomBlockInCell(BlockContext);
	}
	GridLockedState = EMMGridLockState::Unchecked;
	UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("MMPlayGrid::FillGridBlocks - Added %d blocks to grid"), Blocks.Num());
}


void AMMPlayGrid::DestroyGrid()
{
	DestroyBlocks();
	for (AMMPlayGridCell* Cell : Cells)	{
		Cell->DestroyCell();
	}	
	Cells.Empty();
}


void AMMPlayGrid::DestroyBlocks()
{
	for (AMMPlayGridCell* Cell : Cells)
	{
		if (IsValid(Cell) && IsValid(Cell->CurrentBlock)) {
			Cell->CurrentBlock->DestroyBlock();
		}
	}
	UnsettledBlocks.Empty();
	BlocksToCheck.Empty();
	BlockMatches.Empty();
	for (int32 i = 0; i < BlocksFallingIntoGrid.Num(); i++) {
		BlocksFallingIntoGrid[i].Blocks.Empty();
	}
	Blocks.Empty();	
}


bool AMMPlayGrid::GetRandomBlockTypeNameForCell_Implementation(FName& FoundBlockTypeName, const FAddBlockContext& BlockContext)
{
	AMMGameMode* GameMode = Cast<AMMGameMode>(UGameplayStatics::GetGameMode(this));
	AMMPlayGridCell* Cell = BlockContext.AddToCell;
	if (Cell == nullptr) 
	{
		UE_LOG(LogMMGame, Error, TEXT("MMPlayGrid::GetRandomBlockTypeNameForCell - Called with null cell."));
		return false;
	}
	if (GameMode == nullptr) 
	{
		UE_LOG(LogMMGame, Error, TEXT("MMPlayGrid::GetRandomBlockTypeNameForCell - Cannot get game mode"));
		return false;
	}
	if (FMath::FRandRange(0.f, 1.f) < DuplicateSpawnPreventionFactor) {
		return GameMode->GetRandomBlockTypeNameForCell(FoundBlockTypeName, BlockContext);
	}
	else {
		FAddBlockContext TmpContext = BlockContext;
		TmpContext.ExcludedBlockNames.Empty();
		return GameMode->GetRandomBlockTypeNameForCell(FoundBlockTypeName, TmpContext);
	}

}


AMMBlock* AMMPlayGrid::AddBlockInCell(const FName& BlockTypeName, const FAddBlockContext& BlockContext)
{
	AMMGameMode* GameMode = Cast<AMMGameMode>(UGameplayStatics::GetGameMode(this));
	if (BlockContext.AddToCell == nullptr) 
	{
		UE_LOG(LogMMGame, Error, TEXT("MMPlayGrid::AddBlockInCell - Called with null cell."));
		return nullptr;
	}
	if (GameMode == nullptr) 
	{
		UE_LOG(LogMMGame, Error, TEXT("MMPlayGrid::AddBlockInCell - Cannot get game mode"));
		return nullptr;
	}
	AMMPlayGridCell* Cell = BlockContext.AddToCell;
	if (BlockContext.OffsetAboveTopCell == 0.f) {
		UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("MMPlayGrid::AddBlockInCell - Adding block at %s."), *Cell->GetCoords().ToString());
	}
	else {
		UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("MMPlayGrid::AddBlockInCell - Dropping block %0.f units above column %d."), BlockContext.OffsetAboveTopCell, Cell->GetCoords().X);
	}
	FVector BlockLocation = Cell->GetBlockLocalLocation() + FVector(0.f, 0.f, BlockContext.OffsetAboveTopCell);
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
			NewBlock->ChangeOwningGridCell(Cell);
			Blocks.Add(NewBlock);
			NewBlock->SetBlockType(BlockType);
			NewBlock->bFallingIntoGrid = BlockContext.OffsetAboveTopCell > 0.f;
			if (bScaleBlocks)
			{
				FVector BlockOrigin;
				FVector BlockBoxExtent;
				float BlockSphereRadius;
				UKismetSystemLibrary::GetComponentBounds(NewBlock->GetBlockMesh(), BlockOrigin, BlockBoxExtent, BlockSphereRadius);
				if (BlockBoxExtent.X > 0.f) {
					Rescale.X = (BlockSize.X - (BlockMargin * 2)) / (BlockBoxExtent.X * 2);
				}
				if (BlockBoxExtent.Z > 0.f) {
					Rescale.Z = (BlockSize.Z - (BlockMargin * 2)) / (BlockBoxExtent.Z * 2);
				}
				// Scale by smallest X or Z, ignore Y
				Rescale = Rescale.X < Rescale.Z ? Rescale.X * FVector::OneVector : Rescale.Z * FVector::OneVector;
				NewBlock->GetBlockMesh()->SetWorldScale3D(NewBlock->GetBlockMesh()->GetRelativeScale3D() * Rescale);
			}
			NewBlock->AttachToActor(this, FAttachmentTransformRules::SnapToTargetIncludingScale);
			NewBlock->SetActorRelativeLocation(BlockLocation, false, nullptr, ETeleportType::ResetPhysics);
			// Don't unsettle if this is intial grid fill
			if (!BlockContext.bForInitialFill && !BlockContext.bPreventMatches)
			{
				AMMPlayGridCell* TopCell = GetTopCell(Cell->X);
				if (!NewBlock->bFallingIntoGrid || (!IsValid(TopCell->CurrentBlock) || TopCell->CurrentBlock == NewBlock)) 
				{
					ToBeUnsettledBlocks.AddUnique(NewBlock);
					if (!NewBlock->bFallingIntoGrid) {
						BlocksToCheck.AddUnique(NewBlock);
					}
				}
			}
			UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("    SPAWNED block %s for cell %s"), *NewBlock->GetName(), *NewBlock->GetCoords().ToString());
			return NewBlock;
		}
		else {
			UE_LOG(LogMMGame, Error, TEXT("MMPlayGrid::AddBlockInCell - Error spawning new block at coords %s"), *Cell->GetCoords().ToString());
			return nullptr;
		}
	}
	else {
		UE_LOG(LogMMGame, Error, TEXT("MMPlayGrid::AddBlockInCell - Could not get block type for name %s"), *BlockTypeName.ToString());
	}
	return nullptr;
}


AMMBlock* AMMPlayGrid::AddRandomBlockInCell(const FAddBlockContext& BlockContext)
{
	if (bPauseNewBlocks) {
		return nullptr;
	}
	AMMPlayGridCell* Cell = BlockContext.AddToCell;
	check(Cell);
	AMMBlock* NewBlock = nullptr;
	FName BlockTypeName;
	bool bTryAgain = true;
	const int32 MaxTries = 50;
	int32 Tries = 0;
	while (bTryAgain && Tries < MaxTries)
	{
		Tries++;
		bTryAgain = false;
		if (GetRandomBlockTypeNameForCell(BlockTypeName, BlockContext))
		{
			// AddBlockInCell should not do unsettling if we are preventing matches or this is initial fill. If we are preventing matches, we will unsettle the cell
			// ourselves if the block was added successfully without matching.
			NewBlock = AddBlockInCell(BlockTypeName, BlockContext);
			if (IsValid(NewBlock))
			{
				if (BlockContext.bPreventMatches && !NewBlock->bFallingIntoGrid)
				{
					UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("Checking new block for matches %s"), *NewBlock->GetCoords().ToString());
					UBlockMatch* BlockMatchHoriz = nullptr;
					UBlockMatch* BlockMatchVert = nullptr;
					if (CheckForMatches(NewBlock, &BlockMatchHoriz, &BlockMatchVert, false))
					{
						UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("   New block matches"));
						Blocks.RemoveSingle(NewBlock);
						BlocksToCheck.RemoveSingle(NewBlock);
						ToBeUnsettledBlocks.RemoveSingle(NewBlock);
						UnsettledBlocks.RemoveSingle(NewBlock);
						NewBlock->DestroyBlock();
						bTryAgain = true;
						NewBlock = nullptr;
						if (Tries == MaxTries) {
							UE_CLOG(bDebugLog, LogMMGame, Warning, TEXT("MMPlayGrid::AddRandomBlockInCell - Exceeded max tries to find an unmatching block type in cell %s"), *Cell->GetCoords().ToString());
						}
					}
					else 
					{
						// Block was added with no match. Then queue block to be unsettled if unsettling is allowed.
						if (!BlockContext.bForInitialFill)
						{
							AMMPlayGridCell* TopCell = GetTopCell(Cell->X);
							if (!NewBlock->bFallingIntoGrid || (!IsValid(TopCell->CurrentBlock) || TopCell->CurrentBlock == NewBlock)) {
								ToBeUnsettledBlocks.AddUnique(NewBlock);
								if (!NewBlock->bFallingIntoGrid) {
									BlocksToCheck.AddUnique(NewBlock);
								}
							}
						}
					}
				}
			}
			else
			{
				NewBlock = nullptr;
			}
		}
	}
	return NewBlock;
}


AMMBlock* AMMPlayGrid::DropRandomBlockInColumn(AMMPlayGridCell* Cell)
{
	return DropRandomBlockInColumnEx(Cell, TArray<FName>());
}


AMMBlock* AMMPlayGrid::DropRandomBlockInColumnEx(UPARAM(ref) AMMPlayGridCell* Cell, const TArray<FName>& ExcludedBlockNames)
{
	if (bPauseNewBlocks) {
		return nullptr;
	}
	check(Cell);
	int32 Column = Cell->X;
	AMMPlayGridCell* TopCell = GetTopCell(Column);
	check(TopCell);
	FAddBlockContext BlockContext;
	BlockContext.AddToCell = TopCell;
	BlockContext.OffsetAboveTopCell = (
		(GetActorTransform().GetScale3D().Z * NewBlockDropInHeight) +
		((BlockSize.Z + CellBackgroundMargin) * (BlocksFallingIntoGrid[Column].Blocks.Num() + 1))
	);
	if (ExcludedBlockNames.Num() > 0) {
		BlockContext.ExcludedBlockNames = ExcludedBlockNames;
	}
	AMMBlock* NewBlock = AddRandomBlockInCell(BlockContext);
	if (IsValid(NewBlock)) {
		BlocksFallingIntoGrid[Column].Blocks.AddUnique(NewBlock);
	}
	return NewBlock;
}


void AMMPlayGrid::CellBecameOpen(AMMPlayGridCell* Cell)
{
	check(Cell);
	if (IsValid(Cell->CurrentBlock)) { return; }
	AMMBlock* FoundBlock = nullptr;
	bool bDoUnsettle = false;
	FIntPoint Coords = Cell->GetCoords();
	if (Coords.Y == SizeY - 1)
	{
		for (int32 i = 0; i < BlocksFallingIntoGrid[Coords.X].Blocks.Num(); i++)
		{
			AMMBlock* FallingBlock = BlocksFallingIntoGrid[Coords.X].Blocks[i];
			if (FallingBlock->SettleToGridCell == Cell)
			{
				FoundBlock = FallingBlock;
				bDoUnsettle = true;
				break;
			}
		}
	}	
	if (FoundBlock == nullptr)
	{
		for (AMMBlock* Block : UnsettledBlocks)
		{
			if (Block->SettleToGridCell == Cell)
			{	
				FoundBlock = Block;
				break;
			}
		}
	}
	if (FoundBlock == nullptr)
	{
		for (AMMBlock* Block : ToBeUnsettledBlocks)
		{
			if (Block->SettleToGridCell == Cell)
			{
				FoundBlock = Block;
				bDoUnsettle = true;
				break;
			}
		}
	}
	if (FoundBlock) {
		UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("AMMPlayGrid::CellBecameOpen - Found new block %s for cell at %s"), *FoundBlock->GetName(), *Cell->GetCoords().ToString());
		if (!FoundBlock->ChangeOwningGridCell(Cell)) {
			UE_LOG(LogMMGame, Error, TEXT("AMMPlayGrid::CellBecameOpen - Tried to set change owner of block %s to cell %s but could not."), *FoundBlock->GetName(), *Cell->GetCoords().ToString());
		}
	}
	if (FoundBlock == nullptr || (FoundBlock && bDoUnsettle)) {
		UnsettleBlockAboveCell(Cell);
	}
}


AMMPlayGridCell* AMMPlayGrid::GetCell(const FIntPoint& Coords)
{
	if (Coords.X < 0 || Coords.X >= SizeX) { return nullptr; }
	if (Coords.Y < 0 || Coords.Y >= SizeY) { return nullptr; }
	int32 Index = (Coords.Y * SizeX) + Coords.X;
	if (Index >= Cells.Num()) { return nullptr; }
	AMMPlayGridCell* Cell = Cells[Index];
	if (Cell->GetCoords() == Coords) {
		return Cell;
	}
	else {
		UE_LOG(LogMMGame, Error, TEXT("MMPlayGrid::GetCell - Found cell at index %d with incorrect coords %s"), Index, *Cell->GetCoords().ToString());
	}
	return nullptr;
}


TArray<AMMPlayGridCell*> AMMPlayGrid::GetCellNeighbors(const AMMPlayGridCell* Cell, const bool bIncludeDiagonal)
{
	TArray<AMMPlayGridCell*> Neighbors;
	if (Cell == nullptr) {
		return Neighbors;
	}
	FIntPoint StartCoords = Cell->GetCoords();
	AMMPlayGridCell* Neighbor = nullptr;
	TArray<EMMDirection> Directions;
	if (bIncludeDiagonal) {
		Directions = UMMMath::AllDirections;
	}
	else {
		Directions = UMMMath::OrthogonalDirections;
	}
	for (EMMDirection Direction : Directions)
	{
		Neighbor = GetCell(StartCoords + UMMMath::DirectionToOffset(Direction));
		if (IsValid(Neighbor)) {
			Neighbors.Add(Neighbor);
		}
	}
	return Neighbors;
}


AMMPlayGridCell* AMMPlayGrid::GetTopCell(const int32 Column)
{
	if (Column > SizeX - 1) { return nullptr; }
	return GetCell(FIntPoint(Column, SizeY - 1));
}


AMMBlock* AMMPlayGrid::GetBlock(const FIntPoint& Coords)
{
	AMMPlayGridCell* Cell = GetCell(Coords);
	if (Cell != nullptr) {
		return Cell->CurrentBlock;
	}
	return nullptr;
}


FVector AMMPlayGrid::GridCoordsToWorldLocation(const FIntPoint& GridCoords)
{
	return GetActorTransform().TransformPosition(GridCoordsToLocalLocation(GridCoords)); 
}

FVector AMMPlayGrid::GridCoordsToLocalLocation(const FIntPoint& GridCoords)
{
	float BlockSpacingX = BlockSize.X + CellBackgroundMargin;
	float BlockSpacingY = BlockSize.Z + CellBackgroundMargin; // the grid's Y axis is actually world Z;
	float XOffset = ((GridCoords.X * BlockSpacingX) + (BlockSpacingX / 2.f)) - (((BlockSpacingX * SizeX) + CellBackgroundMargin) / 2.f);
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


FBoxSphereBounds AMMPlayGrid::GetVisibleGridBounds()
{
	FBoxSphereBounds VisBounds;
	VisBounds.BoxExtent.X = (((float)BlockSize.X + CellBackgroundMargin) * (float)SizeX) / 2.f;
	VisBounds.BoxExtent.Z = (((float)BlockSize.Z + CellBackgroundMargin) * (float)SizeY) / 2.f;
	VisBounds.BoxExtent.Y = (CellBackgroundOffset + BlockSize.Y) / 2.f;
	VisBounds.SphereRadius = VisBounds.BoxExtent.Size();
	VisBounds.Origin = FVector(0.f, -VisBounds.BoxExtent.Y, VisBounds.BoxExtent.Z);
	return VisBounds.TransformBy(GetActorTransform());
}


void AMMPlayGrid::BlockClicked(AMMBlock* Block)
{
	if (GridState != EMMGridState::Normal || (Block && Block->BlockState != EMMGridState::Normal)) {
		return; 
	}
	if (SelectedBlock == nullptr) 
	{
		SelectedBlock = Block;
		SelectedBlock->Cell()->Highlight(true);
	}
	else 
	{
		if (SelectedBlock == Block)
		{
			SelectedBlock->Cell()->Highlight(false);
			SelectedBlock = nullptr;
			return;
		}
		if (UMMMath::CoordsAdjacent(Block->Cell()->GetCoords(), SelectedBlock->Cell()->GetCoords()))
		{
			AMMPlayGridCell* SelectedCell = SelectedBlock->Cell();
			if (MoveBlock(SelectedBlock, Block->Cell()))
			{
				//PlayerMovesCount++;
				Block->Cell()->Highlight(false);
				SelectedBlock->Cell()->Highlight(false);
				SelectedBlock = nullptr;
			}
			else {
				SelectedBlock = SelectedCell->CurrentBlock;
			}
		}
		else
		{
			SelectedBlock->Cell()->Highlight(false);
			SelectedBlock = Block;
			SelectedBlock->Cell()->Highlight(true);
		}
	}
}


void AMMPlayGrid::CellClicked(AMMPlayGridCell* Cell)
{
	check(Cell);
	if (GridState != EMMGridState::Normal) {
		return;
	}
	if (IsValid(Cell->CurrentBlock)) {
		BlockClicked(Cell->CurrentBlock);
		return;
	}
	if (IsValid(SelectedBlock))
	{
		if (UMMMath::CoordsAdjacent(Cell->GetCoords(), SelectedBlock->GetCoords()))
		{
			AMMPlayGridCell* SelectedCell = SelectedBlock->Cell();
			if (MoveBlock(SelectedBlock, Cell))
			{
				//PlayerMovesCount++;
				Cell->Highlight(false);
				SelectedCell->Highlight(false);
				SelectedBlock = nullptr;
			}
		}
	}
}


void AMMPlayGrid::ToggleBlocksClicked(UPrimitiveComponent* ClickedComp, FKey ButtonClicked)
{
	bPauseNewBlocks = !bPauseNewBlocks;
}


void AMMPlayGrid::OnFingerPressedToggleBlocks(ETouchIndex::Type FingerIndex, UPrimitiveComponent* TouchedComponent)
{
	bPauseNewBlocks = !bPauseNewBlocks;
}


bool AMMPlayGrid::MoveBlock(AMMBlock* MovingBlock, AMMPlayGridCell* ToCell)
{
	if (MovingBlock == nullptr || ToCell == nullptr || MovingBlock->OwningGridCell == nullptr) {
		return false;
	}
	if (MaxPlayerMovesCount > 0 && PlayerMovesCount >= MaxPlayerMovesCount) {
		return false;
	}
	UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("MoveBlock %s from %s to %s"), *MovingBlock->GetName(), *MovingBlock->Cell()->GetCoords().ToString(), *ToCell->GetCoords().ToString());
	TArray<UBlockMatch*> CurrentMatches;
	AMMPlayGridCell* FromCell = MovingBlock->Cell();
	AMMBlock* SwappingBlock = ToCell->CurrentBlock;
	if (SwappingBlock && SwappingBlock->OwningGridCell == nullptr) {
		return false;
	}
	if (!MovingBlock->CanMove() || (SwappingBlock != nullptr && !SwappingBlock->CanMove())) {
		return false;
	}
	if (!UMMMath::CoordsAdjacent(FromCell->GetCoords(), ToCell->GetCoords())) {
		return false;
	}
	if (SwappingBlock) {
		UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("  MoveBlock swapping blocks with %s"), *SwappingBlock->GetName());
	}
	else {
		UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("  MoveBlock moving block"));
	}
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
	UBlockMatch* BlockMatchHoriz = nullptr;
	UBlockMatch* BlockMatchVert = nullptr;
	CheckForMatches(MovingBlock, &BlockMatchHoriz, &BlockMatchVert);
	if (BlockMatchHoriz && BlockMatchHoriz->Blocks.Num() > 0) {
		CurrentMatches.Add(BlockMatchHoriz);
	}
	if (BlockMatchVert && BlockMatchVert->Blocks.Num() > 0) {
		CurrentMatches.Add(BlockMatchVert);
	}
	// Check for matches on swapped block
	if (IsValid(SwappingBlock) && !(SwappingBlock->bMatchedHorizontal && SwappingBlock->bMatchedVertical))
	{
		UBlockMatch* SwapBlockMatchHoriz = nullptr;
		UBlockMatch* SwapBlockMatchVert = nullptr;
		CheckForMatches(SwappingBlock, &SwapBlockMatchHoriz, &SwapBlockMatchVert);
		if (SwapBlockMatchHoriz && SwapBlockMatchHoriz->Blocks.Num() > 0) {
			CurrentMatches.Add(SwapBlockMatchHoriz);
		}
		if (SwapBlockMatchVert && SwapBlockMatchVert->Blocks.Num() > 0) {
			CurrentMatches.Add(SwapBlockMatchVert);
		}
	}
	if (CurrentMatches.Num() == 0)
	{
		UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("  MoveBlock %s no matches for move to %s"), *MovingBlock->GetName(), *ToCell->GetCoords().ToString());
		// No matches, swap the blocks back to orignal spots
		PlaySoundQueue.AddUnique(MoveFailSound.Get());
		FromCell->CurrentBlock = MovingBlock;
		MovingBlock->OwningGridCell = FromCell;
		MovingBlock->OnMoveFail(ToCell);
		ToCell->CurrentBlock = nullptr;
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
		UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("  MoveBlock %s block moved to to %s"), *MovingBlock->GetName(), *ToCell->GetCoords().ToString());
		GridLockedState = EMMGridLockState::Unchecked;
		MovingBlock->bMovedByPlayer = true;
		UnsettledBlocks.AddUnique(MovingBlock);
		MovingBlock->OnMove(ToCell);
		if (SwappingBlock) {
			UnsettledBlocks.AddUnique(SwappingBlock);
			SwappingBlock->OnMove(FromCell);
		}
		else
		{
			// If we moved the block but didn't swap a block, then the block's original cell (FromCell) is now empty.
			CellBecameOpen(FromCell);
			// And we also need to drop an extra block into this column since the column lost one.
			DropRandomBlockInColumn(FromCell);
		}
		BlockMatches.Append(CurrentMatches);
		SortMatches();
		PlayerMovesCount++;
		OnPlayerMoved.Broadcast(this);
		if (MaxPlayerMovesCount > 0 && PlayerMovesCount >= MaxPlayerMovesCount) {
			OnMaxPlayerMoves.Broadcast(this);
		}
		bAllMatchesFinished = false;
	}
	// TODO: Remove this debug
	DebugBlocks(FString("MoveBlock"));
	return true;
}


bool AMMPlayGrid::BlockMoveHasMatch(const AMMBlock* CheckBlock, const EMMDirection DirectionToCheck)
{
	if (CheckBlock == nullptr) {
		return false;
	}
	if (CheckBlock->OwningGridCell == nullptr) {
		if (CheckBlock->Cell()) {
			UE_CLOG(bDebugLog, LogMMGame, Warning, TEXT("    BlockMoveHasMatch found block %s at %s that has no owning grid cell. Returning false."), *CheckBlock->GetName(), *CheckBlock->Cell()->GetCoords().ToString());
		}
		return false;
	}
	if (CheckBlock->OwningGridCell->CurrentBlock != CheckBlock) {
		UE_CLOG(bDebugLog, LogMMGame, Warning, TEXT("    BlockMoveHasMatch found block %s at %s who's owning cell's current block is not this block. It is block %s"), *CheckBlock->GetName(), *CheckBlock->OwningGridCell->GetCoords().ToString(), *CheckBlock->OwningGridCell->CurrentBlock->GetName());
		return false;
	}
	FIntPoint NeighborCoords = CheckBlock->OwningGridCell->GetCoords() + UMMMath::DirectionToOffset(DirectionToCheck);
	AMMPlayGridCell* ToCell = GetCell(NeighborCoords);
	if (ToCell == nullptr) {
		return false;
	}
	UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("    BlockMoveHasMatch checking %s at %s to %s"), *CheckBlock->GetName(), *CheckBlock->OwningGridCell->GetCoords().ToString(), *ToCell->GetCoords().ToString());
	bool bFoundMatch = false;
	AMMPlayGridCell* FromCell = CheckBlock->OwningGridCell;
	AMMBlock* SwappingBlock = ToCell->CurrentBlock;
	if (SwappingBlock && SwappingBlock->OwningGridCell == nullptr) {
		UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("   BlockMoveHasMatch found swapping block %s at %s that has no owning grid cell. Returning false."), *SwappingBlock->GetName(), *SwappingBlock->GetCoords().ToString());
		return false;
	}
	if (!CheckBlock->CanMove() || (SwappingBlock != nullptr && !SwappingBlock->CanMove())) {
		UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("   BlockMoveHasMatch found immobile block. Returning False."));
		return false;
	}
	if (!UMMMath::CoordsAdjacent(FromCell->GetCoords(), ToCell->GetCoords())) {
		UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("   BlockMoveHasMatch coords %s and %s not adjacent. Returning False."), *FromCell->GetCoords().ToString(), *ToCell->GetCoords().ToString());
		return false;
	}
	// Get non-const block ref so we can temporarily move it.
	AMMBlock* MoveBlock = CheckBlock->OwningGridCell->CurrentBlock;
	// Swap the blocks for now
	ToCell->CurrentBlock = MoveBlock;
	MoveBlock->OwningGridCell = ToCell;
	FromCell->CurrentBlock = nullptr;
	if (SwappingBlock)
	{
		FromCell->CurrentBlock = SwappingBlock;
		SwappingBlock->OwningGridCell = FromCell;
	}
	// Check for matches on moved block
	int32 MinMatchSize = GetMinimumMatchSize();
	UBlockMatch* TmpBlockMatch = nullptr;
	if (MatchHorizontal(MoveBlock, &TmpBlockMatch, false) >= MinMatchSize) {
		bFoundMatch = true;
	}
	else {
		TmpBlockMatch = nullptr;
	}
	if (!bFoundMatch && MatchVertical(MoveBlock, &TmpBlockMatch, false) >= MinMatchSize)	{
		bFoundMatch = true;
	}
	else {
		TmpBlockMatch = nullptr;
	}
	// Swap the blocks back to original spots after checking
	FromCell->CurrentBlock = MoveBlock;
	MoveBlock->OwningGridCell = FromCell;
	ToCell->CurrentBlock = nullptr;
	if (SwappingBlock)
	{
		ToCell->CurrentBlock = SwappingBlock;
		SwappingBlock->OwningGridCell = ToCell;
	}
	if (bFoundMatch){
		if (TmpBlockMatch) {
			UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("    BlockMoveHasMatch found match of %d blocks"), TmpBlockMatch->Blocks.Num());
		}
		else {
			UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("    BlockMoveHasMatch found match: block at %s matches block at %s"), *FromCell->GetCoords().ToString(), *ToCell->GetCoords().ToString());
		}
	}
	return bFoundMatch;
}


bool AMMPlayGrid::BlockHasMatchingMove(const AMMBlock* CheckBlock, AMMPlayGridCell** MatchMoveDestination)
{
	UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("  BlockHasMatchingMove checking block %s at %s"), *CheckBlock->GetName(), *CheckBlock->GetCoords().ToString());
	for (EMMDirection CurDirection : UMMMath::OrthogonalDirections)
	{
		if (BlockMoveHasMatch(CheckBlock, CurDirection))
		{
			if (MatchMoveDestination != nullptr) {
				(*MatchMoveDestination) = GetCell(CheckBlock->GetCoords() + UMMMath::DirectionToOffset(CurDirection));
			}
			return true;
		}
	}
	UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("  BlockHasMatchingMove no match for block %s at %s"), *CheckBlock->GetName(), *CheckBlock->GetCoords().ToString());
	return false;
}


bool AMMPlayGrid::CheckFlaggedForMatches()
{
	GridState = EMMGridState::Matching;
	TArray<UBlockMatch*> CurrentMatches;
	for (AMMBlock* Block : BlocksToCheck)
	{
		UBlockMatch* BlockMatchHoriz = nullptr;
		UBlockMatch* BlockMatchVert = nullptr;
		CheckForMatches(Block, &BlockMatchHoriz, &BlockMatchVert);
		if (BlockMatchHoriz && BlockMatchHoriz->Blocks.Num() > 0) {
			CurrentMatches.Add(BlockMatchHoriz);
		}
		if (BlockMatchVert && BlockMatchVert->Blocks.Num() > 0) {
			CurrentMatches.Add(BlockMatchVert);
		}
	}
	BlocksToCheck.Empty();
	if (CurrentMatches.Num() > 0) 
	{
		BlockMatches.Append(CurrentMatches);
		SortMatches();
	}
	if (BlockMatches.Num() > 0) 
	{
		bAllMatchesFinished = false;
		ResolveMatches();
		return true;
	}
	else {
		return false;
	}
}


bool AMMPlayGrid::CheckForMatches(AMMBlock* CheckBlock, UBlockMatch** HorizMatchPtr, UBlockMatch** VertMatchPtr, const bool bMarkBlocks)
{
	if (CheckBlock == nullptr) {
		return false;
	}
	UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("MMPlayGrid::CheckForMatches - Checking block %s at %s"), *CheckBlock->GetName(), *CheckBlock->GetCoords().ToString());
	if (!CheckBlock->bMatchedHorizontal) {
		MatchHorizontal(CheckBlock, HorizMatchPtr, bMarkBlocks);
	}
	if (!CheckBlock->bMatchedVertical) {
		MatchVertical(CheckBlock, VertMatchPtr, bMarkBlocks);
	}
	return (HorizMatchPtr && (*HorizMatchPtr != nullptr) && (*HorizMatchPtr)->Blocks.Num() > 0) || (VertMatchPtr && (*VertMatchPtr != nullptr) && (*VertMatchPtr)->Blocks.Num() > 0);
}


int32 AMMPlayGrid::MatchHorizontal(UPARAM(ref) AMMBlock* StartBlock, UBlockMatch** MatchPtr, const bool bMarkBlocks)
{
	MatchDirection(StartBlock, EMMDirection::West, MatchPtr, true);
	MatchDirection(StartBlock, EMMDirection::East, MatchPtr, true);
	if (MatchPtr && *MatchPtr != nullptr)
	{
		if ((*MatchPtr)->Blocks.Num() >= GetMinimumMatchSize())
		{
			(*MatchPtr)->Orientation = EMMOrientation::Horizontal;
			if (bMarkBlocks) 
			{
				for (AMMBlock* CurBlock : (*MatchPtr)->Blocks) {
					CurBlock->bMatchedHorizontal = true;
				}
				(*MatchPtr)->Sort();
			}
			UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("    MMPlayGrid::MatchHorizontal - Found full match for block %s at %s match has %d blocks"), *StartBlock->GetName(), *StartBlock->GetCoords().ToString(), (*MatchPtr)->Blocks.Num());
		}
		else {
			(*MatchPtr)->Reset();
		}
		return (*MatchPtr)->Blocks.Num();
	}
	return 0;
}


int32 AMMPlayGrid::MatchVertical(UPARAM(ref) AMMBlock* StartBlock, UBlockMatch** MatchPtr, const bool bMarkBlocks)
{
	MatchDirection(StartBlock, EMMDirection::South, MatchPtr, true);
	MatchDirection(StartBlock, EMMDirection::North, MatchPtr, true);
	if (MatchPtr && *MatchPtr != nullptr)
	{
		if ((*MatchPtr)->Blocks.Num() >= GetMinimumMatchSize())
		{
			(*MatchPtr)->Orientation = EMMOrientation::Vertical;
			if (bMarkBlocks) 
			{
				for (AMMBlock* CurBlock : (*MatchPtr)->Blocks) {
					CurBlock->bMatchedVertical = true;
				}
				(*MatchPtr)->Sort();
			}
			UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("    MMPlayGrid::MatchVertical - Found full match for block %s at %s match has %d blocks"), *StartBlock->GetName(), *StartBlock->GetCoords().ToString(), (*MatchPtr)->Blocks.Num());
		}
		else {
			(*MatchPtr)->Reset();
		}
		return (*MatchPtr)->Blocks.Num();
	}
	return 0;
}


int32 AMMPlayGrid::MatchDirection(AMMBlock* StartBlock, const EMMDirection Direction, UBlockMatch** MatchPtr, const bool bRecurse)
{
	if (StartBlock == nullptr) { return false; }
	if (StartBlock->Cell())
	{
		if (StartBlock->OwningGridCell && StartBlock->OwningGridCell->CurrentBlock != StartBlock) {
			UE_LOG(LogMMGame, Error, TEXT("    MMPlayGrid::MatchDirection - Cell at start coords: %s has different block %s than start block %s"), *(StartBlock->Cell()->GetCoords()).ToString(), *StartBlock->Cell()->CurrentBlock->GetName(), *StartBlock->GetName());
		}
		if (StartBlock->OwningGridCell == nullptr) {
			UE_CLOG(bDebugLog, LogMMGame, Warning, TEXT("    MMPlayGrid::MatchDirection - Attempting match with block %s at %s that does not own it's cell"), *StartBlock->GetName(), *StartBlock->GetCoords().ToString());
		}
		FIntPoint NeighborCoords = StartBlock->GetCoords() + UMMMath::DirectionToOffset(Direction);
		AMMPlayGridCell* NeighborCell = GetCell(NeighborCoords);
		if (NeighborCell) {
			UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("    MMPlayGrid::MatchDirection - Matching starting block %s at %s with cell at %s"), *StartBlock->GetName(), *StartBlock->GetCoords().ToString(), *NeighborCoords.ToString());
		}
		if (NeighborCell && IsValid(NeighborCell->CurrentBlock))
		{
			if ((UMMMath::DirectionIsHorizontal(Direction) && NeighborCell->CurrentBlock->bMatchedHorizontal) ||
				(UMMMath::DirectionIsVertical(Direction) && NeighborCell->CurrentBlock->bMatchedVertical))
			{
				if (MatchPtr != nullptr && *MatchPtr != nullptr) {
					return (*MatchPtr)->Blocks.Num();
				}
				else {
					return 0;
				}
			}
			bool bAlreadyMatched = false;
			// Handle bMatchNextToPreviousInMatchGroup. If we already have some matched blocks, compare our neighbor to the previous 
			// non-bMatchNextToPreviousInMatchGroup block found in the match instead of thls start block.
			if (StartBlock->GetBlockType().bMatchNextToPreviousInMatchGroup && (*MatchPtr != nullptr) && (*MatchPtr)->Blocks.Num() > 1)
			{
				// find the last non-any block in match
				AMMBlock* LastMatchingBlock = nullptr;
				for (int32 i = (*MatchPtr)->Blocks.Num() - 1; i >= 0; i--)
				{
					LastMatchingBlock = (*MatchPtr)->Blocks[i];
					if (!LastMatchingBlock->GetBlockType().bMatchNextToPreviousInMatchGroup) {
						break;
					}
					LastMatchingBlock = nullptr;
				}
				if (LastMatchingBlock != nullptr)
				{
					// Check those blocks for match, because if they don't match then reset the match
					if (LastMatchingBlock->Matches(NeighborCell->CurrentBlock))	{
						bAlreadyMatched = true;						
					}
					else 
					{
						// They didn't match.
						// If the match is already large enough, stop matching and return as normal
						if ((*MatchPtr)->Blocks.Num() >= GetMinimumMatchSize()) {
							UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("    MMPlayGrid::MatchDirection - Found match for block %s at %s match has %d blocks"), *StartBlock->GetName(), *StartBlock->GetCoords().ToString(), (*MatchPtr)->Blocks.Num());
							return (*MatchPtr)->Blocks.Num();
						}
						// Otherwise, clear the match
						(*MatchPtr)->Reset();
					}					
				}
				// If we didn't find a last matching block, then all the current matching blocks are bMatchNextToPreviousInMatchGroup=true.
				// So, just do the normal match below.
			}
			if (bAlreadyMatched || StartBlock->Matches(NeighborCell->CurrentBlock))
			{
				UBlockMatch* Match;
				if (MatchPtr != nullptr && *MatchPtr != nullptr) {
					Match = *MatchPtr;
				}
				else 
				{
					Match = NewObject<UBlockMatch>(this);
					Match->Blocks.Empty();
					(*MatchPtr) = Match;
					if (UMMMath::DirectionIsHorizontal(Direction)) {
						Match->Orientation = EMMOrientation::Horizontal;
					}
					else {
						Match->Orientation = EMMOrientation::Vertical;
					}
				}				
				if (Match->Blocks.Num() == 0) { Match->Blocks.Add(StartBlock); }
				Match->Blocks.Add(NeighborCell->CurrentBlock);
				if (bRecurse && Match->Blocks.Num() <= 40) {
					MatchDirection(NeighborCell->CurrentBlock, Direction, &Match, bRecurse);
				}
			}
		}
	}
	if (MatchPtr != nullptr && *MatchPtr != nullptr) {
		UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("    MMPlayGrid::MatchDirection - Found match for block %s at %s match has %d blocks"), *StartBlock->GetName(), *StartBlock->GetCoords().ToString(), (*MatchPtr)->Blocks.Num());
		return (*MatchPtr)->Blocks.Num();
	}
	else {
		return 0;
	}
}


void AMMPlayGrid::SortMatches(/*const bool bForceSort*/ )
{
	int32 Index;
	TArray<UBlockMatch*> TmpMatches;
	TmpMatches.Reserve(BlockMatches.Num());
	for (UBlockMatch* Match : BlockMatches)	{
		Match->Sort();
	}
	for (UBlockMatch* Match : BlockMatches)
	{
		Index = 0;
		for (UBlockMatch* CompareMatch : TmpMatches)
		{
			if (CompareMatch == nullptr) { 
				break; 
			}
			if (Match->StartCoords.Y < CompareMatch->StartCoords.Y || (Match->StartCoords.Y == CompareMatch->StartCoords.Y && Match->StartCoords.X < CompareMatch->StartCoords.X)) {
				break;
			}
			Index++;
		}
		TmpMatches.Insert(Match, Index);
	}
	BlockMatches.Empty(TmpMatches.Num());
	BlockMatches.Append(TmpMatches);
	UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("MMPlayGrid::SortMatches - Sorted %d matches"), BlockMatches.Num());
}


bool AMMPlayGrid::ResolveMatches()
{
	GridState = EMMGridState::Matching;
	AMMGameMode* GameMode = Cast<AMMGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GameMode) {
		return false;
	}
	UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("MMPlayGrid::ResolveMatches - Resolving %d matches"), BlockMatches.Num());
	int32 TotalScoreToAdd = 0;
	TArray<FGoodsQuantity> TotalGoods;
	for (int32 MatchIndex = 0; MatchIndex < BlockMatches.Num(); MatchIndex++)
	{
		UBlockMatch* CurMatch = BlockMatches[MatchIndex];
		FGoodsQuantitySet MatchGoods;
		GameMode->GetGoodsForMatch(CurMatch, MatchGoods);
		if (MatchGoods.Goods.Num() > 0) 
		{
			CurMatch->TotalGoods = MatchGoods.Goods;
			TotalGoods.Append(MatchGoods.Goods);
		}
		TotalScoreToAdd += GameMode->GetScoreForMatch(CurMatch);
		CurMatch->TotalScore = TotalScoreToAdd;
		for (AMMBlock* CurBlock : CurMatch->Blocks)
		{
			CurBlock->OnMatched(CurMatch);
			PlaySoundQueue.AddUnique(CurBlock->MatchSound.Get());			
		}		
	}
	AddScore(TotalScoreToAdd);
	if (TotalGoods.Num() > 0)
	{
		AMMPlayerController* PC = Cast<AMMPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
		PC->GoodsInventory->AddSubtractGoodsArray(TotalGoods, false, true);
	}
	// Call the notification delegate
	OnMatchAwards.Broadcast(BlockMatches);
	return true;
}


bool AMMPlayGrid::PerformActionsForMatch(UBlockMatch* Match)
{
	check(Match);
	bool bAnyPerformed = false;
	bool bAnyPerformedForBlock = false;
	int32 MatchSize = Match->Blocks.Num();
	if (MatchSize == 0) {
		return false;
	}
	if (!Match->Blocks.IsValidIndex(0) || Match->Blocks[0]->Grid() == nullptr) {
		return false;
	}
	TArray<FName> ProcessedBlockTypeNames;
	int32 BonusMatchSize = MatchSize - Match->Blocks[0]->Grid()->GetMinimumMatchSize();
	for (AMMBlock* Block : Match->Blocks)
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
			}
			if (bAnyPerformedForBlock) {
				ProcessedBlockTypeNames.Add(Block->GetBlockType().Name);
			}
		}		
	}
	return bAnyPerformed;
}


bool AMMPlayGrid::PerformActionType(const FMatchActionType& MatchActionType, const UBlockMatch* Match)
{
	UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("MMPlayGrid::PerformActionType - performing %s action for match %s"), *MatchActionType.MatchActionClass->GetName(), *Match->GetName());
	bool bSuccess = false;
	UMatchAction* MatchAction = NewObject<UMatchAction>(this, MatchActionType.MatchActionClass);
	if (MatchAction) {
		bSuccess = MatchAction->Perform(Match, MatchActionType);
	}
	MatchAction = nullptr;
	return bSuccess;
}



EMMGridLockState AMMPlayGrid::CheckGridIsLocked()
{
	if (Blocks.Num() == 0 || Cells.Num() == 0) {
		GridLockedState = EMMGridLockState::NotLocked;
		return GridLockedState;
	}
	if (GridLockedState == EMMGridLockState::Locked || GridLockedState == EMMGridLockState::NotLocked) {
		return GridLockedState;
	}
	if (GridLockedState == EMMGridLockState::Unchecked) 
	{
		GridLockCheckCoords = FIntPoint::ZeroValue;
		GridLockedState = EMMGridLockState::Checking;
	}
	UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("CheckGridIsLocked checking"));
	for (int32 i = 0; i < GridLockChecksPerTick; i++)
	{
		// Check current GridLockCheckCoords
		AMMBlock* CurBlock = GetBlock(GridLockCheckCoords);
		if (CurBlock && BlockHasMatchingMove(CurBlock)) 
		{
			GridLockedState = EMMGridLockState::NotLocked;
			return GridLockedState;
		}
		// Determine next cell to check
		if (GridLockCheckCoords.X == SizeX - 1) {
			// At the end of a row. Check next row up.
			if (GridLockCheckCoords.Y == SizeY - 1)
			{
				// At the top row then there was no match move, so grid is locked.
				GridLockedState = EMMGridLockState::Locked;
				return GridLockedState;
			}
			else
			{
				// Move to next row
				GridLockCheckCoords.X = 0;
				GridLockCheckCoords.Y++;
			}
		}
		else {
			// Move to next cell in row
			GridLockCheckCoords.X++;
		}
	}
	UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("CheckGridIsLocked %s"), GridLockedState == EMMGridLockState::Locked ? TEXT("Locked") : TEXT("Not Locked"));
	return GridLockedState;
}



void AMMPlayGrid::SettleBlocks()
{
	GridState = EMMGridState::Settling;
	DebugBlocks(FString("SettleBlocksStart"));
	// Unsettle all blocks that were queued for unsettling
	for (AMMBlock* CurBlock : UnsettledBlocks)
	{
		if (!CurBlock->bUnsettled) {
			CurBlock->OnUnsettle();
		}
	}
	// TODO: Remove this debug
	DebugBlocks(FString("SettleBlocksDone"));
}


void AMMPlayGrid::UnsettleBlock(AMMBlock* Block)
{
	if (!IsValid(Block)) {
		return;
	}
	// If it is a movable unmatched block, unsettle it.
	if (Block->CanMove() && !Block->IsMatched() && !Block->bUnsettled) 
	{
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
	AMMBlock* AboveBlock = nullptr;
	if (AboveCell) 
	{
		if (IsValid(AboveCell->CurrentBlock) && !AboveCell->CurrentBlock->IsMatched()) {
			ToBeUnsettledBlocks.AddUnique(AboveCell->CurrentBlock);
		}
	}
	else 
	{
		// At top of column, so unsettle lowest block falling into grid that is not already unsettled
		int32 Col = Cell->GetCoords().X;
		AMMBlock* FirstBlock = nullptr;
		for (int32 i = 0; i < BlocksFallingIntoGrid[Col].Blocks.Num(); i++)
		{
			AMMBlock* FallingBlock = BlocksFallingIntoGrid[Col].Blocks[i];
			if (ToBeUnsettledBlocks.Contains(FallingBlock) || UnsettledBlocks.Contains(FallingBlock)) {
				continue;
			}
			if (FirstBlock == nullptr || FallingBlock->GetRelativeLocation().Z < FirstBlock->GetRelativeLocation().Z) {
				FirstBlock = FallingBlock;
			}
		}
		if (FirstBlock) {
			ToBeUnsettledBlocks.AddUnique(FirstBlock);
		}
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
	if (bBlockMoved)
	{
		if (IsValid(Block)) {
			BlocksToCheck.AddUnique(Block);
		}
		if (Block->StopMoveSound) {
			PlaySoundQueue.AddUnique(Block->StopMoveSound.Get());
		}
	}
	if (Block != nullptr) { UnsettledBlocks.RemoveSingle(Block); }
}


void AMMPlayGrid::BlockFinishedMatch(AMMBlock* Block, UBlockMatch* Match)
{
	check(Match);
	bool bWholeMatchFinished = true;
	bool bTmpAllMatchesFinished = true;;
	if (Block->OwningGridCell && Block->OwningGridCell->CurrentBlock == Block) 
	{
		// Clear the owning grid cell. The cell is now open for other blocks. But don't tell grid yet.
		Block->OwningGridCell->CurrentBlock = nullptr;
		//CellBecameOpen(Block->OwningGridCell);
	}
	for (AMMBlock* CurBlock : Match->Blocks)
	{
		if (!CurBlock->IsMatchFinished(Match))
		{
			bWholeMatchFinished = false;
			break;
		}
	}
	if (bWholeMatchFinished) {
		Match->bMatchFinished = true;
	}
	// See if all matches have finished
	for (int32 i = 0; i < BlockMatches.Num(); i++)
	{
		if (!BlockMatches[i]->bMatchFinished)
		{
			bTmpAllMatchesFinished = false;
			break;
		}
	}
	if (bTmpAllMatchesFinished) {
		bAllMatchesFinished = true;
	}
}


void AMMPlayGrid::AllMatchesFinished()
{
	bAllMatchesFinished = false;
	TArray<AMMPlayGridCell*> SpawnCells;
	UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("MMPlayGrid::AllMatchesFinished - processing %d matches"), BlockMatches.Num());
	// Perform all match actions
	for (UBlockMatch* BlockMatch : BlockMatches){
		PerformActionsForMatch(BlockMatch);
	}
	// Apply match damage to unmatched neighbors. 
	for (UBlockMatch* BlockMatch : BlockMatches) {
		// Larger matches do more damage.
		int32 MatchDamage = (BlockMatch->Blocks.Num() - GetMinimumMatchSize()) + 1;
		TArray<AMMPlayGridCell*> Neighbors = BlockMatch->GetCellNeighbors();
		UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("MMPlayGrid::AllMatchesFinished - checking match damage at %s on %d neighbors"), *BlockMatch->StartCoords.ToString(), Neighbors.Num());
		for (AMMPlayGridCell* Neighbor : Neighbors)	{
			// Ignore neighbors that are in a match -- they will be destroyed as part of their match.
			if (Neighbor->CurrentBlock && !Neighbor->CurrentBlock->IsMatched()) {
				Neighbor->CurrentBlock->TakeDamage(MatchDamage);
			}
		}
	}
	// When all matches finished, destroy blocks in all the matches.
	// Also, if cell is still empty add it to list
	for (int32 i = 0; i < BlockMatches.Num(); i++)
	{
		int32 BlockMatchSize = BlockMatches[i]->Blocks.Num();
		for (int32 BIndex = 0; BIndex < BlockMatchSize; BIndex++)
		{
			AMMBlock* CurBlock = BlockMatches[i]->Blocks[BIndex];
			if (IsValid(CurBlock))
			{
				AMMPlayGridCell* BlockCell = CurBlock->Cell();
				UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("MMPlayGrid::AllMatchesFinished - DESTROYING block %s at %s"), *CurBlock->GetName(), *CurBlock->GetCoords().ToString());
				Blocks.RemoveSingle(CurBlock);
				UnsettledBlocks.RemoveSingle(CurBlock);
				ToBeUnsettledBlocks.RemoveSingle(CurBlock);
				BlocksToCheck.RemoveSingle(CurBlock);
				CurBlock->DestroyBlock();	
				check(BlockCell);
				if (!IsValid(BlockCell->CurrentBlock)) {
					SpawnCells.AddUnique(BlockCell);
				}
				else {
					UE_CLOG(bDebugLog, LogMMGame, Warning, TEXT("     cell at %s has valid block %s. Will not spawn new dropping cell"), *BlockCell->GetCoords().ToString(), *BlockCell->CurrentBlock->GetName());
				}
			}
		}
		BlockMatches[i]->Blocks.Empty();
	}
	// Destroy blocks queueud for destruction
	for (AMMBlock* Block : BlocksToDestroy)
	{
		if (IsValid(Block))
		{
			AMMPlayGridCell* BlockCell = Block->Cell();
			Blocks.RemoveSingle(Block);
			UnsettledBlocks.RemoveSingle(Block);
			ToBeUnsettledBlocks.RemoveSingle(Block);
			BlocksToCheck.RemoveSingle(Block);
			Block->DestroyBlock();
			UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("     ##### DESTROYED damaged block at %s"), *BlockCell->GetCoords().ToString());
			//UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("       Droppping new block in column %d"), BlockCell->GetCoords().X);
			//DropRandomBlockInColumn(BlockCell);
			//CellBecameOpen(BlockCell);
			SpawnCells.AddUnique(BlockCell);
		}
	}
	BlocksToDestroy.Empty();
	if (!bPauseNewBlocks) 
	{
		// Drop blocks in column above each empty cell
		AMMBlock* NewBlock;
		TArray<FName> ExcludedNames;
		for (AMMPlayGridCell* SpawnCell : SpawnCells) 
		{
			UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("    match logic dropping block in column %d for cell %s"), SpawnCell->GetCoords().X, *SpawnCell->GetCoords().ToString());
			NewBlock = DropRandomBlockInColumnEx(SpawnCell, ExcludedNames);
			if (!IsValid(NewBlock)) {
				UE_LOG(LogMMGame, Error, TEXT("MMPlayGrid::AllMatchesFinished - match logic FAILED dropping block in column %d"), SpawnCell->GetCoords().X);
			}
			if (NewBlock && GetMinimumMatchSize() > 2) 
			{
				if (ExcludedNames.Num() >= GetMinimumMatchSize() - 1) 
				{
					ExcludedNames.SwapMemory(0, ExcludedNames.Num() - 1);
					ExcludedNames.Pop(false);
				}
				ExcludedNames.Add(NewBlock->GetBlockType().Name);			
			}
		}
	}
	// Notify grid of each empty cell
	for (AMMPlayGridCell* SpawnCell : SpawnCells)
	{
		if (!IsValid(SpawnCell->CurrentBlock)) {
			CellBecameOpen(SpawnCell);
		}
	}
	BlockMatches.Empty();	
}


void AMMPlayGrid::BlockDestroyedByDamage(AMMBlock* Block)
{
	check(Block);
	UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("MMPlayGrid::BlockDestroyedByDamage - will destroy block %s at %s"), *Block->GetName(), *Block->GetCoords().ToString());
	if (IsValid(Block) && !Block->IsMatched()) {
		BlocksToDestroy.AddUnique(Block);
	}
	else {
		UE_CLOG(bDebugLog, LogMMGame, Warning, TEXT("MMPlayGrid::BlockDestroyedByDamage - Did not destroy block %s at %d"), *Block->GetName(), *Block->GetCoords().ToString());
	}
}


void AMMPlayGrid::PlaySounds(const TArray<USoundBase*> Sounds)
{
	for (USoundBase* Sound : Sounds) {
		UGameplayStatics::PlaySound2D(GetWorld(), Sound, 1.f, 1.f, 0.f);
	}
}


bool AMMPlayGrid::DebugBlocks(const FString& ContextString)
{
	if (!bDebugLog) { return false; }
	bool bAnyError = false;
	if (!ContextString.IsEmpty()) {
		UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("DebugBlocks: ####  %s  ###"), *ContextString);
	}
	UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("DebugBlocks: Blocks list has %d blocks"), Blocks.Num());
	UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("DebugBlocks: UnsettledBlocks has %d blocks"), UnsettledBlocks.Num());
	UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("DebugBlocks: ToBeUnsettledBlocks has %d blocks"), ToBeUnsettledBlocks.Num());
	UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("DebugBlocks: BlocksToCheck has %d blocks"), BlocksToCheck.Num());
	for (int32 i = 0; i < BlocksFallingIntoGrid.Num(); i++) {
		UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("DebugBlocks: BlocksFallingIntoGrid[%d] has %d blocks"), i, BlocksFallingIntoGrid[i].Blocks.Num());
	}
	if (BlockMatches.Num() > 0) {
		UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("DebugBlocks: BlockMatches has %d matches"), BlockMatches.Num());
	}
	for (AMMBlock* CurBlock : Blocks)
	{
		if (CurBlock->Cell() == nullptr) {
			UE_CLOG(bDebugLog, LogMMGame, Warning, TEXT("DebugBlocks: Block %s has no cell"), *CurBlock->GetName());
			bAnyError = true;
		}
		else if (CurBlock->Cell()->CurrentBlock == nullptr) {
			UE_CLOG(bDebugLog, LogMMGame, Warning, TEXT("DebugBlocks: Block %s at %s grid cell current block is null"), *CurBlock->GetName(), *CurBlock->GetCoords().ToString());
			bAnyError = true;
		}
		if (CurBlock->OwningGridCell && CurBlock->OwningGridCell->CurrentBlock != CurBlock)
		{
			UE_LOG(LogMMGame, Error, TEXT("DebugBlocks: Block %s at %s owning grid cell references different block %s at %s"), *CurBlock->GetName(), *CurBlock->GetCoords().ToString(), *CurBlock->Cell()->CurrentBlock->GetName(), *CurBlock->Cell()->CurrentBlock->GetCoords().ToString());
			bAnyError = true;
		}
		if (CurBlock->SettleToGridCell != nullptr && !CurBlock->bFallingIntoGrid)
		{
			UE_LOG(LogMMGame, Error, TEXT("DebugBlocks: Block %s at %s has settle to grid cell but is not falling into grid"), *CurBlock->GetName(), *CurBlock->GetCoords().ToString());
			bAnyError = true;
		}
		
		if (CurBlock->BlockState == EMMGridState::Settling && !UnsettledBlocks.Contains(CurBlock)) {
			UE_CLOG(bDebugLog, LogMMGame, Warning, TEXT("DebugBlocks: Block %s at %s is settling but not in UnsettledBlocks list"), *CurBlock->GetName(), *CurBlock->GetCoords().ToString());
			bAnyError = true;
		}
		if (CurBlock->BlockState == EMMGridState::Matching)
		{
			int32 BlockIndex;
			bool bFound = false;
			for (int32 i = 0; i < BlockMatches.Num(); i++)
			{
				if (BlockMatches[i]->Blocks.Find(CurBlock, BlockIndex)) {
					bFound = true;
				}
			}
			if (!bFound) {
				UE_LOG(LogMMGame, Error, TEXT("DebugBlocks: Block %s at %s is matching but not in the BlockMatches list"), *CurBlock->GetName(), *CurBlock->GetCoords().ToString());
				bAnyError = true;
			}
		}
	}
	TArray<AActor*> AllBlocks;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMMBlock::StaticClass(), AllBlocks);
	if (Blocks.Num() != AllBlocks.Num()) {
		UE_CLOG(bDebugLog, LogMMGame, Warning, TEXT("DebugBlocks: %d blocks found in world"), AllBlocks.Num());
	}
	for (AActor* CurActor : AllBlocks)
	{
		AMMBlock* CurBlock = Cast<AMMBlock>(CurActor);
		if (CurBlock)
		{
			if (!Blocks.Contains(CurBlock))
			{
				UE_CLOG(bDebugLog, LogMMGame, Warning, TEXT("DebugBlocks: Block %s found in world but not in Blocks list"), *CurBlock->GetName());
				if (CurBlock->Cell()) {
					UE_CLOG(bDebugLog, LogMMGame, Warning, TEXT("    Orphaned block's cell is %s"), *CurBlock->Cell()->GetCoords().ToString());
				}
			}
		}
	}
	return bAnyError;
}


void AMMPlayGrid::InitBlocksFallingIntoGrid()
{
	if (BlocksFallingIntoGrid.Num() != SizeX) 
	{
		BlocksFallingIntoGrid.Empty();
		BlocksFallingIntoGrid.Reserve(SizeX);
		for (int32 i = 0; i < SizeX; i++) {
			BlocksFallingIntoGrid.Add(i, FBlockSet());
		}
	}
	else 
	{
		for (int32 i = 0; i < BlocksFallingIntoGrid.Num(); i++) {
			BlocksFallingIntoGrid[i].Blocks.Empty();
		}
	}
}

#undef LOCTEXT_NAMESPACE
