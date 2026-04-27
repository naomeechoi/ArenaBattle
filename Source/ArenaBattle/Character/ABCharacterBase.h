// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interface/ABAnimationAttackInterface.h"
#include "Interface/ABCharacterWidgetInterface.h"
#include "Interface/ABCharacterItemInterface.h"
#include "ABCharacterBase.generated.h"

UENUM()
enum class ECharacterControlType : uint8
{
	Shoulder,
	Quarter,
};
DECLARE_DELEGATE_OneParam(FOnTakeItemDelegate, class UABItemData*)

UCLASS()
class ARENABATTLE_API AABCharacterBase
	: public ACharacter,
	public IABAnimationAttackInterface, public IABCharacterWidgetInterface, public IABCharacterItemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AABCharacterBase();

	// IABAnimationAttackInterface을(를) 통해 상속됨
	virtual void AttackHitCheck() override;

	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser) override;

protected:
	virtual void SetDead();
	void PlayDeadAnimation();

protected:
	virtual void SetCharacterControlData(const class UABCharacterControlData* InCharacterControlData);

	void ProcessComboCommand();
	void ComboActionBegin();
	void ComboActionEnd(UAnimMontage* TargetMontage, bool bInterrupted);
	void SetComboChcekTimer();
	void ComboCheck();
	
	// IABCharacterItemInterface을(를) 통해 상속됨
	virtual void TakeItem(UABItemData* InItemData) override;
	virtual void DrinkPotion(UABItemData* InItemData);
	virtual void EquipWeapon(UABItemData* InItemData);
	virtual void ReadScroll(UABItemData* InItemData);

	// from IABCharacterWidgetInterface 
	virtual void SetUpCharacterWidget(UABUserWidget* InUserWidget) override;

	virtual void PostInitializeComponents() override;

protected:
	UPROPERTY(EditAnywhere, Category = CharacterControl)
	TMap<ECharacterControlType, class UABCharacterControlData*> CharacterControlManager;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attack)
	TObjectPtr<class UAnimMontage> ComboAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attack)
	TObjectPtr<class UABComboActionData> ComboActionData;

	UPROPERTY(EditAnywhere, Category = Attack)
	uint32 CurrentCombo = 0;

	FTimerHandle ComboTimerHandle;

	UPROPERTY(VisibleAnywhere, Category = Attack)
	bool bHasNextComboCommand = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dead)
	TObjectPtr<class UAnimMontage> DeadMontage;

	float DeadEventDelayTime = 5.0f;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Stat)
	TObjectPtr<class UABCharacterStatComponent> Stat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Widget)
	TObjectPtr<class UABWidgetComponent> HpBar;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Equipment)
	TObjectPtr<class USkeletalMeshComponent> Weapon;

protected:
	TArray<FOnTakeItemDelegate> TakeItemActions;

public:
	int32 GetLevel() const;
	void SetLevel(int32 InNewLevel);

};
