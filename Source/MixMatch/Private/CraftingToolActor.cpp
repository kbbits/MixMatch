
#include "CraftingToolActor.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "../MixMatch.h"
#include "MMPlayerController.h"
#include "RecipeManagerComponent.h"
#include "RecipePlayGrid.h"



ACraftingToolActor::ACraftingToolActor()
{
	// Create root scene component
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// Create static mesh component
	ToolMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ToolMesh"));
	ToolMesh->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
	ToolMesh->SetupAttachment(SceneRoot);
	ToolMesh->OnClicked.AddDynamic(this, &ACraftingToolActor::ToolClicked);
	ToolMesh->OnInputTouchBegin.AddDynamic(this, &ACraftingToolActor::OnFingerPressedTool);
}


void ACraftingToolActor::CalcCamera(float DeltaTime, struct FMinimalViewInfo& OutResult)
{
	Super::CalcCamera(DeltaTime, OutResult);

	FBoxSphereBounds GridBounds = GetVisibleGridBounds();
	OutResult.FOV = ViewFOV;
	OutResult.Rotation = (GetGridFacing() * -1.0f).Rotation();
	float DistanceH = (GridBounds.BoxExtent.X + GridMarginH) / FMath::Tan(FMath::DegreesToRadians(OutResult.FOV) / 2.0f);
	float DistanceV = (GridBounds.BoxExtent.Z + GridMarginV) / FMath::Tan((FMath::DegreesToRadians(OutResult.FOV) / OutResult.AspectRatio) / 2.0f);
	OutResult.Location = GridBounds.Origin + (GetGridFacing() * (FMath::Max(DistanceH,DistanceV) + ExtraCameraDistance));
}


FVector ACraftingToolActor::GetGridSpawnLocation_Implementation()
{
	return GetActorLocation();
}


void ACraftingToolActor::SetRecipeManager(URecipeManagerComponent* NewRecipeManager)
{
	RecipeManager = NewRecipeManager;
}


URecipeManagerComponent* ACraftingToolActor::GetRecipeManager()
{
	if (IsValid(RecipeManager)) { return RecipeManager; }
	// Grab player's recipe manager if one has not been set.
	AMMPlayerController* PController = Cast<AMMPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (PController) {
		RecipeManager = PController->RecipeManager;
	}
	return RecipeManager;
}


TArray<FCraftingRecipe> ACraftingToolActor::GetCraftableRecipes()
{
	if (GetRecipeManager()) {
		return GetRecipeManager()->GetRecipesWithCategories(CraftableRecipeCategories, true);
	}
	return TArray<FCraftingRecipe>();
}


void ACraftingToolActor::SetRecipe(const FName& RecipeName)
{
	
	if (CraftableRecipeCategories.Num() > 0) 
	{
		FCraftingRecipe TmpRecipe = GetRecipeManager()->GetRecipe(RecipeName);
		for (FName RecipeCatName : TmpRecipe.RecipeCategories)
		{
			if (CraftableRecipeCategories.Contains(RecipeCatName)) {
				CurrentRecipe = TmpRecipe;
				break;
			}
		}
	}
	else {
		CurrentRecipe = GetRecipeManager()->GetRecipe(RecipeName);
	}
	if (CurrentRecipe.Name.IsNone()) {
		UE_LOG(LogMMGame, Error, TEXT("CraftingToolActor::SetRecipe - Recipe %s is not included in %s CraftableRecipeCategories"), *RecipeName.ToString(), *GetName());
	}
	else
	{
		if (IsValid(CurrentGrid))
		{
			ARecipePlayGrid* RecipeGrid = Cast<ARecipePlayGrid>(CurrentGrid);
			if (RecipeGrid)	{
				RecipeGrid->SetRecipe(CurrentRecipe);
			}
		}
	}
}


FCraftingRecipe ACraftingToolActor::GetRecipe()
{
	return CurrentRecipe;
}


TSubclassOf<AMMPlayGridCell> ACraftingToolActor::GetCellClass_Implementation()
{
	return nullptr;
}


int32 ACraftingToolActor::GetTargetBlockTypeCount_Implementation()
{
	return 5;
}


FIntPoint ACraftingToolActor::GetNewGridSize_Implementation()
{
	int32 TargetBlockTypeCount = GetTargetBlockTypeCount();
	return FIntPoint(TargetBlockTypeCount + 2, TargetBlockTypeCount + 2);
}


FVector ACraftingToolActor::GetGridBlockSize_Implementation()
{
	return FVector(-1.f, -1.f, -1.f);
}


float ACraftingToolActor::GetNewBlockDropInHeight_Implementation()
{
	return -1.f;
}


FName ACraftingToolActor::GetDefaultBlockTypeSetName_Implementation()
{
	return FName(TEXT("Default"));
}


void ACraftingToolActor::SpawnGrid()
{
	DestroyGrid();
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;
	TSubclassOf<AMMPlayGrid> LoadedGridClass = GridClass.LoadSynchronous();
	AMMPlayGrid* NewGrid = GetWorld()->SpawnActor<AMMPlayGrid>(
		LoadedGridClass,
		GetGridSpawnLocation(),
		GetActorRotation() + FRotator(0.0f, -90.0f, 0.0f),
		SpawnParams
	);
	check(NewGrid);
	// Basic properties
	FIntPoint NewSize = GetNewGridSize();
	NewGrid->SizeX = NewSize.X;
	NewGrid->SizeY = NewSize.Y;
	if (GetGridBlockSize().GetMax() > 0) {
		NewGrid->BlockSize = GetGridBlockSize();
	}
	if (GetNewBlockDropInHeight() > 0) {
		NewGrid->NewBlockDropInHeight = GetNewBlockDropInHeight();
	}
	NewGrid->BlockTypeSetName = GetDefaultBlockTypeSetName();
	// Recipe grid properties
	ARecipePlayGrid* RecipeGrid = Cast<ARecipePlayGrid>(NewGrid);
	if (RecipeGrid)
	{
		FCraftingRecipe NewRecipe = GetRecipe();
		RecipeGrid->SetRecipe(NewRecipe);
		if (GetCellClass()) {
			RecipeGrid->CellClass = GetCellClass();
		}
		if (GetTargetBlockTypeCount() > 0) {
			RecipeGrid->TargetBlockTypes = GetTargetBlockTypeCount();
		}
	}
	CurrentGrid = NewGrid;
}


void ACraftingToolActor::SpawnGridBackground()
{
	if (IsValid(CurrentGrid)) {
		CurrentGrid->SpawnGrid();
	}
}


void ACraftingToolActor::DestroyGrid()
{
	if (IsValid(CurrentGrid))
	{
		CurrentGrid->DestroyGrid();
		CurrentGrid->Destroy();
		CurrentGrid = nullptr;
	}
}


FVector ACraftingToolActor::GetGridFacing_Implementation()
{
	return GetActorForwardVector();
}

FBoxSphereBounds ACraftingToolActor::GetVisibleGridBounds_Implementation() 
{
	if (IsValid(CurrentGrid))	{
		return CurrentGrid->GetVisibleGridBounds();
	}
	UE_CLOG(bDebugLog, LogMMGame, Log, TEXT("CraftingToolActor::GetVisibleGridBounds - No current grid."));
	FBoxSphereBounds TmpBounds;
	GetActorBounds(true, TmpBounds.Origin, TmpBounds.BoxExtent, true);
	TmpBounds.SphereRadius = TmpBounds.BoxExtent.Size();
	return TmpBounds;
}


void ACraftingToolActor::HandleClick_Implementation()
{
	// Base class does nothing for now.
}


void ACraftingToolActor::ToolClicked(UPrimitiveComponent* ClickedComp, FKey ButtonClicked)
{
	HandleClick();
}


void ACraftingToolActor::OnFingerPressedTool(ETouchIndex::Type FingerIndex, UPrimitiveComponent* TouchedComponent)
{
	HandleClick();
}