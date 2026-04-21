// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ABCharacterBase.h"
#include "ABCharacterControlData.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ABComboActionData.h"

// Sets default values
AABCharacterBase::AABCharacterBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

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
}

void AABCharacterBase::ComboActionBegin()
{
	CurrentCombo = 1;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(!AnimInstance)
		return;

	const float AttackSpeedRate = 1.0f;
	AnimInstance->Montage_Play(ComboAttackMontage);

	FOnMontageEnded OnMontageEnded;
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);

	OnMontageEnded.BindUObject(this, &AABCharacterBase::ComboActionEnd);
	AnimInstance->Montage_SetEndDelegate(OnMontageEnded, ComboAttackMontage);
	
	ComboTimerHandle.Invalidate();
	SetComboChcekTimer();
}

void AABCharacterBase::ComboActionEnd(UAnimMontage* TargetMontage, bool bInterrupted)
{
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
}

void AABCharacterBase::SetComboChcekTimer()
{
	const int32 ComboIndex = CurrentCombo - 1;

	if (ComboActionData)
		ensureAlways(ComboActionData->EffectiveFrameCount.IsValidIndex(ComboIndex));
}

void AABCharacterBase::ComboCheck()
{
}

