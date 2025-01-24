#pragma once

#include "CoreMinimal.h"
#include "InputMappingContext.h"
#include "BlasterTypes/TurningInPlace.h"
#include "GameFramework/Character.h"

#include "Interface/InteractWithCrosshair.h"

#include "BlasterCharacter.generated.h"

class AWeapon;
class UCameraComponent;
class USpringArmComponent;

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshair
{
	GENERATED_BODY()

public:
	ABlasterCharacter();

	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void BeginPlay() override;
	virtual void NotifyControllerChanged() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Jump() override;
protected:
	void AimOffset(float DeltaTime);
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* EquipAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* CrouchAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* AimAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* FireAction;
	// widget
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = UI, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidgetComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* CombatComponent;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	AWeapon* OverlappingWeapon = nullptr;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* OldWeapon);

	// 装备武器
	void EquipButtonPressed();

	// 下蹲
	void CrouchButtonPressed();

	// 瞄准
	void AimButtonPressed();
	void AimButtonReleased();
	
	UFUNCTION(Server, Reliable)
	void Server_EquipButtonPressed();

	// 开火
	//void FireButtonPressed();
	//void FireButtonReleased();
	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotator;

	// turning in place
	ETurningInPlace TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	void TurnInPlace(float DeltaTime);

	UPROPERTY(EditDefaultsOnly, Category=Combat, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAnimMontage> FireWeaponMontage;

	UPROPERTY(EditDefaultsOnly, Category=Combat, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAnimMontage> HitReactMontage;


	
	void HideCameraIfCharacterClose();
	UPROPERTY(EditDefaultsOnly, Category=Combat, meta=(AllowPrivateAccess="true"))
	float CameraThreshold = 200.f;
public:
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped() const;
	bool IsAiming() const;
	float GetAOYaw() const { return AO_Yaw; }
	float GetAOPitch() const { return AO_Pitch; }
	AWeapon* GetEquippedWeapon() const;
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	void PlayFireMontage(bool bAiming);
	void PlayHitReactMontage();
	
	FVector GetHitTarget() const;

	FORCEINLINE UCameraComponent *GetFollowCamera() const { return FollowCamera; }

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Hit();
};




