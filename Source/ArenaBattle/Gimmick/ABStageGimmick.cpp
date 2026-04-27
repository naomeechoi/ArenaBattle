#include "ABStageGimmick.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Physics/ABCollision.h"
#include "Engine/OverlapResult.h"
#include "Character/ABCharacterNonPlayer.h"
#include "Item/ABItemBox.h"


// Sets default values
AABStageGimmick::AABStageGimmick()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Stage = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Stage"));
	RootComponent = Stage;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> StageMeshRef(
		TEXT("/Game/ArenaBattle/Environment/Stages/SM_SQUARE.SM_SQUARE")
	);
	if(StageMeshRef.Succeeded())
	{
		Stage->SetStaticMesh(StageMeshRef.Object);
	}

	StageTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("StageTrigger"));
	StageTrigger->SetBoxExtent(FVector(775.0f, 775.0f, 300.0f));
	StageTrigger->SetupAttachment(Stage);
	StageTrigger->SetRelativeLocation(FVector(0.0f, 0.0f, 250.0f));
	StageTrigger->SetCollisionProfileName(CPROFILE_ABTRIGGER);
	StageTrigger->OnComponentBeginOverlap.AddDynamic(this, &AABStageGimmick::OnStageTriggerBeginOverlap);

	static FName GateSockets[] =
	{
		TEXT("+XGate"),
		TEXT("-XGate"),
		TEXT("+YGate"),
		TEXT("-YGate"),
	};
	static ConstructorHelpers::FObjectFinder<UStaticMesh> GateMeshRef(
		TEXT("/Game/ArenaBattle/Environment/Props/SM_GATE.SM_GATE")
	);

	if (!GateMeshRef.Succeeded())
		return;

	for (FName GateSocket : GateSockets)
	{
		UStaticMeshComponent* Gate = CreateDefaultSubobject<UStaticMeshComponent>(GateSocket);
		Gate->SetStaticMesh(GateMeshRef.Object);
		Gate->SetupAttachment(Stage, GateSocket);
		Gate->SetRelativeLocationAndRotation(FVector(0.0f, -80.0f, 0.0f), FRotator(0.0f, -90.0f, 0.0f));
		Gates.Add(GateSocket, Gate);

		FName TriggerName = *GateSocket.ToString().Append("Trigger");
		UBoxComponent* GateTrigger = CreateDefaultSubobject<UBoxComponent>(TriggerName);
		GateTrigger->SetBoxExtent(FVector(100.0f, 100.0f, 300.0f));
		GateTrigger->SetupAttachment(Stage, GateSocket);
		GateTrigger->SetRelativeLocation(FVector(70.0f, 0.0f, 250.0f));
		GateTrigger->SetCollisionProfileName(CPROFILE_ABTRIGGER);
		GateTrigger->OnComponentBeginOverlap.AddDynamic(this, &AABStageGimmick::OnGateTriggerBeginOverlap);
		GateTrigger->ComponentTags.Add(GateSocket);
		GateTriggers.Add(GateTrigger);
	}

	CurrentState = EStageState::Ready;
	StateChangedActions.Add(EStageState::Ready, FOnStateChangedDelegate::CreateUObject(this, &AABStageGimmick::SetReady));
	StateChangedActions.Add(EStageState::Fight, FOnStateChangedDelegate::CreateUObject(this, &AABStageGimmick::SetFight));
	StateChangedActions.Add(EStageState::Reward, FOnStateChangedDelegate::CreateUObject(this, &AABStageGimmick::SetChooseReward));
	StateChangedActions.Add(EStageState::Next, FOnStateChangedDelegate::CreateUObject(this, &AABStageGimmick::SetChooseNext));

	OpponentSpawnTime = 2.0f;
	OpponentClass = AABCharacterNonPlayer::StaticClass();

	RewardBoxClass = AABItemBox::StaticClass();
	for (FName GateSocket : GateSockets)
	{
		FVector BoxLocation = Stage->GetSocketLocation(GateSocket) / 2;
		RewardBoxLocations.Add(GateSocket, BoxLocation);
	}
}

void AABStageGimmick::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SetState(CurrentState);
}

void AABStageGimmick::OnStageTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	SetState(EStageState::Fight);
}

void AABStageGimmick::OnGateTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (OverlappedComponent->ComponentTags.Num() <= 0)
		return;

	FName ComponentTag = OverlappedComponent->ComponentTags[0];
	FName SocketName = FName(*ComponentTag.ToString().Left(2));

	if (!Stage->DoesSocketExist(SocketName))
		return;

	FVector NewLocation = Stage->GetSocketLocation(SocketName);

	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GateTrigger), false, this);
	bool Result = GetWorld()->OverlapMultiByObjectType(OverlapResults, NewLocation, FQuat::Identity, FCollisionObjectQueryParams::InitType::AllStaticObjects,
		FCollisionShape::MakeSphere(775.0f), Params);

	if (Result)
		return;

	const FTransform SpawnTransform(NewLocation);
	AABStageGimmick* NewGimmick = GetWorld()->SpawnActorDeferred<AABStageGimmick>(AABStageGimmick::StaticClass(), SpawnTransform);
	if (NewGimmick)
	{
		NewGimmick->SetStageNum(CurrentStageNum + 1);
		NewGimmick->FinishSpawning(SpawnTransform);
	}
}

void AABStageGimmick::OpenAllGates()
{
	for (auto Gate : Gates)
	{
		Gate.Value->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	}
}

void AABStageGimmick::CloseAllGates()
{
	for (auto Gate : Gates)
	{
		Gate.Value->SetRelativeRotation(FRotator::ZeroRotator);
	}
}

void AABStageGimmick::SetState(EStageState InNewState)
{
	CurrentState = InNewState;
	if (StateChangedActions.Contains(InNewState))
	{
		StateChangedActions[InNewState].ExecuteIfBound();
	}
}

void AABStageGimmick::SetReady()
{
	StageTrigger->SetCollisionProfileName(CPROFILE_ABTRIGGER);
	for (auto GateTrigger : GateTriggers)
	{
		GateTrigger->SetCollisionProfileName(CPROFILE_NOCOLLISION);
	}
	OpenAllGates();
}

void AABStageGimmick::SetFight()
{
	StageTrigger->SetCollisionProfileName(CPROFILE_NOCOLLISION);
	for (auto GateTrigger : GateTriggers)
	{
		GateTrigger->SetCollisionProfileName(CPROFILE_NOCOLLISION);
	}
	CloseAllGates();
	GetWorld()->GetTimerManager().SetTimer(
		OpponentSpawnTimerHandle,
		this,
		&AABStageGimmick::OnOpponentSpawn,
		OpponentSpawnTime,
		false
	);
}

void AABStageGimmick::SetChooseReward()
{
	StageTrigger->SetCollisionProfileName(CPROFILE_NOCOLLISION);
	for (auto GateTrigger : GateTriggers)
	{
		GateTrigger->SetCollisionProfileName(CPROFILE_NOCOLLISION);
	}
	CloseAllGates();
	SpawnRewardBoxes();
}

void AABStageGimmick::SetChooseNext()
{
	StageTrigger->SetCollisionProfileName(CPROFILE_NOCOLLISION);
	for (auto GateTrigger : GateTriggers)
	{
		GateTrigger->SetCollisionProfileName(CPROFILE_ABTRIGGER);
	}
	OpenAllGates();
}

void AABStageGimmick::OnOpponentDestroyed(AActor* DestroyActor)
{
	SetState(EStageState::Reward);
}

void AABStageGimmick::OnOpponentSpawn()
{
	const FVector SpawnLocation = GetActorLocation() + FVector::UpVector * 88.0f;
	const FTransform SpawnTransform(SpawnLocation);

	AABCharacterNonPlayer* OpponentCharacter
		= GetWorld()->SpawnActorDeferred<AABCharacterNonPlayer>(
			OpponentClass,
			SpawnTransform
		);

	if (!OpponentCharacter)
	{
		return;
	}

	OpponentCharacter->OnDestroyed.AddDynamic(this, &AABStageGimmick::OnOpponentDestroyed);
	OpponentCharacter->SetLevel(CurrentStageNum);
	OpponentCharacter->FinishSpawning(SpawnTransform);
}

void AABStageGimmick::OnRewardTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	for (const auto& RewardBox : RewardBoxes)
	{
		if (RewardBox.IsValid())
		{
			AABItemBox* ValidItemBox = RewardBox.Get();
			AActor* OveralppedActor = OverlappedComponent->GetOwner();
			if (!OveralppedActor)
				continue;

			if (OveralppedActor != ValidItemBox)
			{
				ValidItemBox->Destroy();
			}
		}
	}

	SetState(EStageState::Next);
}

void AABStageGimmick::SpawnRewardBoxes()
{
	for (const auto& RewardBoxLocation : RewardBoxLocations)
	{
		FVector WorldSpawnLocation = GetActorLocation() + RewardBoxLocation.Value + FVector(0.0f, 0.0f, 30.0f);
		const FTransform SpawnTransform(WorldSpawnLocation);


		AABItemBox* RewardBoxActor = GetWorld()->SpawnActorDeferred<AABItemBox>(RewardBoxClass, SpawnTransform);

		if (!RewardBoxActor)
			continue;

		RewardBoxActor->Tags.Add(RewardBoxLocation.Key);
		RewardBoxActor->GetTrigger()->OnComponentBeginOverlap.AddDynamic(this, &AABStageGimmick::OnRewardTriggerBeginOverlap);
		RewardBoxes.Add(RewardBoxActor);

		RewardBoxActor->FinishSpawning(SpawnTransform);
	}
}
