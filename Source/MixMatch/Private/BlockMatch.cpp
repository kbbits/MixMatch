
#include "BlockMatch.h"
#include "..\MixMatch.h"
#include "MMPlayGrid.h"
#include "MMPlayGridCell.h"
#include "MMBlock.h"
#include "MMMath.h"



/*
*/
UBlockMatch::UBlockMatch()
	: Super()
{
}


void UBlockMatch::Sort(const bool bForceSort)
{
	if ((bSorted && !bForceSort) || Blocks.Num() <= 1) { return; }
	FIntPoint MinX = FIntPoint(-1, -1);
	FIntPoint MinY = FIntPoint(-1, -1);
	int32 MatchSize = Blocks.Num();
	TArray<AMMBlock*> TmpBlocks;
	TArray<AMMBlock*> SortedBlocks;
	int32 StartIndex = -1;
	for (AMMBlock* Block : Blocks)
	{
		FIntPoint Coords = Block->GetCoords();
		if (Orientation == EMMOrientation::Unknown && MinX.X >= 0 && MinY.Y >= 0)
		{
			Orientation = Coords.Y != MinY.Y ? EMMOrientation::Vertical : EMMOrientation::Horizontal;
		}
		if (MinX.X < 0 || Coords.X < MinX.X) {
			MinX = Coords;
		}
		if (MinY.Y < 0 || Coords.Y < MinY.Y) {
			MinY = Coords;
		}
	}
	StartCoords = Orientation == EMMOrientation::Horizontal ? MinX : MinY;
	EndCoords = Orientation == EMMOrientation::Horizontal ? StartCoords + FIntPoint(MatchSize - 1, 0) : StartCoords + FIntPoint(0, MatchSize - 1);
	TmpBlocks = Blocks;
	SortedBlocks.SetNum(TmpBlocks.Num());
	for (int32 i = 0; i < TmpBlocks.Num(); i++)
	{
		if (Orientation == EMMOrientation::Horizontal) {
			SortedBlocks[TmpBlocks[i]->GetCoords().X - MinX.X] = TmpBlocks[i];
		}
		else {
			SortedBlocks[TmpBlocks[i]->GetCoords().Y - MinY.Y] = TmpBlocks[i];
		}
	}
	Blocks.Empty();
	for (AMMBlock* TmpBlock : SortedBlocks) {
		if (TmpBlock) {
			Blocks.Add(TmpBlock);
		}
		else {
			break;
		}
	}
	bSorted = true;
}


void UBlockMatch::Reset()
{
	Blocks.Empty();
	StartCoords = FIntPoint::NoneValue;
	EndCoords = FIntPoint::NoneValue;
	bSorted = false;
	TotalScore = 0;
	TotalGoods.Empty();
	Orientation = EMMOrientation::Unknown;
}


FVector UBlockMatch::GetWorldLocation()
{
	if (Blocks.Num() == 0) return FVector(-1.f, -1.f, -1.f);
	AMMPlayGrid* Grid = Blocks[0]->Grid();
	if (Grid == nullptr) return FVector(-1.f, -1.f, -1.f);
	Sort();
	return Grid->GridFloatCoordsToWorldLocation(FVector2D((float)(StartCoords.X + EndCoords.X) / 2.f, (float)(StartCoords.Y + EndCoords.Y) / 2.f));
}


TArray<AMMPlayGridCell*> UBlockMatch::GetCellNeighbors(const bool bIncludeDiagonal)
{
	TArray<AMMPlayGridCell*> NeighborCells;
	if (Blocks.Num() == 0) {
		return NeighborCells;
	}
	Sort();	
	AMMPlayGrid* Grid = Blocks[0]->Grid();
	check(Grid);
	TArray<FIntPoint> NeighborCoords;
	AMMPlayGridCell* NeighborCell = nullptr;
	// Init offset for horizontal
	FIntPoint MatchDirectionOffset = UMMMath::DirectionToOffset(EMMDirection::East);
	FIntPoint NeighborDirectionOffset = UMMMath::DirectionToOffset(EMMDirection::North);
	if (Orientation == EMMOrientation::Vertical) {
		MatchDirectionOffset = UMMMath::DirectionToOffset(EMMDirection::North);
		NeighborDirectionOffset = UMMMath::DirectionToOffset(EMMDirection::East);
	}
	// Iterate over the row/col below/before match row to the row/col above/after match row.
	for (int32 n = -1; n <= 1; n++)
	{
		// n == 0 is the Row/Col along the match.
		if (n == 0) {
			// Get cell before start cell if it is inside the grid
			//if ((StartCoords - MatchDirectionOffset).GetMin() >= 0) {
				NeighborCoords.Add(StartCoords - MatchDirectionOffset);
			//}
			// Get cell after end cell if it is inside the grid
			//if ((FIntPoint(Grid->SizeX - 1, Grid->SizeY - 1) - (EndCoords + MatchDirectionOffset)).GetMin() >= 0) {
				NeighborCoords.Add(EndCoords + MatchDirectionOffset);
			//}
		}
		else {
			// Neighbors below/before (n=-1) or above/after (n=1) match row/col
			// Start coords at cell before match start cell and in correct col/row (below/above)
			FIntPoint Coords = (StartCoords - MatchDirectionOffset) + (NeighborDirectionOffset * n);
			// If this row/col is outside grid, skip it
			if (Orientation == EMMOrientation::Horizontal) {
				if (Coords.Y < 0 || Coords.Y >= Grid->SizeY) {
					continue;
				}
			}
			else if (Coords.X < 0 || Coords.X >= Grid->SizeX) {
				continue;
			}
			// Iterate over blocks from one before match start to one after match end.
			for (int32 i = -1; i <= Blocks.Num(); i++)
			{
				// Skip coords outside grid (grid currently does not allow negative coordinate)
				// For first and last iteration, only add cell if we're including diagonal.
				if ((Coords.X >= 0 && Coords.Y >= 0) && ((i != -1 && i != Blocks.Num()) || bIncludeDiagonal)) {
					NeighborCoords.Add(Coords);
				}				
				Coords += MatchDirectionOffset;
				// If we're iterating outside the grid size, stop.
				if (Coords.X >= Grid->SizeX || Coords.Y >= Grid->SizeY) {
					break;
				}
			}
		}
	}
	// Go through each neighbor coord and add the cell, if it exists.
	for (FIntPoint NCoords : NeighborCoords)
	{
		NeighborCell = Grid->GetCell(NCoords);
		if (IsValid(NeighborCell)) {
			NeighborCells.Add(NeighborCell);
		}
	}
	return NeighborCells;
}