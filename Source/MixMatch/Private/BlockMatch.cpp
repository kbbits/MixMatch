
#include "Algo/Sort.h"
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
	int32 MatchSize = Blocks.Num();
	TArray<AMMBlock*> TmpBlocks;
	TArray<AMMBlock*> SortedBlocks;
	int32 StartIndex = -1;
	// Check any two blocks (this looks at the first two). If they have different Y coords, it is a vertical match. Horizontal otherwise.
	Orientation = Blocks[0]->GetCoords().Y != Blocks[1]->GetCoords().Y ? EMMOrientation::Vertical : EMMOrientation::Horizontal;
	StartCoords = FIntPoint(-1, -1);
	EndCoords = FIntPoint(-1, -1);
	for (AMMBlock* Block : Blocks)
	{
		FIntPoint Coords = Block->GetCoords();
		if (Orientation == EMMOrientation::Horizontal)
		{
			// Horiz. match, start coords at lowest X coord, end coords at highest X
			if (StartCoords.X < 0 || Coords.X < StartCoords.X) {
				StartCoords = Coords;
			}
			if (EndCoords.X < 0 || Coords.X > EndCoords.X) {
				EndCoords = Coords;
			}
			// Validity Check - all should have same Y coord
			if (Coords.Y != StartCoords.Y) {
				UE_LOG(LogMMGame, Error, TEXT("BlockMatch::Sort - Found block out of line orientation. Y coord should be %d but was %d"), StartCoords.Y, Coords.Y);
			}
		}
		else
		{
			// Vert. match, start coords at lowest Y coord, end coords and highest Y
			if (StartCoords.Y < 0 || Coords.Y < StartCoords.Y) {
				StartCoords = Coords;
			}
			if (EndCoords.Y < 0 || Coords.Y > EndCoords.Y) {
				EndCoords = Coords;
			}
			// Validity Check - all should have same X coord
			if (Coords.X != StartCoords.X) {
				UE_LOG(LogMMGame, Error, TEXT("BlockMatch::Sort - Found block out of line orientation. X coord should be %d but was %d"), StartCoords.X, Coords.X);
			}
		}
	}
	// Validitiy check. Distance between start and end should equal the number of blocks in the match - 1.
	if ((Orientation == EMMOrientation::Horizontal && (EndCoords.X - StartCoords.X != MatchSize - 1)) ||
		(Orientation == EMMOrientation::Vertical && (EndCoords.Y - StartCoords.Y != MatchSize - 1))) 
	{
		UE_LOG(LogMMGame, Error, TEXT("BlockMatch::Sort - Found match with start %s end %s but has %d blocks."), *StartCoords.ToString(), *EndCoords.ToString(), MatchSize);
	}
	if (Orientation == EMMOrientation::Horizontal) {
		Algo::Sort(Blocks, [](const AMMBlock* a, const AMMBlock* b) { return a->GetCoords().X < b->GetCoords().X; });
	}
	else {
		Algo::Sort(Blocks, [](const AMMBlock* a, const AMMBlock* b) { return a->GetCoords().Y < b->GetCoords().Y; });
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