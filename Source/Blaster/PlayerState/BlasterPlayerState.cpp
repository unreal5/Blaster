// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerState/BlasterPlayerState.h"

#include "Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
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

void ABlasterPlayerState::AddToDefeats(int32 DefeatsAmount)
{
	Defeats += DefeatsAmount;

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
		PlayerController->SetHUDDefeats(Defeats);
	}
}

void ABlasterPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABlasterPlayerState, Defeats);
}

void ABlasterPlayerState::OnRep_Defeats()
{
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
		PlayerController->SetHUDDefeats(Defeats);
	}
}
