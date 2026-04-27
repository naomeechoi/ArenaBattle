// Fill out your copyright notice in the Description page of Project Settings.


#include "GameData/ABGameSingleton.h"

UABGameSingleton::UABGameSingleton()
{
	static ConstructorHelpers::FObjectFinder<UDataTable> DataTableRef(
		TEXT("/Game/ArenaBattle/GameData/ABCharacterStatTable.ABCharacterStatTable")
	);

	if (DataTableRef.Succeeded())
	{
		const UDataTable* DataTable = DataTableRef.Object;
		ensureAlways(DataTable->GetRowMap().Num() > 0);

		TArray<uint8*> ValueArray;
		DataTable->GetRowMap().GenerateValueArray(ValueArray);

		Algo::Transform(ValueArray, CharacterStatTable, [](uint8* Value) {
			return *reinterpret_cast<FABCharacterStat*>(Value);
		});

		CharacterMaxLevel = CharacterStatTable.Num();
		ensureAlways(CharacterMaxLevel > 0);
	}
}


UABGameSingleton& UABGameSingleton::Get()
{
	UABGameSingleton* Singleton = CastChecked<UABGameSingleton>(GEngine->GameSingleton);

	if (Singleton)
		return *Singleton;

	UE_LOG(LogTemp, Error, TEXT("Invalid Game Singleton"));
	return *NewObject<UABGameSingleton>();
}