// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponent/CombatComponent.h"

#include "Character/BlasterCharacter.h"

#include "Engine/SkeletalMeshSocket.h"

#include "Weapon/Weapon.h"


UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	check((void*)GetOwner() == Character.Get());
}


void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                     FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (!Character.IsValid()) return;
	if (!WeaponToEquip) return;

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	
	auto SkeletalMesh = Character->GetMesh();
	if (!SkeletalMesh) return;
	auto RightHandSocket = SkeletalMesh->GetSocketByName("RightHandSocket");
	if (RightHandSocket)
	{
		RightHandSocket->AttachActor(EquippedWeapon.Get(), SkeletalMesh);
	}
	EquippedWeapon->SetOwner(Character.Get());
	EquippedWeapon->ShowPickupWidget(false);
}
