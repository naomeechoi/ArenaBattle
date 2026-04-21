// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ABCharacterBase.h"
#include "ABCharacterControlData.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ABComboActionData.h"
#include "Components/CapsuleComponent.h"
#include "Physics/ABCollision.h"
#include "Engine/DamageEvents.h"

// Sets default values
AABCharacterBase::AABCharacterBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->SetCollisionProfileName(CPROFILE_ABCAPSULE);

	GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -88.0f), FRotator(0.0f, -90.0f, 0.0f));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> CharacMesh(
		TEXT("/Game/InfinityBladeWarriors/Character/CompleteCharacters/SK_CharM_Cardboard.SK_CharM_Cardboard")
	);

	if (CharacMesh.Succeeded())
		GetMesh()->SetSkeletalMesh(CharacMesh.Object);

	static ConstructorHelpers::FClassFinder<UAnimInstance> CharacterAnim(
		TEXT("/Game/ArenaBattle/Animation/ABP_ABCharacter.ABP_ABCharacter_C")
	);

	if (CharacterAnim.Succeeded())
		GetMesh()->SetAnimInstanceClass(CharacterAnim.Class);

	GetMesh()->SetCollisionProfileName(TEXT("NoCollision"));

	static ConstructorHelpers::FObjectFinder<UABCharacterControlData> ShoulderDataRef(TEXT("/Game/ArenaBattle/CharacterControl/ABC_Shoulder.ABC_Shoulder"));
	if (ShoulderDataRef.Succeeded())
	{
		CharacterControlManager.Add(ECharacterControlType::Shoulder, ShoulderDataRef.Object);
	}

	static ConstructorHelpers::FObjectFinder<UABCharacterControlData> QuarterDataRef(TEXT("/Game/ArenaBattle/CharacterControl/ABC_Quater.ABC_Quater"));
	if (QuarterDataRef.Succeeded())
	{
		CharacterControlManager.Add(ECharacterControlType::Quarter, QuarterDataRef.Object);
	}

	static ConstructorHelpers::FObjectFinder<UAnimMontage> ComboAttackMontageRef(TEXT("/Game/ArenaBattle/Animation/AM_ComboAttack.AM_ComboAttack"));
	if (ComboAttackMontageRef.Succeeded())
	{
		ComboAttackMontage = ComboAttackMontageRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UABComboActionData> ComboActionDataRef(TEXT("/Game/ArenaBattle/ComboData/ABA_ComboAction.ABA_ComboAction"));
	if (ComboActionDataRef.Succeeded())
	{
		ComboActionData = ComboActionDataRef.Object;
	}
}

void AABCharacterBase::SetCharacterControlData(const UABCharacterControlData* InCharacterControlData)
{
	bUseControllerRotationYaw = InCharacterControlData->bUseControllerRotationYaw;

	GetCharacterMovement()->bUseControllerDesiredRotation = InCharacterControlData->bUseControllerDesiredRotation;
	GetCharacterMovement()->bOrientRotationToMovement = InCharacterControlData->bOrientRotationToMovement;
	GetCharacterMovement()->RotationRate = InCharacterControlData->RotationRate;
}

void AABCharacterBase::ProcessComboCommand()
{
	if (CurrentCombo == 0)
	{
		ComboActionBegin();
	}

	if (ComboTimerHandle.IsValid())
	{
		bHasNextComboCommand = true;
	}
	else
	{
		bHasNextComboCommand = false;
	}
}

void AABCharacterBase::ComboActionBegin()
{
	CurrentCombo = 1;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(!AnimInstance)
		return;

	const float AttackSpeedRate = 1.0f;
	AnimInstance->Montage_Play(ComboAttackMontage, AttackSpeedRate);

	FOnMontageEnded OnMontageEnded;
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);

	OnMontageEnded.BindUObject(this, &AABCharacterBase::ComboActionEnd);
	AnimInstance->Montage_SetEndDelegate(OnMontageEnded, ComboAttackMontage);
	
	ComboTimerHandle.Invalidate();
	SetComboChcekTimer();
}

void AABCharacterBase::ComboActionEnd(UAnimMontage* TargetMontage, bool bInterrupted)
{
	ensureAlways(CurrentCombo > 0);
	CurrentCombo = 0;
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
}

void AABCharacterBase::SetComboChcekTimer()
{
	const int32 ComboIndex = CurrentCombo - 1;

	if (!ComboActionData)
		return;

	ensureAlways(ComboActionData->EffectiveFrameCount.IsValidIndex(ComboIndex));

	const float AttackSpeedRate = 1.0f;

	float ComboEffectTime = (ComboActionData->EffectiveFrameCount[ComboIndex] / ComboActionData->FrameRate) / AttackSpeedRate;

	if (ComboEffectTime <= 0)
		return;

	GetWorld()->GetTimerManager().SetTimer(
		ComboTimerHandle,
		this,
		&AABCharacterBase::ComboCheck,
		ComboEffectTime,
		false
	);
}

void AABCharacterBase::ComboCheck()
{
	ComboTimerHandle.Invalidate();

	if (!bHasNextComboCommand)
		return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance || !ComboActionData)
		return;

	CurrentCombo = FMath::Clamp(CurrentCombo + 1, 1, ComboActionData->MaxComboCount);
	FName NextSection = *FString::Printf(TEXT("%s%d"), *ComboActionData->MontageSectionNamePrefix, CurrentCombo);
	AnimInstance->Montage_JumpToSection(NextSection, ComboAttackMontage);

	SetComboChcekTimer();
	bHasNextComboCommand = false;
}

void AABCharacterBase::AttackHitCheck()
{
	const float AttackRange = 200.0f;
	const float AttackRadius = 30.0f;
	FCollisionQueryParams Params(
		SCENE_QUERY_STAT(Attack),
		false,
		this
	);

	FHitResult OutHitResult;
	FVector Start = GetActorLocation() + GetActorForwardVector() * GetCapsuleComponent()->GetScaledCapsuleRadius();
	FVector End = Start + GetActorForwardVector() * AttackRange;
	bool HitDetected = GetWorld()->SweepSingleByChannel(
		OutHitResult, Start, End, FQuat::Identity, CCHANNEL_ABACTION, FCollisionShape::MakeSphere(AttackRadius), Params);

	if (HitDetected)
	{
		const float AttackDamage = 30.0f;
		FDamageEvent DamageEvent;
		OutHitResult.GetActor()->TakeDamage(AttackDamage, DamageEvent, GetController(), this);
	}

#if ENABLE_DRAW_DEBUG
	FVector CapsuleOrigin = Start + (End - Start) * 0.5f;
	float CapsuleHalfHeight = AttackRange * 0.5f;
	FColor DrawColor = HitDetected ? FColor::Green : FColor::Red;

	DrawDebugCapsule(
		GetWorld(),
		CapsuleOrigin,
		CapsuleHalfHeight,
		AttackRadius,
		FRotationMatrix::MakeFromZ(GetActorForwardVector()).ToQuat(),
		DrawColor,
		false,
		5.0f
	);
#endif
}

float AABCharacterBase::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	SetDead();
	return DamageAmount;
}

void AABCharacterBase::SetDead()
{
}

void AABCharacterBase::PlayDeadAnimation()
{
}

