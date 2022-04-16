// Fill out your copyright notice in the Description page of Project Settings.


#include "PersistentDataComponent.h"
//#include "..\Public\PersistentDataComponent.h"
#include "MixMatch/MixMatch.h"
#include "MMPlayerController.h"
#include "PlayerSave.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "GameFramework/SaveGame.h"

const FString UPersistentDataComponent::LocalPlayerFilenameSuffix = FString(TEXT("_player"));
//const FString UTRPersistentDataComponent::RemotePlayerFilenameSuffix = FString(TEXT("_rmtplr"));

// Sets default values for this component's properties
UPersistentDataComponent::UPersistentDataComponent()
	: Super()
{
	SetIsReplicatedByDefault(false);
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
void UPersistentDataComponent::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void UPersistentDataComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}


FString UPersistentDataComponent::GetLocalPlayerSaveFilename(const FGuid& PlayerGuid)
{
	return PlayerGuid.ToString(EGuidFormats::Digits) + LocalPlayerFilenameSuffix;
}


FString UPersistentDataComponent::GetPlayerSaveFilename()
{ 
	AMMPlayerController* MMPlayerController = Cast<AMMPlayerController>(GetOwner());
	if (MMPlayerController) {
		return GetLocalPlayerSaveFilename(MMPlayerController->PlayerGuid);
	}
	UE_LOG(LogMMGame, Error, TEXT("GetPlayerSaveFilename - Could not get player controller."));
	return FString();
}


TArray<FString> UPersistentDataComponent::GetAllSaveProfileFilenames()
{
	//////////////////////////////////////////////////////////////////////////////
	class FFindSavesVisitor : public IPlatformFile::FDirectoryVisitor
	{
	public:
		FFindSavesVisitor() {}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory)
		{
			if (!bIsDirectory)
			{
				if (FilenameOrDirectory)
				{
					//UE_LOG(LogMMGame, Log, TEXT("GetAllSaveProfileNames - potential file: %s."), FilenameOrDirectory);
				}
				else
				{
					UE_LOG(LogMMGame, Error, TEXT("PersistentDataComponent GetAllSaveProfileFilenames encountered null FilenameOrDirectory."));
					return false;
				}
				FString FullFilePath(FilenameOrDirectory);
				if (FPaths::GetExtension(FullFilePath) == TEXT("sav"))
				{
					FString CleanFilename = FPaths::GetBaseFilename(FullFilePath);
					if (CleanFilename.EndsWith(*UPersistentDataComponent::LocalPlayerFilenameSuffix))
					{
						SavesFound.Add(CleanFilename);
						//UE_LOG(LogMMGame, Log, TEXT("GetAllSaveProfileNames - found file: %s."), *CleanFilename);
					}
				}
			}
			return true;
		}
		TArray<FString> SavesFound;
	};
	//////////////////////////////////////////////////////////////////////////////

	const FString SavesFolder = FPaths::ProjectSavedDir() + TEXT("SaveGames");
	//UE_LOG(LogMMGame, Log, TEXT("GetAllSaveProfileNames - checking dir: %s."), *SavesFolder)
	if (!SavesFolder.IsEmpty())
	{
		FFindSavesVisitor Visitor;
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*SavesFolder, Visitor);
		return Visitor.SavesFound;
	}
	return TArray<FString>();
}

TArray<FPlayerSaveData> UPersistentDataComponent::GetAllSaveProfileData()
{
	TArray<FPlayerSaveData> AllFoundData;
	TArray<FString> AllFilenames = GetAllSaveProfileFilenames();
	for (FString TmpFilename : AllFilenames)
	{
		if (UGameplayStatics::DoesSaveGameExist(TmpFilename, 0))
		{
			UPlayerSave* SaveGame = Cast<UPlayerSave>(UGameplayStatics::LoadGameFromSlot(TmpFilename, 0));
			if (SaveGame)
			{
				AllFoundData.Add(SaveGame->PlayerSaveData);
			}
		}
	}
	return AllFoundData;
}


void UPersistentDataComponent::ServerSavePlayerData()
{
	AMMPlayerController* MMPlayerController = Cast<AMMPlayerController>(GetOwner());
	if (MMPlayerController)
	{
		// Create a new save each time
		UPlayerSave* SaveGame = Cast<UPlayerSave>(UGameplayStatics::CreateSaveGameObject(UPlayerSave::StaticClass()));
		// Get the data from our owning controller.
		MMPlayerController->GetPlayerSaveData(SaveGame->PlayerSaveData);
		//UE_LOG(LogMMGame, Log, TEXT("ServerSavePlayerData - Saving player data. Guid: %s"), *SaveGame->PlayerSaveData.PlayerGuid.ToString(EGuidFormats::Digits));
		// Begin debug inventory logging
		/*if (SaveGame->PlayerSaveData.GoodsInventory.Num() == 0)
		{
			UE_LOG(LogMMGame, Log, TEXT("    GoodsInventory is empty."));
		}
		else
		{
			UE_LOG(LogMMGame, Log, TEXT("    Goods Inventory: "));
			for (FGoodsQuantity InvElem : SaveGame->PlayerSaveData.GoodsInventory)
			{
				UE_LOG(LogMMGame, Log, TEXT("     %s: %f"), *InvElem.Name.ToString(), InvElem.Quantity);
			}
		}*/
		// End debug inventory logging 
		if (UGameplayStatics::SaveGameToSlot(SaveGame, GetPlayerSaveFilename(), 0))
		{
			UE_LOG(LogMMGame, Log, TEXT("ServerSavePlayerData - Player data for Guid %s saved to: %s"), *SaveGame->PlayerSaveData.PlayerGuid.ToString(EGuidFormats::Digits), *GetPlayerSaveFilename());
		}
		else
		{
			UE_LOG(LogMMGame, Error, TEXT("PersistentDataComponent ServerSavePlayerData : Error saving player data: %s"), *GetPlayerSaveFilename());
		}
	}
	else {
		UE_LOG(LogMMGame, Error, TEXT("ServerSavePlayerData - Could not get player controller."));
	}
}


void UPersistentDataComponent::ServerLoadPlayerData()
{
	AMMPlayerController* MMPlayerController = Cast<AMMPlayerController>(GetOwner());
	if (MMPlayerController)
	{
		//ATRPlayerState* TRPlayerState = Cast<ATRPlayerState>(TRPlayerController->PlayerState);
		//if (TRPlayerState)
		//{
		//    if (TRPlayerState->PlayerGuid.IsValid()) {}
		//}
		if (MMPlayerController->PlayerGuid.IsValid())
		{
			ServerLoadPlayerDataByGuid(MMPlayerController->PlayerGuid);
		}
		else
		{
			UE_LOG(LogMMGame, Error, TEXT("PersistentDataComponent::ServerLoadPlayerData called when PlayerController.PlayerGuid is not valid."));
		}
	}
}


void UPersistentDataComponent::ServerLoadPlayerDataByGuid(const FGuid ForcePlayerGuid)
{
	AMMPlayerController* MMPlayerController = Cast<AMMPlayerController>(GetOwner());
	if (MMPlayerController)
	{
		if (ForcePlayerGuid.IsValid())
		{
			MMPlayerController->PlayerGuid = ForcePlayerGuid;
		}
		FString PlayerSaveFilename = GetPlayerSaveFilename();
		if (!PlayerSaveFilename.IsEmpty())
		{
			if (UGameplayStatics::DoesSaveGameExist(PlayerSaveFilename, 0))
			{
				UPlayerSave* SaveGame = Cast<UPlayerSave>(UGameplayStatics::LoadGameFromSlot(PlayerSaveFilename, 0));
				if (SaveGame)
				{
					if (ForcePlayerGuid.IsValid() && SaveGame->PlayerSaveData.PlayerGuid.IsValid() && SaveGame->PlayerSaveData.PlayerGuid != ForcePlayerGuid) 
					{
						UE_LOG(LogMMGame, Warning, TEXT("Player guid in file: %s does not match player guid %s."), *SaveGame->PlayerSaveData.PlayerGuid.ToString(EGuidFormats::Digits), *ForcePlayerGuid.ToString(EGuidFormats::Digits));
					}
					else
					{
						// Update data
						MMPlayerController->UpdateFromPlayerSaveData(SaveGame->PlayerSaveData);
						//if (MMPlayerController->IsLocalController()) 
						//{
						//}
						//else
						//{
						//	//UE_LOG(LogMMGame, Log, TEXT("PersistentDataComponent ServerLoadPlayerData going to echo existing to client with ClientEchoLoadPlayerData"));
						//	// Call client to update data
						//	ClientEchoLoadPlayerData(SaveGame->PlayerSaveData);
						//}
						UE_LOG(LogMMGame, Log, TEXT("ServerLoadPlayerData - loaded player data for guid %s from: %s"), *SaveGame->PlayerSaveData.PlayerGuid.ToString(EGuidFormats::Digits), *PlayerSaveFilename);
					}						
				}
				else {
					UE_LOG(LogMMGame, Error, TEXT("ServerLoadPlayerData - error loading save file: %s"), *PlayerSaveFilename);
				}
			}
			else 
			{
				// No existing file, setup guids and init data.
				FPlayerSaveData NewSaveData;
				if (!MMPlayerController->PlayerGuid.IsValid()) 
				{ 
					if (ForcePlayerGuid.IsValid()) { MMPlayerController->PlayerGuid = ForcePlayerGuid; }
					else { MMPlayerController->PlayerGuid = FGuid::NewGuid(); }
				}					
				//UE_LOG(LogMMGame, Log, TEXT("ServerLoadPlayerData - no save file found, setting new player guid: %s."), *MMPlayerController->PlayerGuid.ToString(EGuidFormats::Digits));
				//if (MMPlayerController->IsLocalController())
				//{
				//}
				//else
				//{
				//	//UE_LOG(LogMMGame, Log, TEXT("PersistentDataComponent ServerLoadPlayerData going to echo new to client with ClientEchoLoadPlayerData"));
				//	// Call client to update data
				//	MMPlayerController->GetPlayerSaveData(NewSaveData);
				//	ClientEchoLoadPlayerData(NewSaveData);
				//}
			}
			OnPlayerDataLoaded.Broadcast();
		}
		else
		{
			UE_LOG(LogMMGame, Error, TEXT("ServerLoadPlayerData - no profile name set."));
		}
		
	}
	else {
		UE_LOG(LogMMGame, Error, TEXT("ServerLoadPlayerData - Could not get player controller."));
	}
}


//void UPersistentDataComponent::ClientEchoLoadPlayerData_Implementation(const FPlayerSaveData PlayerSaveData)
//{
//	// Get parent PlayerController
//	AMMPlayerController* MMPlayerController = Cast<AMMPlayerControllerBase>(GetOwner());
//	if (MMPlayerController)
//	{
//		//UE_LOG(LogMMGame, Log, TEXT("PersistentDataComponent ClientEchoLoadPlayerData - update player data for: %s."), *PlayerSaveData.PlayerGuid.ToString());
//		MMPlayerController->UpdateFromPlayerSaveData(PlayerSaveData);		
//	}
//	OnPlayerDataLoaded.Broadcast();
//}


//bool UTRPersistentDataComponent::ClientEchoLoadPlayerData_Validate(const FPlayerSaveData PlayerSaveData)
//{
//	return true;
//}

