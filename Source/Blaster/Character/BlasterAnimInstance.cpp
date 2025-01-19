// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BlasterAnimInstance.h"

#include "BlasterCharacter.h"
#include "KismetAnimationLibrary.h"

#include "GameFramework/CharacterMovementComponent.h"

#include "Kismet/KismetMathLibrary.h"

#include "Weapon/Weapon.h"

void UBlasterAnimInstance::NativeInitializeAnimation()
{
	BlasterCharacter = Cast<ABlasterCharacter>(GetOwningActor());
}

void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	if (BlasterCharacter == nullptr)
	{
		BlasterCharacter = Cast<ABlasterCharacter>(GetOwningActor());
	}
	if (BlasterCharacter == nullptr) return;

	FVector Velocity = BlasterCharacter->GetVelocity();
	Velocity.Z = 0;
	Speed = Velocity.Size();
	bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0;

	bWeaponEquipped = BlasterCharacter->IsWeaponEquipped();
	EquippedWeapon = BlasterCharacter->GetEquippedWeapon();
	bIsCrouched = BlasterCharacter->bIsCrouched;
	bIsAiming = BlasterCharacter->IsAiming();

	// Turn in place
	TurningInPlace = BlasterCharacter->GetTurningInPlace();
	
	FRotator AimRotation = BlasterCharacter->GetBaseAimRotation();

	// offset yaw for strafing
	//YawOffset = UKismetAnimationLibrary::CalculateDirection(Velocity, AimRotation);
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(Velocity);
	FRotator Diff = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, Diff, DeltaSeconds, 15.0f);
	YawOffset = DeltaRotation.Yaw;

	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = BlasterCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(
		CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaSeconds;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaSeconds, 6.0f);
	Lean = FMath::Clamp(Interp, -90.0f, 90.0f);

	// AimOffsets
	AO_Yaw = BlasterCharacter->GetAOYaw();
	AO_Pitch = BlasterCharacter->GetAOPitch();

	// Left hand IK
	auto WeaponMesh = EquippedWeapon ? EquippedWeapon->GetWeaponMesh() : nullptr;
	auto BlasterMesh = BlasterCharacter->GetMesh();
	if (bWeaponEquipped && EquippedWeapon && WeaponMesh && BlasterMesh)
	{
		LeftHandTransform = WeaponMesh->GetSocketTransform("LeftHandSocket", ERelativeTransformSpace::RTS_World);
		FVector OutPosition;
		FRotator OutRotation;
		BlasterMesh->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator,
		                                  OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat{OutRotation});
	}
}
