// Copyright Epic Games, Inc. All Rights Reserved.

#include "MMPawn.h"
#include "MMBlock.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "MMPlayerController.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "MMEnums.h"

AMMPawn::AMMPawn()
	: Super()
{
	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void AMMPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AMMPlayerController* PC = GetController<AMMPlayerController>();
	// If we're in a grid or a selection is needed, trace for blocks below cursor (or HMD trace)
	if (PC && (PC->GetLastInputContext() == EMMInputContext::InPlayGrid || PC->GetLastInputContext() == EMMInputContext::EffectSelect))
	{
		if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
		{
			if (UCameraComponent* OurCamera = PC->GetViewTarget()->FindComponentByClass<UCameraComponent>())
			{
				FVector Start = OurCamera->GetComponentLocation();
				FVector End = Start + (OurCamera->GetComponentRotation().Vector() * 12000.0f);
				TraceForBlock(Start, End, true);
			}
		}
		else
		{
			FVector Start, Dir, End;
			PC->DeprojectMousePositionToWorld(Start, Dir);
			End = Start + (Dir * 12000.0f);
			TraceForBlock(Start, End, false);
		}
	}
}

void AMMPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("ResetVR", EInputEvent::IE_Pressed, this, &AMMPawn::OnResetVR);
	PlayerInputComponent->BindAction("TriggerClick", EInputEvent::IE_Pressed, this, &AMMPawn::TriggerClick);
}

void AMMPawn::CalcCamera(float DeltaTime, struct FMinimalViewInfo& OutResult)
{
	Super::CalcCamera(DeltaTime, OutResult);

	if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled()){
		OutResult.Rotation = FRotator(-90.0f, -90.0f, 0.0f);
	}
}

void AMMPawn::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AMMPawn::TriggerClick()
{
	if (CurrentBlockFocus){
		CurrentBlockFocus->HandleClicked();
	}
}

void AMMPawn::TraceForBlock(const FVector& Start, const FVector& End, bool bDrawDebugHelpers)
{
	AMMPlayerController* PC = GetController<AMMPlayerController>();
	if (PC && (PC->GetLastInputContext() == EMMInputContext::InPlayGrid || PC->GetLastInputContext() == EMMInputContext::EffectSelect))
	{
		FHitResult HitResult;
		GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility);
		if (bDrawDebugHelpers)
		{
			DrawDebugLine(GetWorld(), Start, HitResult.Location, FColor::Red);
			DrawDebugSolidBox(GetWorld(), HitResult.Location, FVector(20.0f), FColor::Red);
		}
		if (HitResult.Actor.IsValid())
		{
			AMMBlock* HitBlock = Cast<AMMBlock>(HitResult.Actor.Get());
			if (CurrentBlockFocus != HitBlock)
			{
				if (IsValid(CurrentBlockFocus))
				{
					CurrentBlockFocus->Highlight(false);
					PC->GridCellUnhovered(CurrentBlockFocus->GetCoords());
				}
				if (IsValid(HitBlock))
				{
					HitBlock->Highlight(true);
					PC->GridCellHovered(HitBlock->GetCoords());
				}
				CurrentBlockFocus = HitBlock;
			}
		}
		else if (CurrentBlockFocus)
		{
			if (IsValid(CurrentBlockFocus)) 
			{
				CurrentBlockFocus->Highlight(false);
				PC->GridCellUnhovered(CurrentBlockFocus->GetCoords());
			}
			CurrentBlockFocus = nullptr;
		}
	}
}