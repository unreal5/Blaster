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

		/**
		 * 修正武器方向与上瞄准方向不一致的问题
		 */

		// 骨骼名不区分大小写
		// 右手位置
		FTransform RightHandTransform = WeaponMesh->GetSocketTransform("Hand_R", ERelativeTransformSpace::RTS_World);
		// 目标位置
		auto HitTarget = BlasterCharacter->GetHitTarget();
		//右手前向指向父骨骼，要取反向
		///*UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(),
		//		                                                           RightHandTransform.GetLocation() +
		//		                                                           (RightHandTransform.GetLocation() - HitTarget));
		//		                                                           */
		RightHandRotation = UKismetMathLibrary::FindLookAtRotation(HitTarget, RightHandTransform.GetLocation());

		// 调试绘制
		/*
		// 1. 获取武器的MuzzleFlash位置
		FTransform MuzzleTipTransform = WeaponMesh->GetSocketTransform("MuzzleFlash",
		                                                               ERelativeTransformSpace::RTS_World);
		FVector MuzzleX = FRotationMatrix{MuzzleTipTransform.GetRotation().Rotator()}.GetUnitAxis(EAxis::X);

		auto World = GetWorld();
		// 2. 画出MuzzleFlash的方向
		DrawDebugLine(World, MuzzleTipTransform.GetLocation(), MuzzleTipTransform.GetLocation() + MuzzleX * 10000.f,
		              FColor::Red, false, 0.f, 0, 2);

		// 绘制瞄准方向
		DrawDebugLine(World, MuzzleTipTransform.GetLocation(), HitTarget, FColor::Blue, false, 0.f, 0, 2);
		*/
	}
}
