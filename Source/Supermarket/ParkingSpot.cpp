// ParkingSpot.cpp
#include "ParkingSpot.h"

AParkingSpot::AParkingSpot()
{
    PrimaryActorTick.bCanEverTick = false;
    bIsOccupied = false;

    // Create a simple visual representation for the parking spot
    UStaticMeshComponent* VisualComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualComponent"));
    RootComponent = VisualComponent;

    // You can set a default static mesh here if you have one
    // VisualComponent->SetStaticMesh(....);
}

void AParkingSpot::BeginPlay()
{
    Super::BeginPlay();

    // Set the parking location and rotation based on the actor's transform
    ParkingLocation = GetActorLocation();
    ParkingRotation = GetActorRotation();
}

void AParkingSpot::SetOccupied(bool bNewOccupied)
{
    bIsOccupied = bNewOccupied;
}