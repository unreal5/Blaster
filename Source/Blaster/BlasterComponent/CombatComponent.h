// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "Hud/BlasterHUD.h"

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
	float CrosshairAimFactor = 0.f;
	float CrosshairShootingFactor = 0.f;
	// 准星碰撞的位置
	FVector HitTarget;
	FHUDPackage HUDPackage;
	/*
	 * Aiming and FOV
	 */

	/** Field of view when not aiming;set to camera's base FOV in beginplay*/
	float DefaultFOV = 0.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	float ZoomedFOV = 30.0f;

	float CurrentFOV = 0.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	float ZoomInterpSpeed = 20.0f;

	void InterpFOV(float DeltaTime);
	/*
	 * 自动开火
	 */
	FTimerHandle FireTimer;
	// 以下变量重构到Weapon类
	// UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	// float FireDelay =0.15f;
	//
	// UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	// bool bAutmatic = true;
	bool bCanFire = true;
	
	void StartFireTimer();
	void FireTimerFinished();
	void Fire();

public:
	AWeapon* GetEquippedWeapon() const { return EquippedWeapon; }
	bool IsAiming() const { return bAiming; }
	FORCEINLINE FVector GetHitTarget() const { return HitTarget; }
};
