// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BlasterPlayerState.generated.h"

class ABlasterPlayerController;
class ABlasterCharacter;
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerState : public APlayerState
{
	GENERATED_BODY()
public:
	virtual void OnRep_Score() override;
	void AddToScore(float ScoreAmount);
	void AddToDefeats(int32 DefeatsAmount);
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	TWeakObjectPtr<ABlasterCharacter> Character;
	TWeakObjectPtr<ABlasterPlayerController> PlayerController;

	UPROPERTY(ReplicatedUsing=OnRep_Defeats, VisibleAnywhere, BlueprintReadOnly, Category = "Player Stats")
	int32 Defeats = 0;
	UFUNCTION()
	virtual void OnRep_Defeats();
	
};
