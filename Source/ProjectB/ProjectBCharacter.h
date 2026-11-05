// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/PBPlayerCharacter.h"
#include "ProjectBCharacter.generated.h"

class AActor;
class AShooterWeapon;
class UActorComponent;
class UAnimMontage;
class UCameraComponent;
class UInputAction;
class UInputComponent;
class USkeletalMeshComponent;

struct FDamageEvent;
struct FInputActionValue;


/**
 * Generic combat state for the project character.
 *
 * This is deliberately broader than the old ShooterCharacter's
 * simple HP/dead state so that the same character foundation can
 * later support RPG combat, stagger, knockdown, finishers, etc.
 */
UENUM(BlueprintType)
enum class EProjectBCombatState : uint8
{
	Neutral		UMETA(DisplayName = "Neutral"),
	Combat		UMETA(DisplayName = "Combat"),
	HitReact	UMETA(DisplayName = "Hit Reaction"),
	Staggered	UMETA(DisplayName = "Staggered"),
	Downed		UMETA(DisplayName = "Downed"),
	Defeated	UMETA(DisplayName = "Defeated")
};


/**
 * Generic attack payload.
 *
 * This is intentionally independent of the old ShooterWeapon class.
 * Melee, firearms, abilities, environmental attacks, etc. can all
 * eventually use this structure.
 */
USTRUCT(BlueprintType)
struct FProjectBAttackData
{
	GENERATED_BODY()

	/** Normal HP / vitality damage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float Damage = 0.0f;

	/** Stability / poise damage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float StabilityDamage = 0.0f;

	/** Optional physical knockback amount. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float Knockback = 0.0f;

	/** Direction of the incoming attack. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	FVector HitDirection = FVector::ZeroVector;

	/** Project-defined attack identifier/type. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	FName AttackType = NAME_None;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FProjectBHealthChangedDelegate,
	float,
	CurrentHealth,
	float,
	MaxHealth
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FProjectBStabilityChangedDelegate,
	float,
	CurrentStability,
	float,
	MaxStability
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FProjectBCombatStateChangedDelegate,
	EProjectBCombatState,
	PreviousState,
	EProjectBCombatState,
	NewState
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FProjectBAmmoChangedDelegate,
	int32,
	MagazineSize,
	int32,
	Bullets
);


/**
 * Project-specific player character base.
 *
 * Inheritance:
 *
 * ACharacter
 *     -> APBPlayerCharacter
 *         -> AProjectBCharacter
 *             -> BP_ProjectBPBCharacter
 *
 * PBPlayerCharacter owns the movement implementation.
 * This class owns project-level character functionality.
 */
UCLASS(Abstract)
class PROJECTB_API AProjectBCharacter : public APBPlayerCharacter
{
	GENERATED_BODY()

protected:

	/* ------------------------------------------------------------------ */
	/* Components                                                         */
	/* ------------------------------------------------------------------ */

	/**
	 * Project first-person camera.
	 *
	 * PB owns Mesh1P; this class owns the actual project camera.
	 */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Presentation")
	FRotator ThirdPersonMeshRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Presentation")
	FRotator FirstPersonMeshRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Components",
		meta = (AllowPrivateAccess = "true")
	)
	TObjectPtr<UCameraComponent> FirstPersonCameraComponent;


	/* ------------------------------------------------------------------ */
	/* Legacy / compatibility input                                       */
	/* ------------------------------------------------------------------ */

	/**
	 * These actions are retained so the old C++ ShooterCharacter
	 * input architecture can continue to compile/work.
	 *
	 * BP_ProjectBPBCharacter currently uses its own Enhanced Input
	 * graph, so these can remain unassigned there.
	 */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "First Person")
	TObjectPtr<USkeletalMeshComponent> FirstPersonMesh;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MouseLookAction;


	/* ------------------------------------------------------------------ */
	/* Character control                                                   */
	/* ------------------------------------------------------------------ */

	/**
	 * Master gameplay-input switch.
	 *
	 * Useful later for death, stagger, dialogue, cutscenes, menus,
	 * finishers, etc.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Character|Input"
	)
	bool bProjectBInputEnabled = true;


	/* ------------------------------------------------------------------ */
	/* RPG / combat                                                       */
	/* ------------------------------------------------------------------ */

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "RPG|Stats",
		meta = (ClampMin = "0.0")
	)
	float MaxRPGHealth = 100.0f;

	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "RPG|Stats"
	)
	float CurrentRPGHealth = 0.0f;


	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "RPG|Combat",
		meta = (ClampMin = "0.0")
	)
	float MaxRPGStability = 100.0f;

	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "RPG|Combat"
	)
	float CurrentRPGStability = 0.0f;


	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "RPG|Combat"
	)
	EProjectBCombatState RPGCombatState = EProjectBCombatState::Neutral;


	/** Optional current RPG target. */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "RPG|Targeting"
	)
	TObjectPtr<AActor> CurrentRPGTarget;


	/* ------------------------------------------------------------------ */
	/* Shooter                                                            */
	/* ------------------------------------------------------------------ */

	/**
	 * Project-side weapon inventory.
	 *
	 * This is intentionally separate from the legacy AShooterCharacter
	 * fields for now. Once the migration is complete, the old
	 * ShooterCharacter fields can be removed and this becomes the
	 * common weapon inventory.
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Shooter|Weapons"
	)
	TArray<TObjectPtr<AShooterWeapon>> ProjectBOwnedWeapons;


	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Shooter|Weapons"
	)
	TObjectPtr<AShooterWeapon> ProjectBCurrentWeapon;


	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Shooter|Weapons"
	)
	FName FirstPersonWeaponSocket = FName(TEXT("HandGrip_R"));


	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Shooter|Weapons"
	)
	FName ThirdPersonWeaponSocket = FName(TEXT("HandGrip_R"));


	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Shooter|Aim",
		meta = (ClampMin = "0.0", ClampMax = "100000.0", Units = "cm")
	)
	float MaxAimDistance = 10000.0f;


	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Shooter|Tags"
	)
	FName PlayerTag = FName(TEXT("Player"));


	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Shooter|Input"
	)
	bool bShooterEnabled = true;


	/* ------------------------------------------------------------------ */
	/* Delegates                                                          */
	/* ------------------------------------------------------------------ */

public:

	UPROPERTY(BlueprintAssignable, Category = "RPG|Events")
	FProjectBHealthChangedDelegate OnRPGHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "RPG|Events")
	FProjectBStabilityChangedDelegate OnRPGStabilityChanged;

	UPROPERTY(BlueprintAssignable, Category = "RPG|Events")
	FProjectBCombatStateChangedDelegate OnRPGCombatStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Shooter|Events")
	FProjectBAmmoChangedDelegate OnShooterAmmoChanged;


	/* ------------------------------------------------------------------ */
	/* Constructor / lifecycle                                             */
	/* ------------------------------------------------------------------ */

public:


	

	AProjectBCharacter(
		const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()
	);

protected:

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	void DebugCharacterPresentation();

	float CharacterDebugLogTimer = 0.0f;

	virtual void SetupPlayerInputComponent(
		UInputComponent* PlayerInputComponent
	) override;


	/* ------------------------------------------------------------------ */
	/* Legacy movement API                                                */
	/* ------------------------------------------------------------------ */

protected:

	virtual void MoveInput(const FInputActionValue& Value);

	virtual void LookInput(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoMove(float Right, float Forward);

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoAim(float Yaw, float Pitch);

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpStart();

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpEnd();


	/* ------------------------------------------------------------------ */
	/* Project input control API                                           */
	/* ------------------------------------------------------------------ */

public:

	UFUNCTION(BlueprintPure, Category = "Character|Input")
	bool IsProjectBInputEnabled() const
	{
		return bProjectBInputEnabled;
	}

	UFUNCTION(BlueprintCallable, Category = "Character|Input")
	void SetProjectBInputEnabled(bool bEnabled)
	{
		bProjectBInputEnabled = bEnabled;
	}

	UFUNCTION(BlueprintPure, Category = "Character|Input")
	virtual bool CanReceivePlayerInput() const;


	/* ------------------------------------------------------------------ */
	/* RPG stats API                                                       */
	/* ------------------------------------------------------------------ */

public:

	UFUNCTION(BlueprintPure, Category = "RPG|Stats")
	float GetRPGHealth() const
	{
		return CurrentRPGHealth;
	}

	UFUNCTION(BlueprintPure, Category = "RPG|Stats")
	float GetMaxRPGHealth() const
	{
		return MaxRPGHealth;
	}

	UFUNCTION(BlueprintPure, Category = "RPG|Stats")
	float GetRPGHealthPercent() const;

	UFUNCTION(BlueprintCallable, Category = "RPG|Stats")
	virtual void SetMaxRPGHealth(float NewMaxHealth);

	UFUNCTION(BlueprintCallable, Category = "RPG|Stats")
	virtual float RestoreRPGHealth(float Amount);

	UFUNCTION(BlueprintPure, Category = "RPG|Combat")
	float GetRPGStability() const
	{
		return CurrentRPGStability;
	}

	UFUNCTION(BlueprintPure, Category = "RPG|Combat")
	float GetMaxRPGStability() const
	{
		return MaxRPGStability;
	}

	UFUNCTION(BlueprintPure, Category = "RPG|Combat")
	float GetRPGStabilityPercent() const;

	UFUNCTION(BlueprintCallable, Category = "RPG|Combat")
	virtual void SetMaxRPGStability(float NewMaxStability);

	UFUNCTION(BlueprintCallable, Category = "RPG|Combat")
	virtual float ApplyRPGStabilityDamage(float Amount);

	UFUNCTION(BlueprintCallable, Category = "RPG|Combat")
	virtual void ResetRPGStability();


	/* ------------------------------------------------------------------ */
	/* Generic attack API                                                  */
	/* ------------------------------------------------------------------ */

public:

	/**
	 * Generic RPG/combat damage entry point.
	 *
	 * This is separate from the old ShooterCharacter's TakeDamage
	 * implementation so the eventual RPG system can handle
	 * vitality and stability independently.
	 */
	UFUNCTION(BlueprintCallable, Category = "RPG|Combat")
	virtual float ReceiveRPGAttack(
		const FProjectBAttackData& AttackData
	);

	/**
	 * Standard UE damage entry point.
	 *
	 * BP_ProjectBPBCharacter will use this if something calls
	 * ApplyDamage() / TakeDamage() on the actor.
	 */
	virtual float TakeDamage(
		float Damage,
		const FDamageEvent& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser
	) override;

protected:

	float ApplyRPGDamage(float Amount);


	/* ------------------------------------------------------------------ */
	/* Combat state                                                        */
	/* ------------------------------------------------------------------ */

public:

	UFUNCTION(BlueprintPure, Category = "RPG|Combat")
	EProjectBCombatState GetRPGCombatState() const
	{
		return RPGCombatState;
	}

	UFUNCTION(BlueprintPure, Category = "RPG|Combat")
	bool IsRPGDefeated() const
	{
		return RPGCombatState == EProjectBCombatState::Defeated;
	}

	UFUNCTION(BlueprintPure, Category = "RPG|Combat")
	bool IsRPGDowned() const
	{
		return RPGCombatState == EProjectBCombatState::Downed;
	}

	UFUNCTION(BlueprintPure, Category = "RPG|Combat")
	bool IsRPGStaggered() const
	{
		return RPGCombatState == EProjectBCombatState::Staggered;
	}

	UFUNCTION(BlueprintCallable, Category = "RPG|Combat")
	virtual void SetRPGCombatState(EProjectBCombatState NewState);

	UFUNCTION(BlueprintCallable, Category = "RPG|Combat")
	virtual void BeginRPGCombat();

	UFUNCTION(BlueprintCallable, Category = "RPG|Combat")
	virtual void EndRPGCombat();

	UFUNCTION(BlueprintCallable, Category = "RPG|Combat")
	virtual void EnterRPGDowned();

	UFUNCTION(BlueprintCallable, Category = "RPG|Combat")
	virtual void DefeatRPGCharacter();


	/* ------------------------------------------------------------------ */
	/* Ability API                                                         */
	/* ------------------------------------------------------------------ */

public:

	UFUNCTION(
		BlueprintNativeEvent,
		BlueprintCallable,
		Category = "RPG|Abilities"
	)
	bool CanUseAbility(FName AbilityId) const;

	virtual bool CanUseAbility_Implementation(FName AbilityId) const;

	UFUNCTION(
		BlueprintNativeEvent,
		BlueprintCallable,
		Category = "RPG|Abilities"
	)
	bool UseAbility(FName AbilityId);

	virtual bool UseAbility_Implementation(FName AbilityId);

	UFUNCTION(
		BlueprintNativeEvent,
		BlueprintCallable,
		Category = "RPG|Abilities"
	)
	bool CancelAbility(FName AbilityId);

	virtual bool CancelAbility_Implementation(FName AbilityId);


	/* ------------------------------------------------------------------ */
	/* Inventory API                                                       */
	/* ------------------------------------------------------------------ */

public:

	UFUNCTION(
		BlueprintNativeEvent,
		BlueprintCallable,
		Category = "RPG|Inventory"
	)
	bool AddRPGItem(FName ItemId, int32 Quantity);

	virtual bool AddRPGItem_Implementation(
		FName ItemId,
		int32 Quantity
	);

	UFUNCTION(
		BlueprintNativeEvent,
		BlueprintCallable,
		Category = "RPG|Inventory"
	)
	bool RemoveRPGItem(FName ItemId, int32 Quantity);

	virtual bool RemoveRPGItem_Implementation(
		FName ItemId,
		int32 Quantity
	);

	UFUNCTION(
		BlueprintNativeEvent,
		BlueprintPure,
		Category = "RPG|Inventory"
	)
	bool HasRPGItem(FName ItemId, int32 Quantity) const;

	virtual bool HasRPGItem_Implementation(
		FName ItemId,
		int32 Quantity
	) const;


	/* ------------------------------------------------------------------ */
	/* Targeting API                                                       */
	/* ------------------------------------------------------------------ */

public:

	UFUNCTION(BlueprintPure, Category = "RPG|Targeting")
	AActor* GetRPGTarget() const
	{
		return CurrentRPGTarget;
	}

	UFUNCTION(BlueprintCallable, Category = "RPG|Targeting")
	virtual void SetRPGTarget(AActor* NewTarget);

	UFUNCTION(BlueprintCallable, Category = "RPG|Targeting")
	virtual void ClearRPGTarget();


	/* ------------------------------------------------------------------ */
	/* RPG component bridge                                                */
	/* ------------------------------------------------------------------ */

public:

	/**
	 * Allows the project character to find jRPGTemplate or future
	 * project ActorComponents without hardcoding their C++ classes
	 * into this base class.
	 */
	UFUNCTION(BlueprintCallable, Category = "RPG|Components")
	UActorComponent* FindRPGComponentByClass(
		TSubclassOf<UActorComponent> ComponentClass
	) const;


	/* ------------------------------------------------------------------ */
	/* Shooter API                                                         */
	/* ------------------------------------------------------------------ */

public:

	UFUNCTION(BlueprintCallable, Category = "Shooter")
	virtual void DoStartFiring();

	UFUNCTION(BlueprintCallable, Category = "Shooter")
	virtual void DoStopFiring();

	UFUNCTION(BlueprintCallable, Category = "Shooter")
	virtual void DoSwitchWeapon();

	UFUNCTION(BlueprintCallable, Category = "Shooter|Weapons")
	virtual void AddWeaponClass(
		const TSubclassOf<AShooterWeapon>& WeaponClass
	);

	UFUNCTION(BlueprintCallable, Category = "Shooter|Weapons")
	virtual void AttachWeaponMeshes(AShooterWeapon* Weapon);

	UFUNCTION(BlueprintCallable, Category = "Shooter|Weapons")
	virtual void OnWeaponActivated(AShooterWeapon* Weapon);

	UFUNCTION(BlueprintCallable, Category = "Shooter|Weapons")
	virtual void OnWeaponDeactivated(AShooterWeapon* Weapon);

	UFUNCTION(BlueprintCallable, Category = "Shooter|Weapons")
	virtual void OnSemiWeaponRefire();

	UFUNCTION(BlueprintCallable, Category = "Shooter|Weapons")
	virtual void PlayFiringMontage(UAnimMontage* Montage);

	UFUNCTION(BlueprintCallable, Category = "Shooter|Weapons")
	virtual void AddWeaponRecoil(float Recoil);

	UFUNCTION(BlueprintCallable, Category = "Shooter|Weapons")
	virtual void UpdateWeaponHUD(
		int32 CurrentAmmo,
		int32 MagazineSize
	);

	UFUNCTION(BlueprintCallable, Category = "Shooter|Aim")
	virtual FVector GetWeaponTargetLocation() const;


	/* ------------------------------------------------------------------ */
	/* Shooter getters                                                     */
	/* ------------------------------------------------------------------ */

public:

	UFUNCTION(BlueprintPure, Category = "Shooter|Weapons")
	AShooterWeapon* GetProjectBCurrentWeapon() const
	{
		return ProjectBCurrentWeapon;
	}

	UFUNCTION(BlueprintPure, Category = "Shooter|Weapons")
	TArray<AShooterWeapon*> GetProjectBOwnedWeapons() const;

	UFUNCTION(BlueprintPure, Category = "Shooter|Weapons")
	bool IsShooterEnabled() const
	{
		return bShooterEnabled;
	}

	UFUNCTION(BlueprintCallable, Category = "Shooter|Weapons")
	void SetShooterEnabled(bool bEnabled);

	/* ------------------------------------------------------------------ */
	/* First-person bridge                                                 */
	/* ------------------------------------------------------------------ */

public:

	/**
	 * Returns PBPlayerCharacter's existing Mesh1P.
	 *
	 * We intentionally do NOT create a second first-person mesh.
	 */
	UFUNCTION(BlueprintPure, Category = "Components")
	USkeletalMeshComponent* GetFirstPersonMesh() const;

	/** Returns the ProjectB first-person camera. */
	UFUNCTION(BlueprintPure, Category = "Components")
	UCameraComponent* GetFirstPersonCameraComponent() const;


protected:

	/** Finds a weapon already owned by this project character. */
	AShooterWeapon* FindProjectBWeaponOfType(
		TSubclassOf<AShooterWeapon> WeaponClass
	) const;
};