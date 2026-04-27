// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ABCharacterStat.h"
#include "ABGameSingleton.generated.h"

/**
 * 
 */
UCLASS()
class ARENABATTLE_API UABGameSingleton : public UObject
{
	GENERATED_BODY()
	
public:
	UABGameSingleton();

	FORCEINLINE FABCharacterStat GetCharacterStat(int32 InLevel) const
	{
		int32 level = InLevel - 1;
		return CharacterStatTable.IsValidIndex(level) == true ? CharacterStatTable[level] : FABCharacterStat();
	}

	static UABGameSingleton& Get();

	UPROPERTY()
	int32 CharacterMaxLevel;

private:
	TArray<FABCharacterStat> CharacterStatTable;
};
