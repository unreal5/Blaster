// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Casing.generated.h"

UCLASS()
class BLASTER_API ACasing : public AActor
{
	GENERATED_BODY()

public:
	ACasing();

protected:
	virtual void BeginPlay() override;
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                   FVector NormalImpulse, const FHitResult& Hit);

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Casing Properties", meta = (AllowPrivateAccess = "true"))
	float ShellEjectionImpulse = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Casing Properties", meta = (AllowPrivateAccess = "true"))
	USoundCue* ShellEjectionSound;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* CasingMesh;
};
