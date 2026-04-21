// Fill out your copyright notice in the Description page of Project Settings.


#include "Props/ABFountain.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AABFountain::AABFountain()
{
 	// Set this actor to call Tick() evert frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	if (Body)
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> FountainBodyMesh(
			TEXT("/Game/ArenaBattle/Environment/Props/SM_Plains_Castle_Fountain_01.SM_Plains_Castle_Fountain_01")
		);

		if (FountainBodyMesh.Succeeded())
			Body->SetStaticMesh(FountainBodyMesh.Object);

		RootComponent = Body;
	};

	Water = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Water"));
	if (Water)
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> FountainWaterMesh(
			TEXT("/Game/ArenaBattle/Environment/Props/SM_Plains_Fountain_02.SM_Plains_Fountain_02")
		);

		if (FountainWaterMesh.Succeeded())
			Water->SetStaticMesh(FountainWaterMesh.Object);

		Water->SetupAttachment(RootComponent);
		Water->SetRelativeLocation(FVector(0.0f, 0.0f, 132.0f));
	};
}

// Called when the game starts or when spawned
void AABFountain::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AABFountain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

