// CarManager.cpp
#include "CarManager.h"
#include "Kismet/GameplayStatics.h"
ACarManager::ACarManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ACarManager::BeginPlay()
{
    Super::BeginPlay();
    FindAllParkingSpots();
}

void ACarManager::SpawnCarAndNavigateToParkingSpot(TSubclassOf<ACar> CarClass, const FVector& StartLocation)
{
    AParkingSpot* AvailableSpot = FindAvailableParkingSpot();
    if (!AvailableSpot)
    {
        UE_LOG(LogTemp, Warning, TEXT("No available parking spots."));
        return;
    }

    if (!Pathfinder)
    {
        Pathfinder = GetWorld()->SpawnActor<ACarSplinePathfinder>(ACarSplinePathfinder::StaticClass());
        if (Pathfinder)
        {
            Pathfinder->ObstacleClasses = ObstacleClasses;
        }
    }

    if (Pathfinder)
    {
        // Use the actual parking spot location
        FVector EndLocation = AvailableSpot->GetActorLocation();

        UE_LOG(LogTemp, Log, TEXT("Generating path from (%s) to parking spot at (%s)"),
            *StartLocation.ToString(), *EndLocation.ToString());

        USplineComponent* Path = Pathfinder->GeneratePathForCar(StartLocation, EndLocation);

        if (Path)
        {
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

            ACar* Car = GetWorld()->SpawnActor<ACar>(CarClass, StartLocation, FRotator::ZeroRotator, SpawnParams);

            if (Car)
            {
                Car->FollowSpline(Path);
                Car->OnCarParked.AddDynamic(this, &ACarManager::OnCarParked);
                AvailableSpot->OccupySpot();
                UE_LOG(LogTemp, Log, TEXT("Car spawned and set to follow path to parking spot."));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to spawn car."));
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to generate path for car."));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to spawn CarSplinePathfinder."));
    }
}

void ACarManager::FindAllParkingSpots()
{
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AParkingSpot::StaticClass(), reinterpret_cast<TArray<AActor*>&>(ParkingSpots));
    UE_LOG(LogTemp, Log, TEXT("Found %d parking spots."), ParkingSpots.Num());
}

void ACarManager::OnCarParked(ACar* ParkedCar)
{
    // Handle any post-parking logic here
    UE_LOG(LogTemp, Log, TEXT("Car parked successfully."));

    // Destroy the spline component
    if (Pathfinder)
    {
        USplineComponent* SplineToDestroy = Pathfinder->GetSplineComponent();
        if (SplineToDestroy)
        {
            SplineToDestroy->DestroyComponent();
        }
    }
}

AParkingSpot* ACarManager::FindAvailableParkingSpot()
{
    for (AParkingSpot* Spot : ParkingSpots)
    {
        if (Spot && !Spot->bIsOccupied)
        {
            UE_LOG(LogTemp, Log, TEXT("Found available parking spot at location: %s"), *Spot->GetActorLocation().ToString());
            return Spot;
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("No available parking spots found."));
    return nullptr;
}