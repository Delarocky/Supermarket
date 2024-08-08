// CarManager.cpp
#include "CarManager.h"
#include "Kismet/GameplayStatics.h"

ACarManager::ACarManager()
{
    PrimaryActorTick.bCanEverTick = false;
    OccupiedParkingSpots = 0;
}

void ACarManager::BeginPlay()
{
    Super::BeginPlay();
    FindAllParkingSpots();

    UE_LOG(LogTemp, Log, TEXT("CarManager: Begin Play. Found %d parking spots."), ParkingSpots.Num());

    GetWorld()->GetTimerManager().SetTimer(SpawnTimerHandle, this, &ACarManager::SpawnCarTimerCallback, CarSpawnInterval, true);
}

void ACarManager::SpawnCarTimerCallback()
{
    UE_LOG(LogTemp, Log, TEXT("CarManager: Spawn Timer Callback. Occupied spots: %d/%d"), OccupiedParkingSpots, ParkingSpots.Num());

    if (OccupiedParkingSpots < ParkingSpots.Num())
    {
        if (CarClassToSpawn)
        {
            SpawnCarAndNavigateToParkingSpot(CarClassToSpawn);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("CarManager: CarClassToSpawn is not set!"));
        }
    }
    else
    {
        GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);
        UE_LOG(LogTemp, Log, TEXT("CarManager: All parking spots are full. Stopped spawning cars."));
    }
}

void ACarManager::SpawnCarAndNavigateToParkingSpot(TSubclassOf<ACar> CarClass)
{
    UE_LOG(LogTemp, Log, TEXT("CarManager: Attempting to spawn car and navigate to parking spot."));

    AParkingSpot* AvailableSpot = FindAvailableParkingSpot();
    if (!AvailableSpot)
    {
        UE_LOG(LogTemp, Warning, TEXT("CarManager: No available parking spots."));
        return;
    }

    ACarSplinePathfinder* Pathfinder = GetWorld()->SpawnActor<ACarSplinePathfinder>(ACarSplinePathfinder::StaticClass());
    if (!Pathfinder)
    {
        UE_LOG(LogTemp, Error, TEXT("CarManager: Failed to spawn CarSplinePathfinder."));
        return;
    }

    Pathfinder->ObstacleClasses = ObstacleClasses;

    FVector EndLocation = AvailableSpot->GetActorLocation();

    UE_LOG(LogTemp, Log, TEXT("CarManager: Generating path from (%s) to parking spot at (%s)"),
        *CarSpawnLocation.ToString(), *EndLocation.ToString());

    USplineComponent* Path = Pathfinder->GeneratePathForCar(CarSpawnLocation, EndLocation);

    if (Path)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        ACar* Car = GetWorld()->SpawnActor<ACar>(CarClass, CarSpawnLocation, FRotator::ZeroRotator, SpawnParams);

        if (Car)
        {
            Car->FollowSpline(Path);
            Car->OnCarParked.AddDynamic(this, &ACarManager::OnCarParked);
            AvailableSpot->OccupySpot();
            OccupiedParkingSpots++;
            UE_LOG(LogTemp, Log, TEXT("CarManager: Car spawned at (%s) and set to follow path to parking spot. Occupied spots: %d/%d"),
                *CarSpawnLocation.ToString(), OccupiedParkingSpots, ParkingSpots.Num());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("CarManager: Failed to spawn car."));
            Pathfinder->Destroy(); // Clean up the pathfinder if car spawn fails
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("CarManager: Failed to generate path for car."));
        Pathfinder->Destroy(); // Clean up the pathfinder if path generation fails
    }
}

void ACarManager::FindAllParkingSpots()
{
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AParkingSpot::StaticClass(), reinterpret_cast<TArray<AActor*>&>(ParkingSpots));
    UE_LOG(LogTemp, Log, TEXT("CarManager: Found %d parking spots."), ParkingSpots.Num());
}

AParkingSpot* ACarManager::FindAvailableParkingSpot()
{
    for (AParkingSpot* Spot : ParkingSpots)
    {
        if (Spot && !Spot->bIsOccupied)
        {
            UE_LOG(LogTemp, Log, TEXT("CarManager: Found available parking spot at location: %s"), *Spot->GetActorLocation().ToString());
            return Spot;
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("CarManager: No available parking spots found."));
    return nullptr;
}

void ACarManager::OnCarParked(ACar* ParkedCar)
{
    UE_LOG(LogTemp, Log, TEXT("CarManager: Car parked successfully."));

    // Clean up the spline
    if (ParkedCar)
    {
        ParkedCar->CleanupSpline();
    }
}