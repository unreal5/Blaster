// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/BlasterPlayerController.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Hud/BlasterHUD.h"
#include "Hud/CharacterOverlay.h"

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	if (!IsLocalController()) return;
	
	if (BlasterHUD && BlasterHUD->CharacterOverlay)
	{
		BlasterHUD->CharacterOverlay->HealthBar->SetPercent(Health / MaxHealth);
		BlasterHUD->CharacterOverlay->HealthText->SetText(FText::FromString(FString::Printf(TEXT("%.0f/%.0f"), Health, MaxHealth)));
	}
}

void ABlasterPlayerController::SetHUDScore(float Score)
{
	if (!IsLocalController()) return;
	
	if (BlasterHUD && BlasterHUD->CharacterOverlay)
	{
		BlasterHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(FString::Printf(TEXT("%d"), FMath::FloorToInt(Score))));
	}
}

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalPlayerController())
	{
		BlasterHUD = GetHUD<ABlasterHUD>();
	}
}
