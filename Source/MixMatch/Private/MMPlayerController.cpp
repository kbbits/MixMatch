// Copyright Epic Games, Inc. All Rights Reserved.

#include "MMPlayerController.h"
//#include "SimpleNamedTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Goods/GoodsFunctionLibrary.h"
#include "../MixMatch.h"
#include "MMGameMode.h"
#include "CraftingToolActor.h"
#include "ActionBarItem.h"
#include "Goods/UsableGoodsContext.h"


AMMPlayerController::AMMPlayerController()
	: Super()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableTouchEvents = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
	LoadedTotalPlaytime = 0.0f;
	TimeAtLastSave = 0.0f;

	PersistentDataComponent = CreateDefaultSubobject<UPersistentDataComponent>(TEXT("PersistentDataComponent"));
	if (PersistentDataComponent) {
		AddOwnedComponent(PersistentDataComponent);
		//PersistentDataComponent->SetIsReplicated(true);
	}
	else { UE_LOG(LogMMGame, Error, TEXT("PlayerController constructor failed to create PersistentDataComponent")); }

	GoodsInventory = CreateDefaultSubobject<UInventoryActorComponent>(TEXT("GoodsInventory"));
	if (GoodsInventory) {
		AddOwnedComponent(GoodsInventory);
	}

	RecipeManager = CreateDefaultSubobject<URecipeManagerComponent>(TEXT("RecipeManager"));
	if (RecipeManager) {
		AddOwnedComponent(RecipeManager);
	}
}


bool AMMPlayerController::GetPlayerSaveData_Implementation(FPlayerSaveData& SaveData)
{
	// Copy empty default struct in first in case this save already has data.
	SaveData = FPlayerSaveData();
	if (!PlayerGuid.IsValid()) {
		PlayerGuid = FGuid::NewGuid();
	}
	SaveData.PlayerGuid = PlayerGuid;
	SaveData.ProfileName = FName(PlayerDisplayName.ToString());
	SaveData.DisplayName = PlayerDisplayName;
	SaveData.TotalPlaytime = LoadedTotalPlaytime + (UGameplayStatics::GetTimeSeconds(this) - TimeAtLastSave);
	SaveData.ActionBarSize = GetActionBarSize();
	SaveData.ActionBarItemNames.Empty(ActionBarSize);
	for (int32 i = 0; i < ActionBarSize; i++) 
	{
		UActionBarItem* Item = GetActionBarItem(i);
		if (Item) {
			SaveData.ActionBarItemNames.Add(Item->GetName());
		}
		else {
			SaveData.ActionBarItemNames.Add(NAME_None);
		}
	}	
	TimeAtLastSave = UGameplayStatics::GetTimeSeconds(this);
	
	// Inventory
	GoodsInventory->GetSaveableGoods(SaveData.GoodsInventory);
	GoodsInventory->GetSnapshotGoods(SaveData.SnapshotInventory);
	// Other data
	if (RecipeManager) 
	{
		RecipeManager->GetSaveData(SaveData);
	}
	else 
	{
		UE_LOG(LogMMGame, Error, TEXT("PlayerController GetPlayerSaveData found null RecipeManager."));
		return false;
	}
	return true;
}


bool AMMPlayerController::UpdateFromPlayerSaveData_Implementation(const FPlayerSaveData& SaveData)
{
	PlayerGuid = SaveData.PlayerGuid;
	PlayerDisplayName = SaveData.DisplayName;
	LoadedTotalPlaytime = SaveData.TotalPlaytime;
	TimeAtLastSave = UGameplayStatics::GetTimeSeconds(this);
	// Inventory
	GoodsInventory->ServerSetInventory(SaveData.GoodsInventory, SaveData.SnapshotInventory);
	SetActionBarSize(SaveData.ActionBarSize);
	// Set action bar items
	for (int32 i = 0; i < SaveData.ActionBarItemNames.Num(); i++) {
		SetActionBarItemByName(SaveData.ActionBarItemNames[i], i);
	}
	// Clear empty action bar slots
	for (int32 i = SaveData.ActionBarItemNames.Num(); i < ActionBarSize; i++) {
		ClearActionBarItem(i);
	}
	// Other data
	if (RecipeManager)
	{
		RecipeManager->UpdateFromSaveData(SaveData);
	}
	return true;
}


float AMMPlayerController::GetBlockMoveSpeedMultiplier_Implementation()
{
	return 1.f;
}


void AMMPlayerController::SetCurrentGrid(AMMPlayGrid* NewCurrentGrid)
{
	if (IsValid(NewCurrentGrid)) 
	{
		CurrentGrid = NewCurrentGrid;
		OnCurrentGridChanged.Broadcast(CurrentGrid);
	}
}


void AMMPlayerController::ClearCurrentGrid()
{
	CurrentGrid = nullptr;
	OnCurrentGridChanged.Broadcast(nullptr);
}


AMMPlayGrid* AMMPlayerController::GetCurrentGrid()
{
	return CurrentGrid;
}


AMMPlayGrid* AMMPlayerController::GetCurrentGridValid(bool& bIsValid)
{
	if (IsValid(GetCurrentGrid())) {
		bIsValid = true;
	} 
	else {
		bIsValid = false;
	}
	return GetCurrentGrid();
}


void AMMPlayerController::GridCellHovered_Implementation(const FIntPoint HoveredCoords)
{
	if (GetLastInputContext() == EMMInputContext::EffectSelect && CurrentActionBarItemIndex >= 0)
	{
		if (GetCurrentGrid() == nullptr) { return; }
		UActionBarItem* ActionBarItem = GetActionBarItem(CurrentActionBarItemIndex);
		if (ActionBarItem == nullptr || ActionBarItem->UsableGoodsContext == nullptr) {
			return;
		}
		ActionBarItem->UsableGoodsContext->SelectedCoords.Empty(1);
		ActionBarItem->UsableGoodsContext->SelectedCoords.Add(HoveredCoords);
		GetCurrentGrid()->PreviewUsableGoodsSelection(ActionBarItem->UsableGoodsContext);
	}
}


void AMMPlayerController::GridCellUnhovered_Implementation(const FIntPoint UnhoveredCoords)
{
	if (GetLastInputContext() == EMMInputContext::EffectSelect && CurrentActionBarItemIndex >= 0)
	{
		GetCurrentGrid()->ClearUsableGoodsPreviews();
	}
}


void AMMPlayerController::CollectGoods(const TArray<FGoodsQuantity> CollectedGoods)
{
	GoodsInventory->AddSubtractGoodsArray(CollectedGoods, false, true);
	UGoodsFunctionLibrary::AddToGoodsQuantities(TotalGoodsCollected, CollectedGoods);
	OnGoodsCollected.Broadcast(CollectedGoods);
}


bool AMMPlayerController::CraftRecipe(const FCraftingRecipe& Recipe)
{
	if (GoodsInventory->AddSubtractGoodsArray(Recipe.CraftingInputs, true, true))
	{
		TArray<FGoodsQuantity> CraftedGoods;
		if (RecipeManager->GetGoodsForRecipe(Recipe, CraftedGoods))
		{
			GoodsInventory->AddSubtractGoodsArray(CraftedGoods, false, true);
			RecipeManager->IncrementRecipeCraftingCount(Recipe.Name);
			OnRecipeCrafted.Broadcast(Recipe, 1);
			return true;
		}
	}
	return false;
}


int32 AMMPlayerController::GetRecipeLevel(const FName RecipeName)
{
	return RecipeManager->GetRecipeLevel(RecipeName);
}


void AMMPlayerController::SetRecipeLevel(const FName RecipeName, const int32 NewLevel)
{
	RecipeManager->SetRecipeLevel(RecipeName, NewLevel);
}


int32 AMMPlayerController::GetActionBarSize()
{
	return ActionBarSize;
}


void AMMPlayerController::SetActionBarSize(const int32 NewSize)
{
	AMMGameMode* GameMode = Cast<AMMGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode == nullptr) { return; }
	int32 ValidNewSize = NewSize < 0 ? 0 : (NewSize > GameMode->MaxActionBarSize ? GameMode->MaxActionBarSize : NewSize);
	// Clear out the items that won't fit in the new size.
	for (int32 i = ActionBarItems.Num() - 1; i > ValidNewSize - 1; i--)	{
		ClearActionBarItem(i, false);
	}
	ActionBarSize = ValidNewSize;
	ActionBarItems.SetNum(ActionBarSize, true);
	OnActionBarChanged.Broadcast();
}


UActionBarItem* AMMPlayerController::GetActionBarItem(const int32 SlotIndex)
{
	if (SlotIndex >= ActionBarSize || !ActionBarItems.IsValidIndex(SlotIndex)) {
		return nullptr;
	}
	return ActionBarItems[SlotIndex];
}


bool AMMPlayerController::SetActionBarItemByName(const FName& UsableGoodsName, const int32 SlotIndex)
{
	if (SlotIndex >= ActionBarSize) { return false; }
	// Sanity check, make sure action bar has been sized according to ActionBarSize
	if (SlotIndex >= ActionBarItems.Num()) {
		SetActionBarSize(ActionBarSize);
	}
	bool bFound = false;
	UUsableGoodsContext* UsableGoodsContext = nullptr;
	UUsableGoods* UsableGoods = nullptr;
	if (UsableGoodsName != NAME_None) 
	{
		AMMGameMode* GameMode = Cast<AMMGameMode>(UGameplayStatics::GetGameMode(this));
		if (GameMode == nullptr) { return false; }
		//GoodsInventory->GetGoodsCount(UsableGoodsName);
		UsableGoods = GameMode->GetUsableGoods(UsableGoodsName, bFound);
	}
	if (bFound && UsableGoods && UsableGoods->IsUsable())
	{
		// Clear any current item at the index, don't fire events.
		ClearActionBarItem(SlotIndex, false);
		// Create the usable goods context
		UsableGoodsContext = NewObject<UUsableGoodsContext>(this, UUsableGoodsContext::StaticClass());
		if (UsableGoodsContext)
		{
			UsableGoodsContext->SetUsableGoods(UsableGoods);
			// Create the new ActionBarItem
			UActionBarItem* Item = NewObject<UActionBarItem>(this, UActionBarItem::StaticClass());
			if (Item)
			{
				// Init its properties and add it to the action bar items array
				Item->UsableGoodsContext = UsableGoodsContext;
				ActionBarItems[SlotIndex] = Item;
				// Fire notification
				OnActionBarItemChanged.Broadcast(SlotIndex, Item);
				return true;
			}
		}
	}
	else 
	{
		// No usable goods found, so just clear the slot, let it fire events.
		ClearActionBarItem(SlotIndex);
	}
	return false;
}


void AMMPlayerController::UseActionBarItem_Implementation(const int32 SlotIndex, const FIntPoint& SelectedCoords)
{
	if (CurrentActionBarItemIndex >= 0 && CurrentActionBarItemIndex != SlotIndex) {
		UE_LOG(LogMMGame, Warning, TEXT("MMPlayerController::UseActionBarItem - Cannot use item %d. A different action bar item already in use: %d."), SlotIndex, CurrentActionBarItemIndex);
		return;
	}
	UActionBarItem* ActionBarItem = GetActionBarItem(SlotIndex);
	if (ActionBarItem == nullptr) {
		UE_LOG(LogMMGame, Warning, TEXT("MMPlayerController:UseActionBarItem - No action bar item at index %d"), SlotIndex);
		return;
	}
	if (ActionBarItem->UsableGoodsContext == nullptr) { 
		UE_LOG(LogMMGame, Warning, TEXT("MMPlayerController:UseActionBarItem - ActionBarItem %s has null UsableGoodsContext"), *ActionBarItem->GetName().ToString());
		return; 
	}
	if (ActionBarItem->GetName() == NAME_None) { 
		UE_LOG(LogMMGame, Warning, TEXT("MMPlayerController:UseActionBarItem - ActionBarItem name = NAME_None"));
		return; 
	}
	if (GoodsInventory->GetGoodsCount(ActionBarItem->GetName()) < 1) { 
		UE_LOG(LogMMGame, Warning, TEXT("MMPlayerController:UseActionBarItem - Player does not have inventory to use item %s"), *ActionBarItem->GetName().ToString());
		return; 
	}
	if (ActionBarItem->UsableGoodsContext->RequiresSelection() && (SelectedCoords == FIntPoint::NoneValue || SelectedCoords.GetMin() < 0)) 
	{
		UE_LOG(LogMMGame, Log, TEXT("MMPlayerController:UseActionBarItem - Item %s needs selection."), *ActionBarItem->GetName().ToString());
		GetSelectionForActionBarItem(SlotIndex);
		return;
	}
	UUsableGoods* UsableGoods = ActionBarItem->UsableGoodsContext->GetUsableGoods();
	TArray<UGameEffect*> GameEffects;
	if (UsableGoods == nullptr) { 
		UE_LOG(LogMMGame, Warning, TEXT("MMPlayerController:UseActionBarItem - No UsableGoods instance for ActionBarItem %s"), *ActionBarItem->GetName().ToString());
		return; 
	}	
	ActionBarItem->UsableGoodsContext->GetGameEffects(GameEffects);
	if (GoodsInventory->AddSubtractGoods(FGoodsQuantity(ActionBarItem->GetName(), -1.0f), false, true))
	{
		UE_LOG(LogMMGame, Log, TEXT("MMPlayerController:UseActionBarItem - Using action bar item %s at coords %s"), *ActionBarItem->GetName().ToString(), *SelectedCoords.ToString());
		AMMGameMode* GameMode = Cast<AMMGameMode>(UGameplayStatics::GetGameMode(this));
		if (GameMode == nullptr) { return; }
		CurrentActionBarItemIndex = SlotIndex;
		TArray<FIntPoint> CoordsArray;
		CoordsArray.Add(SelectedCoords);
		for (UGameEffect* GameEffect : GameEffects)	{
			UE_LOG(LogMMGame, Log, TEXT("    Applying game effect %s"), *GameEffect->GetClass()->GetName());
			GameMode->AddGameEffect(GameEffect, CoordsArray);
		}
	}
	else {
		UE_LOG(LogMMGame, Warning, TEXT("MMPlayerController:UseActionBarItem - No inventory to use action bar item %s."), *ActionBarItem->GetName().ToString());
	}
	CurrentActionBarItemIndex = -1;
}


void AMMPlayerController::GetSelectionForActionBarItem_Implementation(const int32 SlotIndex)
{
	if (CurrentActionBarItemIndex >= 0 && CurrentActionBarItemIndex != SlotIndex) { 
		CancelActionBarSelection();
	}
	CurrentActionBarItemIndex = SlotIndex;
	UActionBarItem* ActionBarItem = GetActionBarItem(SlotIndex);
	if (ActionBarItem)
	{
		UE_LOG(LogMMGame, Log, TEXT("MMPlayerController:GetSelectionForActionBarItem - getting selection for %s."), *ActionBarItem->GetName().ToString());
		// Add the EffectSelect input context
		AddInputContext(EMMInputContext::EffectSelect);
		// then  "wait" for user to make selection. Blocks/Grid handle the EffectSelect context and call ActionBarSelectionFinished()
	}
	else {
		UE_LOG(LogMMGame, Warning, TEXT("MMPlayerController::GetSelectionForActionBarItem - No action bar item at index %d"), SlotIndex);
	}
}


void AMMPlayerController::ActionBarSelectionFinished_Implementation(const FIntPoint& SelectedCoords)
{
	RemoveInputContext(EMMInputContext::EffectSelect);
	GetCurrentGrid()->ClearUsableGoodsPreviews();
	if (CurrentActionBarItemIndex < 0) { 
		UE_LOG(LogMMGame, Warning, TEXT("MMPlayerController:ActionBarSelectionFinished - No current action bar item for selection."));
		return; 
	}
	UE_LOG(LogMMGame, Log, TEXT("MMPlayerController:ActionBarSelectionFinished - Finished selection for item index %d"), CurrentActionBarItemIndex);
	UseActionBarItem(CurrentActionBarItemIndex, SelectedCoords);
	CurrentActionBarItemIndex = -1;
}


void AMMPlayerController::CancelActionBarSelection_Implementation()
{
	RemoveInputContext(EMMInputContext::EffectSelect);
	GetCurrentGrid()->ClearUsableGoodsPreviews();
	CurrentActionBarItemIndex = -1;
}


void AMMPlayerController::ClearActionBarItem(const int32 SlotIndex, const bool bFireEvents)
{
	UActionBarItem* Item = nullptr;
	if (CurrentActionBarItemIndex == SlotIndex) {
		CancelActionBarSelection();
	}
	if (ActionBarItems.IsValidIndex(SlotIndex)) 
	{
		Item = ActionBarItems[SlotIndex];
		if (Item) 
		{
			// Have the item cleanup after itself
			Item->Cleanup();
			ActionBarItems[SlotIndex] = nullptr;
			if (bFireEvents) {
				OnActionBarItemChanged.Broadcast(SlotIndex, nullptr);
			}
		}
	}
}


void AMMPlayerController::HandleToolClicked(ACraftingToolActor* ClickedTool)
{
	if (GetLastInputContext() != EMMInputContext::None)
	{
		return;
	}
	if (IsValid(CurrentTool) && CurrentTool != ClickedTool)
	{
		CurrentTool->DestroyGrid();
		CurrentTool = nullptr;
	}
	CurrentTool = ClickedTool;
	ClearCurrentGrid();
	if (IsValid(CurrentTool))
	{
		CurrentTool->SpawnGrid();
		CurrentTool->SpawnGridBackground();
		SetCurrentGrid(CurrentTool->CurrentGrid);
	}
	AddInputContext(EMMInputContext::InPlayGrid);
	OnPrePlayGrid();
}


void AMMPlayerController::OnPrePlayGrid_Implementation()
{
	OnPlayGrid();
}


void AMMPlayerController::OnPlayGrid_Implementation()
{
	if (GetCurrentGrid())
	{
		GetCurrentGrid()->StartPlayGrid();
		OnPlayGridStarted.Broadcast(GetCurrentGrid());
	}
}


void AMMPlayerController::OnStopGrid_Implementation()
{
	if (GetCurrentGrid())
	{
		GetCurrentGrid()->StopPlayGrid();
		OnPlayGridStopped.Broadcast(GetCurrentGrid());
		if (CurrentTool) 
		{
			CurrentTool->DestroyGrid();
			CurrentTool = nullptr;
		}
		else {
			GetCurrentGrid()->DestroyGrid();
		}		
		ClearCurrentGrid();
		RemoveInputContext(EMMInputContext::InPlayGrid);
	}	
}


void AMMPlayerController::AddInputContext(const EMMInputContext NewContext)
{
	InputContextStack.Push(NewContext);
}


void AMMPlayerController::RemoveInputContext(const EMMInputContext RemoveContext, const bool bRemoveAll)
{
	if (bRemoveAll)	{
		InputContextStack.Remove(RemoveContext);
	}
	else {
		int32 Index;
		InputContextStack.FindLast(RemoveContext, Index);
		if (Index >= 0) {
			InputContextStack.RemoveAt(Index);
		}
	}
}


void AMMPlayerController::ClearInputContextStack()
{
	InputContextStack.Empty();
}


EMMInputContext AMMPlayerController::GetLastInputContext()
{
	if (InputContextStack.Num() > 0) {
		return InputContextStack.Last();
	}
	else {
		return EMMInputContext::None;
	}
}


bool AMMPlayerController::HasInputContext(const EMMInputContext CheckContext)
{
	return InputContextStack.Contains(CheckContext);
}