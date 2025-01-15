// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"


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
private:
	TWeakObjectPtr<ABlasterCharacter> Character;

	UFUNCTION()
	void OnRep_EquippedWeapon(AWeapon* OldWeapon);
	
	UPROPERTY(ReplicatedUsing=OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;

	UPROPERTY(Replicated)
	bool bAiming = false;

	UFUNCTION(Server, Reliable)
	void Server_SetAiming(bool bNewAiming);

public:
	AWeapon* GetEquippedWeapon() const { return EquippedWeapon; }
	bool IsAiming() const { return bAiming; }
};



