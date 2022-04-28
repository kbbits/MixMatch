// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameEffect/GameEffectPreviewActor.h"
#include "MMPlayGrid.h"
#include "UObject/ConstructorHelpers.h"

AGameEffectPreviewActor::AGameEffectPreviewActor()
{
	// Create root scene component
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void AGameEffectPreviewActor::StartPreview_Implementation()
{

}

void AGameEffectPreviewActor::DestroyPreviewActor_Implementation()
{
	Destroy();
}