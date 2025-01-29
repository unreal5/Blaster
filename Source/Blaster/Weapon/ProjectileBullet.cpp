// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileBullet.h"

#include "Kismet/GameplayStatics.h"

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{

	auto EventInstigatorController = GetInstigatorController();
	if (!EventInstigatorController)
	{
		checkf(false, TEXT("No EventInstigatorController"));
		return;
	}
	UGameplayStatics::ApplyDamage(OtherActor, Damage, EventInstigatorController, this, UDamageType::StaticClass());
	
	Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);

	
}
