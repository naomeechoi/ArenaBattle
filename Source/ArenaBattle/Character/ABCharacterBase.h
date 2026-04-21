// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ABCharacterBase.generated.h"


UENUM()
enum class ECharacterControlType : uint8
{
	Shoulder,
	Quarter,
};

UCLASS()
class ARENABATTLE_API AABCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AABCharacterBase();

protected:
	virtual void SetCharacterControlData(const class UABCharacterControlData* InCharacterControlData);

	void ProcessComboCommand();
	void ComboActionBegin();
	void ComboActionEnd(UAnimMontage* TargetMontage, bool bInterrupted);
	void SetComboChcekTimer();
	void ComboCheck();

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
};
