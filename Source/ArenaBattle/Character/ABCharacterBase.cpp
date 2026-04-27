// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ABCharacterBase.h"
#include "ABCharacterControlData.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ABComboActionData.h"
#include "Components/CapsuleComponent.h"
#include "Physics/ABCollision.h"
#include "Engine/DamageEvents.h"

#include "CharacterStat/ABCharacterStatComponent.h"
#include "UI/ABWidgetComponent.h"
#include "UI/ABHpBarWidget.h"

#include "Item/ABItemData.h"
#include "Item/ABWeaponItemData.h"

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

	static ConstructorHelpers::FObjectFinder<UAnimMontage> DeadMontageRef(TEXT("/Game/ArenaBattle/Animation/AM_Dead.AM_Dead"));
	if (DeadMontageRef.Succeeded())
	{
		DeadMontage = DeadMontageRef.Object;
	}

	Stat = CreateDefaultSubobject<UABCharacterStatComponent>(TEXT("Stat"));
	HpBar = CreateDefaultSubobject<UABWidgetComponent>(TEXT("Widget"));
	Weapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Weapon"));

	if (!Stat || !HpBar || !Weapon)
		return;

	HpBar->SetupAttachment(GetMesh());
	HpBar->SetRelativeLocation(FVector(0.0f, 0.0f, 180.0f));

	static ConstructorHelpers::FClassFinder<UUserWidget> HpBarWidgetRef(
		TEXT("/Game/ArenaBattle/UI/WBP_HpBar.WBP_HpBar_C")
	);

	if (HpBarWidgetRef.Succeeded())
	{
		HpBar->SetWidgetClass(HpBarWidgetRef.Class);
		HpBar->SetWidgetSpace(EWidgetSpace::Screen);
		HpBar->SetDrawSize(FVector2D(150.0f, 15.0f));
		HpBar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	TakeItemActions.Add(FOnTakeItemDelegate::CreateUObject(this, &AABCharacterBase::EquipWeapon));
	TakeItemActions.Add(FOnTakeItemDelegate::CreateUObject(this, &AABCharacterBase::DrinkPotion));
	TakeItemActions.Add(FOnTakeItemDelegate::CreateUObject(this, &AABCharacterBase::ReadScroll));

	Weapon->SetupAttachment(GetMesh(), TEXT("hand_rSocket"));
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
	UE_LOG(LogTemp, Log, TEXT("ProcessComboCommand"));

	if (CurrentCombo == 0)
	{
		ComboActionBegin();
		return;
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

	const float AttackSpeedRate = Stat->GetTotalStat().AttackSpeed;
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

	const float AttackSpeedRate = Stat->GetTotalStat().AttackSpeed;

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

void AABCharacterBase::SetUpCharacterWidget(UABUserWidget* InUserWidget)
{
	UABHpBarWidget* HpBarWidget = Cast<UABHpBarWidget>(InUserWidget);
	if (!HpBarWidget)
		return;

	HpBarWidget->SetMaxHp(Stat->GetTotalStat().MaxHp);
	HpBarWidget->UpdateHpBar(Stat->GetCurrentHp());
	Stat->OnHpChanged.AddUObject(HpBarWidget, &UABHpBarWidget::UpdateHpBar);
}

void AABCharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	Stat->OnHpZero.AddUObject(this, &AABCharacterBase::SetDead);
}

void AABCharacterBase::TakeItem(UABItemData* InItemData)
{
	if (!InItemData)
		return;

	TakeItemActions[(uint8)InItemData->Type].ExecuteIfBound(InItemData);
}

void AABCharacterBase::DrinkPotion(UABItemData* InItemData)
{
}

void AABCharacterBase::EquipWeapon(UABItemData* InItemData)
{
	UABWeaponItemData* WeaponItemData = Cast<UABWeaponItemData>(InItemData);
	if (WeaponItemData)
	{
		if (WeaponItemData->WeaponMesh.IsPending())
			WeaponItemData->WeaponMesh.LoadSynchronous();
			
		Weapon->SetSkeletalMesh(WeaponItemData->WeaponMesh.Get());

		Stat->SetModifierStat(WeaponItemData->ModifierStat);
	}
}

void AABCharacterBase::ReadScroll(UABItemData* InItemData)
{
}

void AABCharacterBase::AttackHitCheck()
{
	const float AttackRange = Stat->GetTotalStat().AttackRange;
	const float AttackRadius = 50.0f;
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
		const float AttackDamage = Stat->GetTotalStat().Attack;
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
	//SetDead();
	Stat->ApplyDamage(DamageAmount);
	return DamageAmount;
}

void AABCharacterBase::SetDead()
{
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
	PlayDeadAnimation();
	SetActorEnableCollision(false);
	HpBar->SetHiddenInGame(true);
}

void AABCharacterBase::PlayDeadAnimation()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance)
		return;

	AnimInstance->StopAllMontages(0.0f);
	const float DeadSpeedRate = 1.0f;
	AnimInstance->Montage_Play(DeadMontage, DeadSpeedRate);
}


int32 AABCharacterBase::GetLevel() const
{
	return Stat->GetCurrentLevel();
}

void AABCharacterBase::SetLevel(int32 InNewLevel)
{
	Stat->SetLevelStat(InNewLevel);
}
