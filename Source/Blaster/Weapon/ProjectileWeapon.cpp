// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/ProjectileWeapon.h"
#include "Projectile.h"

#include "Engine/SkeletalMeshSocket.h"



void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	if (!HasAuthority()) return;
	
	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	if (!InstigatorPawn)
		return;

	// spawn projectile
	auto MuzzleFlashSocket = WeaponMesh->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket)
	{
		auto SocketTransform = MuzzleFlashSocket->GetSocketTransform(WeaponMesh);
		FVector ToTareget = HitTarget - SocketTransform.GetLocation();
		FRotator TargetRotation = ToTareget.Rotation();

		if (ProjectileClass)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = GetOwner();
			SpawnParams.Instigator = InstigatorPawn;
			auto Projectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(),
			                                                      TargetRotation, SpawnParams);
		}
	}
}
