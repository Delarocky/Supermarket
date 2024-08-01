// MenuBackgroundManager.cpp
#include "MenuBackgroundManager.h"
#include "GameFramework/PlayerController.h"

AMenuBackgroundManager::AMenuBackgroundManager()
{
    PrimaryActorTick.bCanEverTick = true;

    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    RootComponent = RootSceneComponent;

    CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
    CameraComponent->SetupAttachment(RootComponent);

    CurrentPositionIndex = 0;
    TimeUntilNextSwitch = DisplayDuration;
    bIsCycling = false;
}

void AMenuBackgroundManager::BeginPlay()
{
    Super::BeginPlay();

    // Set this pawn to be controlled by the default player controller
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        PC->Possess(this);

        // Show the mouse cursor and enable input for UI
        PC->bShowMouseCursor = true;
        PC->bEnableClickEvents = true;
        PC->bEnableMouseOverEvents = true;

        // Set input mode to UI only
        FInputModeUIOnly InputMode;
        PC->SetInputMode(InputMode);
    }
}

void AMenuBackgroundManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsCycling && CameraPositions.Num() > 0)
    {
        TimeUntilNextSwitch -= DeltaTime;
        if (TimeUntilNextSwitch <= 0)
        {
            SwitchToNextPosition();
            TimeUntilNextSwitch = DisplayDuration;
        }
    }
}

void AMenuBackgroundManager::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    // We don't need to bind any input for camera control
}

void AMenuBackgroundManager::AddCameraPosition(const FVector& Location, const FRotator& Rotation)
{
    CameraPositions.Add(FCameraPosition(Location, Rotation));
}

void AMenuBackgroundManager::StartCycling()
{
    if (CameraPositions.Num() > 0)
    {
        bIsCycling = true;
        SwitchToNextPosition();
    }
}

void AMenuBackgroundManager::StopCycling()
{
    bIsCycling = false;
}

void AMenuBackgroundManager::SwitchToNextPosition()
{
    if (CameraPositions.Num() > 0)
    {
        CurrentPositionIndex = (CurrentPositionIndex + 1) % CameraPositions.Num();
        const FCameraPosition& NewPosition = CameraPositions[CurrentPositionIndex];

        // Instantly teleport to the new position
        SetActorLocationAndRotation(NewPosition.Location, NewPosition.Rotation);
    }
}