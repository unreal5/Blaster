// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponent/CombatComponent.h"

#include "Character/BlasterCharacter.h"

#include "Components/SphereComponent.h"

#include "Engine/SkeletalMeshSocket.h"

#include "GameFramework/CharacterMovementComponent.h"

#include "Net/UnrealNetwork.h"

#include "Weapon/Weapon.h"


UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	//IMPORTANT! 必须设置为true，否则组件将不调用InitializeComponent
	bWantsInitializeComponent = true;
}

void UCombatComponent::InitializeComponent()
{
	Super::InitializeComponent();
	Character = Cast<ABlasterCharacter>(GetOwner());
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	//check((void*)GetOwner() == Character.Get());
	if (Character.IsValid())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	}
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
		RightHandSocket->AttachActor(EquippedWeapon, SkeletalMesh);
	}
	EquippedWeapon->SetOwner(Character.Get());
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, EquippedWeapon);
	DOREPLIFETIME(ThisClass, bAiming);
}

void UCombatComponent::SetAiming(bool bNewAiming)
{
	if (bNewAiming == bAiming) return;
	if (!Character.IsValid()) return;

	// 技巧：客户端立即响应（不需要等待RPC延迟）
	bAiming = bNewAiming;

	//如果位于客户端，则需要别的客户端也同步显示。
	// 进一步思考，可以略过if判断（位于服务端时，会两次设置bAiming为同一值）
	//if (!Character->HasAuthority())
	Server_SetAiming(bNewAiming);

	Character->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
}


void UCombatComponent::Server_SetAiming_Implementation(bool bNewAiming)
{
	check(Character.IsValid() && Character->HasAuthority());

	bAiming = bNewAiming;
	Character->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
}

void UCombatComponent::OnRep_EquippedWeapon(AWeapon* OldWeapon)
{
	if (EquippedWeapon && Character.IsValid())
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}
