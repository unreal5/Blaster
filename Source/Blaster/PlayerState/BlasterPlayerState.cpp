// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerState/BlasterPlayerState.h"

#include "Character/BlasterCharacter.h"
#include "PlayerController/BlasterPlayerController.h"

void ABlasterPlayerState::OnRep_Score()
{
	Super::OnRep_Score();
	if (!Character.IsValid())
	{
		Character = GetPawn<ABlasterCharacter>();
		if (Character.IsValid())
		{
			PlayerController = Character->GetController<ABlasterPlayerController>();
		}
	}
	if (PlayerController.IsValid())
	{
		PlayerController->SetHUDScore(GetScore());
	}
}

void ABlasterPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount);
	if (!Character.IsValid())
	{
		Character = GetPawn<ABlasterCharacter>();
		if (Character.IsValid())
		{
			PlayerController = Character->GetController<ABlasterPlayerController>();
		}
	}
	if (PlayerController.IsValid())
	{
		PlayerController->SetHUDScore(GetScore());
	}
}
