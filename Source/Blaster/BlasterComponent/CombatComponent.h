// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class ABlasterHUD;
class ABlasterPlayerController;
const float TRACE_LENGTH = 80000.0f;
class AWeapon;
class ABlasterCharacter;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()
	//friend class ABlasterCharacter;

public:
	UCombatComponent();
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	void EquipWeapon(AWeapon* WeaponToEquip);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetAiming(bool bNewAiming);

	void FireButtonPressed(bool bPressed);

protected:
	UFUNCTION(Server, Reliable)
	void Server_Fire(const FVector_NetQuantize& TraceHitTarget);

	void SetHudCrosshair(float DeltaTime);

private:
	TWeakObjectPtr<ABlasterCharacter> Character;
	TWeakObjectPtr<ABlasterPlayerController> BlasterPlayerController;
	TWeakObjectPtr<ABlasterHUD> BlasterHUD;

	UFUNCTION()
	void OnRep_EquippedWeapon(AWeapon* OldWeapon);

	UPROPERTY(ReplicatedUsing=OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;

	UPROPERTY(Replicated)
	bool bAiming = false;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed = 600.0f;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed = 450.0f;

	UFUNCTION(Server, Reliable)
	void Server_SetAiming(bool bNewAiming);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Fire(const FVector_NetQuantize& TraceHitTarget);

	bool bFireButtonPressed = false;

	void TraceUnderCrosshair(FHitResult& TraceHitResult);

	/* Hud and crosshair */
	float CrosshairVelocityFactor = 0.f;
	float CrosshairInAirFactor = 0.f;

public:
	AWeapon* GetEquippedWeapon() const { return EquippedWeapon; }
	bool IsAiming() const { return bAiming; }
};
