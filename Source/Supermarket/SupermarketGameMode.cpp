// Copyright Epic Games, Inc. All Rights Reserved.

#include "SupermarketGameMode.h"
#include "SupermarketCharacter.h"
#include "UObject/ConstructorHelpers.h"

ASupermarketGameMode::ASupermarketGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
