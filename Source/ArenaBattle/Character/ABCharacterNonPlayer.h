// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/ABCharacterBase.h"
#include "Engine/StreamableManager.h"
#include "Interface/ABCharacterAIInterface.h"
#include "ABCharacterNonPlayer.generated.h"

/**
 * 
 */
UCLASS(config=ArenaBattle)
class ARENABATTLE_API AABCharacterNonPlayer
	: public AABCharacterBase, public IABCharacterAIInterface
{
	GENERATED_BODY()

public:
	AABCharacterNonPlayer();

protected:
	virtual void PostInitializeComponents() override;

	void NPCMeshLoadCompleted();

	virtual void SetDead() override;
	
	UPROPERTY(config)
	TArray<FSoftObjectPath> NPCMeshes;

	TSharedPtr<FStreamableHandle> NPCMeshHandle;

	// IABCharacterAIInterface을(를) 통해 상속됨
	virtual float GetAIPatrolRadius() override;
	virtual float GetAIDetectRange() override;
	virtual float GetAIAttackRange() override;
	virtual float GetAITurnSpeed() override;
};
