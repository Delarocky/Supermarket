// CarManager.cpp
#include "CarManager.h"
#include "Kismet/GameplayStatics.h"

ACarManager::ACarManager()
{
    PrimaryActorTick.bCanEverTick = true;
    SpawnInterval = 5.0f;
}

void ACarManager::BeginPlay()
{
    Super::BeginPlay();

    StoreManager = AStoreManager::GetInstance(GetWorld());
    if (StoreManager)
    {
        StoreManager->OnStoreStatusChanged.AddDynamic(this, &ACarManager::OnStoreStatusChanged);
    }

    FindAllParkingSpots();

    // Start spawning cars if the store is open
    if (StoreManager && StoreManager->IsStoreOpen())
    {
        GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &ACarManager::SpawnCar, SpawnInterval, true);
    }
}

void ACarManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ACarManager::SpawnCar()
{
    if (!CarClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("CarClass is not set in CarManager"));
        return;
    }

    AParkingSpot* AvailableSpot = FindAvailableParkingSpot();
    if (AvailableSpot)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        ACarAI* NewCar = GetWorld()->SpawnActor<ACarAI>(CarClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
        if (NewCar)
        {
            NewCar->SetDestination(AvailableSpot);
            SpawnedCars.Add(NewCar);
            AvailableSpot->SetOccupied(true);
        }
    }
    else
    {
        // No available parking spots, stop spawning
        GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
    }
}

void ACarManager::FindAllParkingSpots()
{
    TArray<AActor*> FoundParkingSpots;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AParkingSpot::StaticClass(), FoundParkingSpots);

    for (AActor* Actor : FoundParkingSpots)
    {
        AParkingSpot* ParkingSpot = Cast<AParkingSpot>(Actor);
        if (ParkingSpot)
        {
            ParkingSpots.Add(ParkingSpot);
        }
    }
}

AParkingSpot* ACarManager::FindAvailableParkingSpot()
{
    for (AParkingSpot* ParkingSpot : ParkingSpots)
    {
        if (!ParkingSpot->IsOccupied())
        {
            return ParkingSpot;
        }
    }
    return nullptr;
}

void ACarManager::OnStoreStatusChanged(bool bIsOpen)
{
    if (bIsOpen)
    {
        // Start spawning cars
        GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &ACarManager::SpawnCar, SpawnInterval, true);
    }
    else
    {
        // Stop spawning cars
        GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
    }
}