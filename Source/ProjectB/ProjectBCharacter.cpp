// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectBCharacter.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
 
#include "Components/ActorComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "InputActionValue.h"
#include "ProjectB.h"
#include "ShooterWeapon.h"
#include "DrawDebugHelpers.h"

static TAutoConsoleVariable<int32> CVarProjectBCharacterDebug(
	TEXT("ProjectB.CharacterDebug"),
	1,
	TEXT("Enable ProjectB character presentation diagnostics.\n")
	TEXT("0 = disabled\n")
	TEXT("1 = enabled\n"),
	ECVF_Default
);


AProjectBCharacter::AProjectBCharacter(
	const FObjectInitializer& ObjectInitializer
)
	: Super(ObjectInitializer)
{
	/*
	 * APBPlayerCharacter already creates and configures:
	 *
	 * - UPBPlayerMovement
	 * - CapsuleComponent size
	 * - Mesh1P
	 * - third-person mesh
	 * - eye heights
	 * - PB movement parameters
	 *
	 * We deliberately do NOT recreate any of these.
	 */

	FirstPersonMesh = GetMesh1P();

	// PB's Manny skeletal mesh is visually oriented 90 degrees
	// relative to the character's forward axis.
	if (FirstPersonMesh)
	{
		FirstPersonMesh->SetRelativeRotation(
			FRotator(0.0f, -90.0f, 0.0f)
		);
	}

	 /* -------------------------------------------------------------- */
	 /* First-person camera                                             */
	 /* -------------------------------------------------------------- */

	// Create the project first-person camera.
	FirstPersonCameraComponent =
		ObjectInitializer.CreateDefaultSubobject<UCameraComponent>(
			this,
			TEXT("First Person Camera")
		);

	// Attach the camera to the first-person head.
	// This keeps the camera at the actual character head instead of
	// leaving it in the middle of the capsule.
	if (USkeletalMeshComponent* FirstPersonMeshComponent = GetFirstPersonMesh())
	{
		FirstPersonCameraComponent->SetupAttachment(
			FirstPersonMeshComponent,
			TEXT("head")
		);
	}
	else
	{
		// Fallback only if PB first-person mesh is unavailable.
		FirstPersonCameraComponent->SetupAttachment(
			GetCapsuleComponent()
		);
	}

	// Manny/PB skeletal orientation requires this camera-space correction.
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(
		FVector(-2.8f, 5.89f, 0.0f),
		FRotator(0.0f, 90.0f, -90.0f)
	);

	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;

	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	/* -------------------------------------------------------------- */
	/* Default RPG state                                               */
	/* -------------------------------------------------------------- */

	CurrentRPGHealth = MaxRPGHealth;
	CurrentRPGStability = MaxRPGStability;
	RPGCombatState = EProjectBCombatState::Neutral;
}


/* ================================================================== */
/* Lifecycle                                                          */
/* ================================================================== */


void AProjectBCharacter::BeginPlay()
{
	Super::BeginPlay();

	//if (AController* PlayerController = GetController())
	//{
		//PlayerController->SetControlRotation(GetActorRotation());
	//}

	/*
	 * Initialize the runtime values here rather than relying on the
	 * constructor values. This also makes editor-tuned Max values work
	 * correctly.
	 */
	CurrentRPGHealth = FMath::Clamp(
		CurrentRPGHealth <= 0.0f
		? MaxRPGHealth
		: CurrentRPGHealth,
		0.0f,
		MaxRPGHealth
	);

	CurrentRPGStability = FMath::Clamp(
		CurrentRPGStability <= 0.0f
		? MaxRPGStability
		: CurrentRPGStability,
		0.0f,
		MaxRPGStability
	);

	RPGCombatState = EProjectBCombatState::Neutral;

	OnRPGHealthChanged.Broadcast(
		CurrentRPGHealth,
		MaxRPGHealth
	);

	OnRPGStabilityChanged.Broadcast(
		CurrentRPGStability,
		MaxRPGStability
	);

	if (CVarProjectBCharacterDebug.GetValueOnGameThread() != 0)
	{
		DebugCharacterPresentation();
	}
}


void AProjectBCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CVarProjectBCharacterDebug.GetValueOnGameThread() == 0)
	{
		return;
	}

	CharacterDebugLogTimer += DeltaTime;

	/*
	 * Log/refresh diagnostics every 0.25 seconds.
	 * Debug drawing itself is refreshed every tick with a short lifetime.
	 */
	DebugCharacterPresentation();

	if (CharacterDebugLogTimer >= 0.25f)
	{
		CharacterDebugLogTimer = 0.0f;

		UE_LOG(
			LogProjectB,
			Warning,
			TEXT("=== ProjectB Character Presentation Debug ===")
		);

		UE_LOG(
			LogProjectB,
			Warning,
			TEXT(
				"ACTOR  Loc=%s Rot=%s Forward=%s Right=%s Up=%s"
			),
			*GetActorLocation().ToString(),
			*GetActorRotation().ToString(),
			*GetActorForwardVector().ToString(),
			*GetActorRightVector().ToString(),
			*GetActorUpVector().ToString()
		);

		if (USkeletalMeshComponent* Mesh1PComponent = GetMesh1P())
		{
			const FName HeadSocket = TEXT("head");

			if (Mesh1PComponent->DoesSocketExist(HeadSocket))
			{
				const FTransform HeadTransform =
					Mesh1PComponent->GetSocketTransform(
						HeadSocket,
						RTS_World
					);

				UE_LOG(
					LogProjectB,
					Warning,
					TEXT(
						"1P HEAD: Loc=%s Rot=%s Forward=%s"
					),
					*HeadTransform.GetLocation().ToString(),
					*HeadTransform.Rotator().ToString(),
					*HeadTransform.GetUnitAxis(EAxis::X).ToString()
				);
			}
			else
			{
				UE_LOG(
					LogProjectB,
					Warning,
					TEXT("1P Mesh has no 'head' socket")
				);
			}
		}

		if (GetMesh())
		{
			const FName HeadSocket = TEXT("head");

			if (GetMesh()->DoesSocketExist(HeadSocket))
			{
				const FTransform HeadTransform =
					GetMesh()->GetSocketTransform(
						HeadSocket,
						RTS_World
					);

				UE_LOG(
					LogProjectB,
					Warning,
					TEXT(
						"3P HEAD: Loc=%s Rot=%s Forward=%s"
					),
					*HeadTransform.GetLocation().ToString(),
					*HeadTransform.Rotator().ToString(),
					*HeadTransform.GetUnitAxis(EAxis::X).ToString()
				);
			}
		}

		if (GetCapsuleComponent())
		{
			UE_LOG(
				LogProjectB,
				Warning,
				TEXT(
					"CAPSULE RelLoc=%s RelRot=%s WorldLoc=%s WorldRot=%s"
				),
				*GetCapsuleComponent()->GetRelativeLocation().ToString(),
				*GetCapsuleComponent()->GetRelativeRotation().ToString(),
				*GetCapsuleComponent()->GetComponentLocation().ToString(),
				*GetCapsuleComponent()->GetComponentRotation().ToString()
			);
		}

		if (GetMesh())
		{
			UE_LOG(
				LogProjectB,
				Warning,
				TEXT(
					"3P MESH Asset=%s RelLoc=%s RelRot=%s WorldLoc=%s WorldRot=%s Forward=%s"
				),
				*GetNameSafe(GetMesh()->GetSkeletalMeshAsset()),
				*GetMesh()->GetRelativeLocation().ToString(),
				*GetMesh()->GetRelativeRotation().ToString(),
				*GetMesh()->GetComponentLocation().ToString(),
				*GetMesh()->GetComponentRotation().ToString(),
				*GetMesh()->GetForwardVector().ToString()
			);
		}

		if (USkeletalMeshComponent* Mesh1PComponent = GetMesh1P())
		{
			UE_LOG(
				LogProjectB,
				Warning,
				TEXT(
					"1P MESH Asset=%s RelLoc=%s RelRot=%s WorldLoc=%s WorldRot=%s Forward=%s"
				),
				*GetNameSafe(Mesh1PComponent->GetSkeletalMeshAsset()),
				*Mesh1PComponent->GetRelativeLocation().ToString(),
				*Mesh1PComponent->GetRelativeRotation().ToString(),
				*Mesh1PComponent->GetComponentLocation().ToString(),
				*Mesh1PComponent->GetComponentRotation().ToString(),
				*Mesh1PComponent->GetForwardVector().ToString()
			);
		}

		if (FirstPersonCameraComponent)
		{
			UE_LOG(
				LogProjectB,
				Warning,
				TEXT(
					"CAMERA RelLoc=%s RelRot=%s WorldLoc=%s WorldRot=%s Forward=%s"
				),
				*FirstPersonCameraComponent->GetRelativeLocation().ToString(),
				*FirstPersonCameraComponent->GetRelativeRotation().ToString(),
				*FirstPersonCameraComponent->GetComponentLocation().ToString(),
				*FirstPersonCameraComponent->GetComponentRotation().ToString(),
				*FirstPersonCameraComponent->GetForwardVector().ToString()
			);
		}

		if (AController* ControllerPtr = GetController())
		{
			UE_LOG(
				LogProjectB,
				Warning,
				TEXT(
					"CONTROLLER ControlRot=%s"
				),
				*ControllerPtr->GetControlRotation().ToString()
			);
		}

		UE_LOG(
			LogProjectB,
			Warning,
			TEXT(
				"BASE EYE HEIGHT=%.3f | FullCrouchedEyeHeight=%.3f"
			),
			BaseEyeHeight,
			FullCrouchedEyeHeight
		);
	}
}

void AProjectBCharacter::DebugCharacterPresentation()
{
	if (!GetWorld())
	{
		return;
	}

	constexpr float ActorAxisLength = 100.0f;
	constexpr float ComponentAxisLength = 60.0f;
	constexpr float ForwardLineLength = 100.0f;
	constexpr float Thickness = 2.5f;
	constexpr float LifeTime = 0.05f;

	/*
	 * Actor coordinate system.
	 *
	 * X = red
	 * Y = green
	 * Z = blue
	 */
	DrawDebugCoordinateSystem(
		GetWorld(),
		GetActorLocation(),
		GetActorRotation(),
		ActorAxisLength,
		false,
		LifeTime,
		0,
		Thickness
	);

	/*
	 * Capsule coordinate system.
	 */
	if (GetCapsuleComponent())
	{
		DrawDebugCoordinateSystem(
			GetWorld(),
			GetCapsuleComponent()->GetComponentLocation(),
			GetCapsuleComponent()->GetComponentRotation(),
			ComponentAxisLength,
			false,
			LifeTime,
			0,
			Thickness
		);
	}

	/*
	 * Third-person mesh coordinate system.
	 */
	if (GetMesh())
	{
		DrawDebugCoordinateSystem(
			GetWorld(),
			GetMesh()->GetComponentLocation(),
			GetMesh()->GetComponentRotation(),
			ComponentAxisLength,
			false,
			LifeTime,
			0,
			Thickness
		);

		DrawDebugLine(
			GetWorld(),
			GetMesh()->GetComponentLocation(),
			GetMesh()->GetComponentLocation() +
			GetMesh()->GetForwardVector() * ForwardLineLength,
			FColor::Yellow,
			false,
			LifeTime,
			0,
			4.0f
		);
	}



	/*
	 * First-person mesh coordinate system.
	 */
	if (USkeletalMeshComponent* Mesh1PComponent = GetMesh1P())
	{
		DrawDebugCoordinateSystem(
			GetWorld(),
			Mesh1PComponent->GetComponentLocation(),
			Mesh1PComponent->GetComponentRotation(),
			ComponentAxisLength,
			false,
			LifeTime,
			0,
			Thickness
		);

		DrawDebugLine(
			GetWorld(),
			Mesh1PComponent->GetComponentLocation(),
			Mesh1PComponent->GetComponentLocation() +
			Mesh1PComponent->GetForwardVector() * ForwardLineLength,
			FColor::Cyan,
			false,
			LifeTime,
			0,
			4.0f
		);
	}

	/*
	 * Camera coordinate system.
	 */
	if (FirstPersonCameraComponent)
	{
		DrawDebugCoordinateSystem(
			GetWorld(),
			FirstPersonCameraComponent->GetComponentLocation(),
			FirstPersonCameraComponent->GetComponentRotation(),
			ComponentAxisLength,
			false,
			LifeTime,
			0,
			Thickness
		);

		DrawDebugLine(
			GetWorld(),
			FirstPersonCameraComponent->GetComponentLocation(),
			FirstPersonCameraComponent->GetComponentLocation() +
			FirstPersonCameraComponent->GetForwardVector() *
			ForwardLineLength,
			FColor::Green,
			false,
			LifeTime,
			0,
			4.0f
		);
	}

	/*
	 * Eye/camera position marker.
	 */
	if (FirstPersonCameraComponent)
	{
		DrawDebugSphere(
			GetWorld(),
			FirstPersonCameraComponent->GetComponentLocation(),
			6.0f,
			12,
			FColor::Green,
			false,
			LifeTime,
			0,
			2.0f
		);
	}
}


/* ================================================================== */
/* Input compatibility                                                */
/* ================================================================== */

void AProjectBCharacter::SetupPlayerInputComponent(
	UInputComponent* PlayerInputComponent
)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	/*
	 * This is the original ProjectB/Shooter C++ input path.
	 *
	 * BP_ProjectBPBCharacter currently uses its own Enhanced Input
	 * nodes, so these properties can remain empty there.
	 *
	 * Keeping this code means the existing ShooterCharacter hierarchy
	 * remains usable while we migrate the project.
	 */

	if (UEnhancedInputComponent* EnhancedInputComponent =
		Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (IsValid(JumpAction))
		{
			EnhancedInputComponent->BindAction(
				JumpAction,
				ETriggerEvent::Started,
				this,
				&AProjectBCharacter::DoJumpStart
			);

			EnhancedInputComponent->BindAction(
				JumpAction,
				ETriggerEvent::Completed,
				this,
				&AProjectBCharacter::DoJumpEnd
			);
		}

		if (IsValid(MoveAction))
		{
			EnhancedInputComponent->BindAction(
				MoveAction,
				ETriggerEvent::Triggered,
				this,
				&AProjectBCharacter::MoveInput
			);
		}

		if (IsValid(LookAction))
		{
			EnhancedInputComponent->BindAction(
				LookAction,
				ETriggerEvent::Triggered,
				this,
				&AProjectBCharacter::LookInput
			);
		}

		if (IsValid(MouseLookAction))
		{
			EnhancedInputComponent->BindAction(
				MouseLookAction,
				ETriggerEvent::Triggered,
				this,
				&AProjectBCharacter::LookInput
			);
		}
	}
	else
	{
		UE_LOG(
			LogProjectB,
			Error,
			TEXT("'%s' Failed to find an Enhanced Input Component."),
			*GetNameSafe(this)
		);
	}
}


void AProjectBCharacter::MoveInput(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();

	DoMove(
		MovementVector.X,
		MovementVector.Y
	);
}


void AProjectBCharacter::LookInput(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	DoAim(
		LookAxisVector.X,
		LookAxisVector.Y
	);
}


/* ================================================================== */
/* Movement compatibility                                             */
/* ================================================================== */

void AProjectBCharacter::DoMove(
	float Right,
	float Forward
)
{
	if (!CanReceivePlayerInput())
	{
		return;
	}

	/*
	 * IMPORTANT:
	 *
	 * Do not duplicate the old ProjectBCharacter implementation:
	 *
	 * AddMovementInput(GetActorRightVector(), Right);
	 * AddMovementInput(GetActorForwardVector(), Forward);
	 *
	 * PBPlayerCharacter already provides MoveRight/MoveForward and
	 * those feed the PB movement implementation.
	 */

	MoveRight(Right);
	MoveForward(Forward);
}


void AProjectBCharacter::DoAim(
	float Yaw,
	float Pitch
)
{
	if (!CanReceivePlayerInput())
	{
		return;
	}

	AddControllerYawInput(Yaw);
	AddControllerPitchInput(Pitch);
}


void AProjectBCharacter::DoJumpStart()
{
	if (!CanReceivePlayerInput())
	{
		return;
	}

	/*
	 * Calls APBPlayerCharacter::Jump(), not ACharacter::Jump().
	 *
	 * Therefore PB jump boost / deferred stop / bunnyhop behavior
	 * remains active.
	 */
	Jump();
}


void AProjectBCharacter::DoJumpEnd()
{
	/*
	 * Do not block StopJumping with the same state check used above if
	 * the character is already airborne; PB itself has special deferred
	 * jump-stop handling.
	 */
	if (!bProjectBInputEnabled)
	{
		return;
	}

	StopJumping();
}


bool AProjectBCharacter::CanReceivePlayerInput() const
{
	if (!bProjectBInputEnabled)
	{
		return false;
	}

	if (RPGCombatState == EProjectBCombatState::Downed)
	{
		return false;
	}

	if (RPGCombatState == EProjectBCombatState::Defeated)
	{
		return false;
	}

	return true;
}


/* ================================================================== */
/* RPG stats                                                          */
/* ================================================================== */

float AProjectBCharacter::GetRPGHealthPercent() const
{
	if (MaxRPGHealth <= SMALL_NUMBER)
	{
		return 0.0f;
	}

	return FMath::Clamp(
		CurrentRPGHealth / MaxRPGHealth,
		0.0f,
		1.0f
	);
}


void AProjectBCharacter::SetMaxRPGHealth(
	float NewMaxHealth
)
{
	MaxRPGHealth = FMath::Max(
		0.0f,
		NewMaxHealth
	);

	CurrentRPGHealth = FMath::Clamp(
		CurrentRPGHealth,
		0.0f,
		MaxRPGHealth
	);

	OnRPGHealthChanged.Broadcast(
		CurrentRPGHealth,
		MaxRPGHealth
	);
}


float AProjectBCharacter::RestoreRPGHealth(
	float Amount
)
{
	if (Amount <= 0.0f || IsRPGDefeated())
	{
		return 0.0f;
	}

	const float OldHealth = CurrentRPGHealth;

	CurrentRPGHealth = FMath::Clamp(
		CurrentRPGHealth + Amount,
		0.0f,
		MaxRPGHealth
	);

	const float AppliedAmount =
		CurrentRPGHealth - OldHealth;

	if (!FMath::IsNearlyZero(AppliedAmount))
	{
		OnRPGHealthChanged.Broadcast(
			CurrentRPGHealth,
			MaxRPGHealth
		);
	}

	return AppliedAmount;
}


float AProjectBCharacter::GetRPGStabilityPercent() const
{
	if (MaxRPGStability <= SMALL_NUMBER)
	{
		return 0.0f;
	}

	return FMath::Clamp(
		CurrentRPGStability / MaxRPGStability,
		0.0f,
		1.0f
	);
}


void AProjectBCharacter::SetMaxRPGStability(
	float NewMaxStability
)
{
	MaxRPGStability = FMath::Max(
		0.0f,
		NewMaxStability
	);

	CurrentRPGStability = FMath::Clamp(
		CurrentRPGStability,
		0.0f,
		MaxRPGStability
	);

	OnRPGStabilityChanged.Broadcast(
		CurrentRPGStability,
		MaxRPGStability
	);
}


float AProjectBCharacter::ApplyRPGStabilityDamage(
	float Amount
)
{
	if (Amount <= 0.0f || IsRPGDefeated())
	{
		return 0.0f;
	}

	const float OldStability = CurrentRPGStability;

	CurrentRPGStability = FMath::Clamp(
		CurrentRPGStability - Amount,
		0.0f,
		MaxRPGStability
	);

	const float AppliedAmount =
		OldStability - CurrentRPGStability;

	if (!FMath::IsNearlyZero(AppliedAmount))
	{
		OnRPGStabilityChanged.Broadcast(
			CurrentRPGStability,
			MaxRPGStability
		);
	}

	if (CurrentRPGStability <= SMALL_NUMBER &&
		RPGCombatState != EProjectBCombatState::Downed &&
		RPGCombatState != EProjectBCombatState::Defeated)
	{
		SetRPGCombatState(
			EProjectBCombatState::Staggered
		);
	}

	return AppliedAmount;
}


void AProjectBCharacter::ResetRPGStability()
{
	CurrentRPGStability = MaxRPGStability;

	OnRPGStabilityChanged.Broadcast(
		CurrentRPGStability,
		MaxRPGStability
	);

	if (RPGCombatState == EProjectBCombatState::Staggered)
	{
		SetRPGCombatState(
			EProjectBCombatState::Combat
		);
	}
}


/* ================================================================== */
/* Generic RPG attack                                                 */
/* ================================================================== */

float AProjectBCharacter::ReceiveRPGAttack(
	const FProjectBAttackData& AttackData
)
{
	if (IsRPGDefeated())
	{
		return 0.0f;
	}

	if (RPGCombatState == EProjectBCombatState::Downed)
	{
		return 0.0f;
	}

	if (AttackData.Damage > 0.0f)
	{
		ApplyRPGDamage(
			AttackData.Damage
		);
	}

	if (AttackData.StabilityDamage > 0.0f)
	{
		ApplyRPGStabilityDamage(
			AttackData.StabilityDamage
		);
	}

	/*
	 * Knockback is deliberately NOT applied here yet.
	 *
	 * PB already has its own damage momentum implementation.
	 * Later we can decide whether project attacks should use that,
	 * explicit impulses, or a dedicated combat reaction system.
	 */

	return FMath::Max(
		0.0f,
		AttackData.Damage
	);
}


float AProjectBCharacter::TakeDamage(
	float Damage,
	const FDamageEvent& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser
)
{
	if (Damage <= 0.0f)
	{
		return 0.0f;
	}

	return ApplyRPGDamage(
		Damage
	);
}


/*
 * Internal health operation.
 *
 * Kept separate from TakeDamage() so the RPG system can eventually
 * distinguish normal UE damage from physical combat damage.
 */
float AProjectBCharacter::ApplyRPGDamage(
	float Amount
)
{
	if (Amount <= 0.0f || IsRPGDefeated())
	{
		return 0.0f;
	}

	const float OldHealth = CurrentRPGHealth;

	CurrentRPGHealth = FMath::Clamp(
		CurrentRPGHealth - Amount,
		0.0f,
		MaxRPGHealth
	);

	const float AppliedAmount =
		OldHealth - CurrentRPGHealth;

	if (!FMath::IsNearlyZero(AppliedAmount))
	{
		OnRPGHealthChanged.Broadcast(
			CurrentRPGHealth,
			MaxRPGHealth
		);
	}

	if (CurrentRPGHealth <= SMALL_NUMBER)
	{
		DefeatRPGCharacter();
	}

	return AppliedAmount;
}


/* ================================================================== */
/* RPG combat state                                                    */
/* ================================================================== */

void AProjectBCharacter::SetRPGCombatState(
	EProjectBCombatState NewState
)
{
	if (RPGCombatState == NewState)
	{
		return;
	}

	const EProjectBCombatState PreviousState =
		RPGCombatState;

	RPGCombatState = NewState;

	/*
	 * Defeated/Downed characters should not receive normal movement
	 * input. This does not destroy the Actor.
	 */
	if (NewState == EProjectBCombatState::Downed ||
		NewState == EProjectBCombatState::Defeated)
	{
		bProjectBInputEnabled = false;
	}

	OnRPGCombatStateChanged.Broadcast(
		PreviousState,
		NewState
	);
}


void AProjectBCharacter::BeginRPGCombat()
{
	if (IsRPGDefeated())
	{
		return;
	}

	SetRPGCombatState(
		EProjectBCombatState::Combat
	);

	bProjectBInputEnabled = true;
}


void AProjectBCharacter::EndRPGCombat()
{
	if (IsRPGDefeated())
	{
		return;
	}

	SetRPGCombatState(
		EProjectBCombatState::Neutral
	);

	bProjectBInputEnabled = true;
}


void AProjectBCharacter::EnterRPGDowned()
{
	if (IsRPGDefeated())
	{
		return;
	}

	SetRPGCombatState(
		EProjectBCombatState::Downed
	);
}


void AProjectBCharacter::DefeatRPGCharacter()
{
	if (IsRPGDefeated())
	{
		return;
	}

	SetRPGCombatState(
		EProjectBCombatState::Defeated
	);
}


/* ================================================================== */
/* RPG abilities                                                       */
/* ================================================================== */

bool AProjectBCharacter::CanUseAbility_Implementation(
	FName AbilityId
) const
{
	/*
	 * Deliberately generic.
	 *
	 * jRPGTemplate / future ability system can override this in BP
	 * without this base class needing to know how an ability is stored.
	 */
	return !AbilityId.IsNone() &&
		CanReceivePlayerInput();
}


bool AProjectBCharacter::UseAbility_Implementation(
	FName AbilityId
)
{
	/*
	 * Base implementation intentionally does not execute an ability.
	 *
	 * This is the extension point for jRPGTemplate / Blueprint / later
	 * C++ ability code.
	 */
	return CanUseAbility(
		AbilityId
	);
}


bool AProjectBCharacter::CancelAbility_Implementation(
	FName AbilityId
)
{
	return !AbilityId.IsNone();
}


/* ================================================================== */
/* RPG inventory                                                       */
/* ================================================================== */

bool AProjectBCharacter::AddRPGItem_Implementation(
	FName ItemId,
	int32 Quantity
)
{
	/*
	 * No inventory storage is implemented here.
	 *
	 * jRPGTemplate's InventoryManager will eventually own the real
	 * inventory. This function exists as the project-level API.
	 */
	return !ItemId.IsNone() &&
		Quantity > 0;
}


bool AProjectBCharacter::RemoveRPGItem_Implementation(
	FName ItemId,
	int32 Quantity
)
{
	return !ItemId.IsNone() &&
		Quantity > 0;
}


bool AProjectBCharacter::HasRPGItem_Implementation(
	FName ItemId,
	int32 Quantity
) const
{
	/*
	 * No inventory storage in the base class yet.
	 *
	 * Return false until a real inventory backend is connected.
	 */
	return false;
}


/* ================================================================== */
/* RPG targeting                                                       */
/* ================================================================== */

void AProjectBCharacter::SetRPGTarget(
	AActor* NewTarget
)
{
	CurrentRPGTarget = NewTarget;
}


void AProjectBCharacter::ClearRPGTarget()
{
	CurrentRPGTarget = nullptr;
}


/* ================================================================== */
/* RPG component bridge                                                */
/* ================================================================== */

UActorComponent* AProjectBCharacter::FindRPGComponentByClass(
	TSubclassOf<UActorComponent> ComponentClass
) const
{
	if (!ComponentClass)
	{
		return nullptr;
	}

	return GetComponentByClass(
		ComponentClass
	);
}


/* ================================================================== */
/* Shooter API                                                         */
/* ================================================================== */

void AProjectBCharacter::DoStartFiring()
{
	if (!bShooterEnabled ||
		!CanReceivePlayerInput())
	{
		return;
	}

	if (IsValid(ProjectBCurrentWeapon))
	{
		ProjectBCurrentWeapon->StartFiring();
	}
}


void AProjectBCharacter::DoStopFiring()
{
	if (IsValid(ProjectBCurrentWeapon))
	{
		ProjectBCurrentWeapon->StopFiring();
	}
}


void AProjectBCharacter::DoSwitchWeapon()
{
	if (!bShooterEnabled ||
		!CanReceivePlayerInput())
	{
		return;
	}

	if (ProjectBOwnedWeapons.Num() <= 1)
	{
		return;
	}

	if (!IsValid(ProjectBCurrentWeapon))
	{
		return;
	}

	ProjectBCurrentWeapon->DeactivateWeapon();

	int32 WeaponIndex =
		ProjectBOwnedWeapons.Find(
			ProjectBCurrentWeapon
		);

	if (WeaponIndex == INDEX_NONE)
	{
		return;
	}

	if (WeaponIndex == ProjectBOwnedWeapons.Num() - 1)
	{
		WeaponIndex = 0;
	}
	else
	{
		++WeaponIndex;
	}

	ProjectBCurrentWeapon =
		ProjectBOwnedWeapons[WeaponIndex];

	if (IsValid(ProjectBCurrentWeapon))
	{
		ProjectBCurrentWeapon->ActivateWeapon(
			PlayerTag
		);
	}
}


void AProjectBCharacter::AddWeaponClass(
	const TSubclassOf<AShooterWeapon>& WeaponClass
)
{
	if (!bShooterEnabled ||
		!WeaponClass ||
		!GetWorld())
	{
		return;
	}

	if (FindProjectBWeaponOfType(WeaponClass))
	{
		return;
	}

	FActorSpawnParameters SpawnParams;

	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	SpawnParams.TransformScaleMethod =
		ESpawnActorScaleMethod::MultiplyWithRoot;

	AShooterWeapon* AddedWeapon =
		GetWorld()->SpawnActor<AShooterWeapon>(
			WeaponClass,
			GetActorTransform(),
			SpawnParams
		);

	if (!IsValid(AddedWeapon))
	{
		return;
	}

	ProjectBOwnedWeapons.Add(
		AddedWeapon
	);

	if (IsValid(ProjectBCurrentWeapon))
	{
		ProjectBCurrentWeapon->DeactivateWeapon();
	}

	ProjectBCurrentWeapon =
		AddedWeapon;

	ProjectBCurrentWeapon->ActivateWeapon(
		PlayerTag
	);
}


void AProjectBCharacter::AttachWeaponMeshes(
	AShooterWeapon* Weapon
)
{
	if (!IsValid(Weapon))
	{
		return;
	}

	const FAttachmentTransformRules AttachmentRule(
		EAttachmentRule::SnapToTarget,
		false
	);

	Weapon->AttachToActor(
		this,
		AttachmentRule
	);

	if (USkeletalMeshComponent* FirstPersonMeshComponent =
		GetFirstPersonMesh())
	{
		if (USkeletalMeshComponent* WeaponFirstPersonMesh =
			Weapon->GetFirstPersonMesh())
		{
			WeaponFirstPersonMesh->AttachToComponent(
				FirstPersonMeshComponent,
				AttachmentRule,
				FirstPersonWeaponSocket
			);
		}
	}

	if (USkeletalMeshComponent* ThirdPersonMesh =
		GetMesh())
	{
		if (USkeletalMeshComponent* WeaponThirdPersonMesh =
			Weapon->GetThirdPersonMesh())
		{
			WeaponThirdPersonMesh->AttachToComponent(
				ThirdPersonMesh,
				AttachmentRule,
				ThirdPersonWeaponSocket
			);
		}
	}
}


void AProjectBCharacter::OnWeaponActivated(
	AShooterWeapon* Weapon
)
{
	if (!IsValid(Weapon))
	{
		return;
	}

	UpdateWeaponHUD(
		Weapon->GetBulletCount(),
		Weapon->GetMagazineSize()
	);

	/*
	 * This preserves the old ShooterWeapon API.
	 *
	 * Later, when the player uses a MetaHuman animation setup,
	 * this can be replaced by a more appropriate animation layer
	 * rather than directly replacing the whole AnimInstance.
	 */

	if (USkeletalMeshComponent* FirstPersonMeshComponent =
		GetFirstPersonMesh())
	{
		if (TSubclassOf<UAnimInstance> AnimClass =
			Weapon->GetFirstPersonAnimInstanceClass())
		{
			FirstPersonMeshComponent->SetAnimInstanceClass(
				AnimClass
			);
		}
	}

	if (USkeletalMeshComponent* ThirdPersonMesh =
		GetMesh())
	{
		if (TSubclassOf<UAnimInstance> AnimClass =
			Weapon->GetThirdPersonAnimInstanceClass())
		{
			ThirdPersonMesh->SetAnimInstanceClass(
				AnimClass
			);
		}
	}
}


void AProjectBCharacter::OnWeaponDeactivated(
	AShooterWeapon* Weapon
)
{
	/*
	 * Extension point.
	 *
	 * Currently no common cleanup is necessary.
	 */
}


void AProjectBCharacter::OnSemiWeaponRefire()
{
	/*
	 * Extension point for semi-automatic weapons.
	 */
}


void AProjectBCharacter::PlayFiringMontage(
	UAnimMontage* Montage
)
{
	if (!Montage)
	{
		return;
	}

	/*
	 * First-person firing montage.
	 *
	 * The old ShooterCharacter left this as a stub, so this is now
	 * a minimal useful implementation.
	 */
	if (USkeletalMeshComponent* FirstPersonMeshComponent =
		GetFirstPersonMesh())
	{
		if (UAnimInstance* AnimInstance =
			FirstPersonMeshComponent->GetAnimInstance())
		{
			AnimInstance->Montage_Play(
				Montage
			);
		}
	}
}


void AProjectBCharacter::AddWeaponRecoil(
	float Recoil
)
{
	if (!CanReceivePlayerInput())
	{
		return;
	}

	AddControllerPitchInput(
		Recoil
	);
}


void AProjectBCharacter::UpdateWeaponHUD(
	int32 CurrentAmmo,
	int32 MagazineSize
)
{
	OnShooterAmmoChanged.Broadcast(
		MagazineSize,
		CurrentAmmo
	);
}


FVector AProjectBCharacter::GetWeaponTargetLocation() const
{
	if (!IsValid(FirstPersonCameraComponent))
	{
		return GetActorLocation() +
			GetActorForwardVector() *
			MaxAimDistance;
	}

	FHitResult HitResult;

	const FVector Start =
		FirstPersonCameraComponent->GetComponentLocation();

	const FVector End =
		Start +
		FirstPersonCameraComponent->GetForwardVector() *
		MaxAimDistance;

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(ProjectBWeaponAim),
		false
	);

	QueryParams.AddIgnoredActor(
		this
	);

	if (GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Visibility,
		QueryParams
	))
	{
		return HitResult.ImpactPoint;
	}

	return End;
}


/* ================================================================== */
/* Shooter helpers                                                     */
/* ================================================================== */
void AProjectBCharacter::SetShooterEnabled(bool bEnabled)
{
	bShooterEnabled = bEnabled;

	if (!bShooterEnabled && IsValid(ProjectBCurrentWeapon))
	{
		ProjectBCurrentWeapon->StopFiring();
	}
}

TArray<AShooterWeapon*> AProjectBCharacter::GetProjectBOwnedWeapons() const
{
	TArray<AShooterWeapon*> Result;
	Result.Reserve(ProjectBOwnedWeapons.Num());

	for (const TObjectPtr<AShooterWeapon>& Weapon : ProjectBOwnedWeapons)
	{
		if (IsValid(Weapon))
		{
			Result.Add(Weapon.Get());
		}
	}

	return Result;
}

AShooterWeapon* AProjectBCharacter::FindProjectBWeaponOfType(
	TSubclassOf<AShooterWeapon> WeaponClass
) const
{
	if (!WeaponClass)
	{
		return nullptr;
	}

	for (AShooterWeapon* Weapon : ProjectBOwnedWeapons)
	{
		if (IsValid(Weapon) &&
			Weapon->IsA(WeaponClass))
		{
			return Weapon;
		}
	}

	return nullptr;
}


/* ================================================================== */
/* First-person bridge                                                 */
/* ================================================================== */

USkeletalMeshComponent*
AProjectBCharacter::GetFirstPersonMesh() const
{
	/*
	 * GetMesh1P() is protected inside APBPlayerCharacter.
	 * AProjectBCharacter is a derived class, so it can expose it.
	 *
	 * This is the compatibility bridge used by ShooterWeapon code.
	 */
	return GetMesh1P();
}


UCameraComponent*
AProjectBCharacter::GetFirstPersonCameraComponent() const
{
	return FirstPersonCameraComponent;
}