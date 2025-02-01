#pragma once

#include "CoreMinimal.h"
#include "InputMappingContext.h"
#include "BlasterTypes/TurningInPlace.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Character.h"

#include "Interface/InteractWithCrosshair.h"

#include "BlasterCharacter.generated.h"

class ABlasterPlayerController;
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
	virtual void OnRep_ReplicateMovement() override;
	virtual void Jump() override;
	virtual void Destroyed() override;
	virtual void OnRep_PlayerState() override;
	// only on server
	void Elim();
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();
protected:
	void AimOffset(float DeltaTime);
	void SimProxiesTurn();

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor,  float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);
	
	void UpdateHudHealth();
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
	void CalculateAO_Pitch();
	float CalculateSpeed();

	UPROPERTY(EditDefaultsOnly, Category=Combat, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAnimMontage> FireWeaponMontage;

	UPROPERTY(EditDefaultsOnly, Category=Combat, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAnimMontage> HitReactMontage;

	UPROPERTY(EditDefaultsOnly, Category=Combat, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAnimMontage> ElimMontage;
	
	void HideCameraIfCharacterClose();
	
	UPROPERTY(EditDefaultsOnly, Category=Combat, meta=(AllowPrivateAccess="true"))
	float CameraThreshold = 200.f;

	bool bRotateRootBone = false;
	float TurnThreshold = 0.5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw = 0.f;
	float TimeSinceLastMovementReplication = 0.f;

	/*
	 * Player Health
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Player Stats", meta = (AllowPrivateAccess = "true"))
	float MaxHealth =100.f;

	UPROPERTY(ReplicatedUsing=OnRep_Health, VisibleAnywhere, Category = "Player Stats", meta = (AllowPrivateAccess = "true"))
	float Health = 100.f;

	UFUNCTION()
	void OnRep_Health();

	ABlasterPlayerController* BlasterPlayerController;

	bool bElimmed = false;

	FTimerHandle ElimTimerHandle;

	UPROPERTY(EditDefaultsOnly, Category = Elim, meta = (AllowPrivateAccess = "true"))
	float ElimDelay = 5.f;
	void ElimTimerFinished();

	/**
	 * Dissolve effect
	 */
	FOnTimelineFloat DissolveTrack;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTimelineComponent> DissolveTimeline;

	UPROPERTY(EditDefaultsOnly, Category = Elim, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCurveFloat> DissolveCurve;
	UFUNCTION()
	void UpdateDissolveMaterial(float Value);

	void StartDissolve();

	// 动态材质，运行时可以改变材质参数。由网格的材质实例创建，因此不可以在编辑器中设置
	UPROPERTY(Transient, VisibleAnywhere, Category = Elim, meta = (AllowPrivateAccess = "true"))
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;
	// 静态材质，运行时不可以改变材质参数
	UPROPERTY(EditDefaultsOnly, Category = Elim, meta = (AllowPrivateAccess = "true"))
	UMaterialInstance* DissolveMaterialInstance;

	/*
	 * Elim bot
	 */
	UPROPERTY(EditDefaultsOnly, Category = Elim, meta = (AllowPrivateAccess = "true"))
	UParticleSystem* ElimBotEffect;
	UPROPERTY(VisibleAnywhere, Category = Elim, meta = (AllowPrivateAccess = "true"))
	UParticleSystemComponent* ElimBotComponent;
	UPROPERTY(EditDefaultsOnly, Category = Elim, meta = (AllowPrivateAccess = "true"))
	USoundBase* ElimBotSound;
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
	void PlayElimMontage();
	
	FVector GetHitTarget() const;

	FORCEINLINE UCameraComponent *GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }

	// 只有Server才注册OhHit事件，因此此函数只会在Server上调用
	// 子类已经进行了伤害处理，生命值变化时，会引起客户端的OnRep_Health调用。
	// 复制效率比RPC高，因此我们不再使用以下代码
	//UFUNCTION(NetMulticast, Reliable)
	//void Multicast_Hit();
	FORCEINLINE bool IsElimmed() const { return bElimmed; }	
};




