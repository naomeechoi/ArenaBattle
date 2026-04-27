// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ABCharacterNonPlayer.h"
#include "Engine/AssetManager.h"

AABCharacterNonPlayer::AABCharacterNonPlayer()
{
	GetMesh()->SetHiddenInGame(true);
}

void AABCharacterNonPlayer::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	int32 MeshNum = NPCMeshes.Num();
	ensureAlways(MeshNum);

	int RandIndex = FMath::RandRange(0, MeshNum - 1);

	NPCMeshHandle = UAssetManager::Get().GetStreamableManager().RequestAsyncLoad(
		NPCMeshes[RandIndex],
		FStreamableDelegate::CreateUObject(this, &AABCharacterNonPlayer::NPCMeshLoadCompleted)
	);
}

void AABCharacterNonPlayer::NPCMeshLoadCompleted()
{
	if (!NPCMeshHandle.IsValid())
	{
		return;
	}

	USkeletalMesh* NPCMesh = Cast<USkeletalMesh>(NPCMeshHandle->GetLoadedAsset());
	if (!NPCMesh)
	{
		return;
	}

	GetMesh()->SetSkeletalMesh(NPCMesh);
	GetMesh()->SetHiddenInGame(false);

	NPCMeshHandle->ReleaseHandle();
}

void AABCharacterNonPlayer::SetDead()
{
	Super::SetDead();

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateLambda(
		[&]() {
			Destroy();
		}), DeadEventDelayTime, false);
}
