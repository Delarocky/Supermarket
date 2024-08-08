
// ParkingSpot.cpp
#include "ParkingSpot.h"

AParkingSpot::AParkingSpot()
{
    PrimaryActorTick.bCanEverTick = false;
    bIsOccupied = false;

    // Create a static mesh component for visualization
    UStaticMeshComponent* MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ParkingSpotMesh"));
    RootComponent = MeshComponent;

    // You can set a default mesh here if you want
    // static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Game/Path/To/Your/Mesh"));
    // if (MeshAsset.Succeeded())
    //     MeshComponent->SetStaticMesh(MeshAsset.Object);
}

void AParkingSpot::OccupySpot()
{
    bIsOccupied = true;
}

void AParkingSpot::VacateSpot()
{
    bIsOccupied = false;
}