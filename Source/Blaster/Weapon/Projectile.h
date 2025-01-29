// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class UProjectileMovementComponent;
class UBoxComponent;

UCLASS()
class BLASTER_API AProjectile : public AActor
{
	GENERATED_BODY()

public:
	AProjectile();
	virtual void BeginPlay() override;
	virtual void Destroyed() override;

protected:
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                   FVector NormalImpulse, const FHitResult& Hit);
	UPROPERTY(EditAnywhere, Category = "Projectile Properties", meta = (AllowPrivateAccess = "true"))
	float Damage = 20.0f;
private:
	UPROPERTY(EditAnywhere)
	UBoxComponent* CollisionBox;

	UPROPERTY(VisibleAnywhere)
	UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile Properties", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UParticleSystem> Tracer;

	UPROPERTY()
	UParticleSystemComponent* TracerComponent;

	UPROPERTY(EditAnywhere, Category = "Projectile Properties", meta = (AllowPrivateAccess = "true"))
	UParticleSystem* ImpactParticle;

	UPROPERTY(EditAnywhere, Category = "Projectile Properties", meta = (AllowPrivateAccess = "true"))
	USoundCue* ImpactSound;


};
