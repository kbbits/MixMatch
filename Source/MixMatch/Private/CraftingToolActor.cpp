
#include "CraftingToolActor.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "MMPlayerController.h"
#include "RecipeManagerComponent.h"



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


void ACraftingToolActor::SetRecipe(const FName& RecipeName)
{
	CurrentRecipe = GetRecipeManager()->GetRecipe(RecipeName);
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
	if (IsValid(CurrentGrid)) 
	{
		CurrentGrid->DestroyGrid();
		CurrentGrid = nullptr;
	}
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;
	TSubclassOf<AMMPlayGrid> LoadedGridClass = GridClass.LoadSynchronous();
	AMMPlayGrid* NewGrid = GetWorld()->SpawnActor<AMMPlayGrid>(
		LoadedGridClass,
		FVector::ZeroVector,
		GetActorRotation(),
		SpawnParams
	);
	check(NewGrid);
	CurrentGrid = NewGrid;
}


void ACraftingToolActor::DestroyGrid()
{
	// TODO: Implement
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