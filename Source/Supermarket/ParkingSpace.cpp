// ParkingSpace.cpp
#include "ParkingSpace.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "AICustomerPawn.h"
#include "StoreManager.h"

AParkingSpace::AParkingSpace()
{
    PrimaryActorTick.bCanEverTick = true;

    ParkingSpaceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ParkingSpaceMesh"));
    RootComponent = ParkingSpaceMesh;

    CarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CarMesh"));
    CarMesh->SetupAttachment(RootComponent);
    CarMesh->SetVisibility(false);

    SpawnPoint1 = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint1"));
    SpawnPoint1->SetupAttachment(CarMesh);

    SpawnPoint2 = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint2"));
    SpawnPoint2->SetupAttachment(CarMesh);

    SpawnPoint3 = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint3"));
    SpawnPoint3->SetupAttachment(CarMesh);

    SpawnPoint4 = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint4"));
    SpawnPoint4->SetupAttachment(CarMesh);

    SpawnChance = 0.1f;
    SpawnInterval = 1.0f;
    bIsOccupied = false;
    SpawnedCustomersCount = 0;
    ReturnedCustomersCount = 0;
}

void AParkingSpace::BeginPlay()
{
    Super::BeginPlay();

    StoreManager = AStoreManager::GetInstance(GetWorld());
    if (StoreManager)
    {
        StoreManager->OnStoreStatusChanged.AddDynamic(this, &AParkingSpace::OnStoreStatusChanged);
    }

    GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AParkingSpace::TrySpawnCar, SpawnInterval, true);
}

void AParkingSpace::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AParkingSpace::TrySpawnCar()
{
    if (StoreManager && !StoreManager->IsStoreOpen())
    {
        return; // Don't spawn cars when the store is closed
    }

    if (!bIsOccupied && FMath::FRand() < SpawnChance)
    {
        bIsOccupied = true;
        CarMesh->SetVisibility(true);
        FTimerHandle CustomerSpawnTimerHandle;
        GetWorldTimerManager().SetTimer(CustomerSpawnTimerHandle, this, &AParkingSpace::SpawnCustomers, 1.0f, false);
    }
}

void AParkingSpace::SpawnCustomers()
{
    if (!CustomerClass)
    {
        //UE_LOGLogTemp, Error, TEXT("CustomerClass is not set in ParkingSpace!"));
        return;
    }

    SpawnedCustomersCount = FMath::RandRange(1, 4);
    ReturnedCustomersCount = 0;
    SpawnedCustomers.Empty();

    TArray<USceneComponent*> SpawnPoints = { SpawnPoint1, SpawnPoint2, SpawnPoint3, SpawnPoint4 };

    for (int32 i = 0; i < SpawnedCustomersCount; ++i)
    {
        FVector SpawnLocation = SpawnPoints[i]->GetComponentLocation();
        FRotator SpawnRotation = SpawnPoints[i]->GetComponentRotation();

        AAICustomerPawn* SpawnedCustomer = GetWorld()->SpawnActor<AAICustomerPawn>(CustomerClass, SpawnLocation, SpawnRotation);
        if (SpawnedCustomer)
        {
            SpawnedCustomers.Add(SpawnedCustomer);
            SpawnedCustomer->SetInitialSpawnLocation(SpawnLocation);
            SpawnedCustomer->SetAssignedParkingSpace(this);
        }
    }
}

void AParkingSpace::CustomerReturned(AActor* Customer)
{
    if (SpawnedCustomers.Contains(Customer))
    {
        ReturnedCustomersCount++;
        SpawnedCustomers.Remove(Customer);

        if (ReturnedCustomersCount >= SpawnedCustomersCount)
        {
            // All customers have returned, remove the car after a delay
            GetWorld()->GetTimerManager().SetTimer(RemoveCarTimerHandle, this, &AParkingSpace::RemoveCar, 2.0f, false);
        }
    }
}

void AParkingSpace::RemoveCar()
{
    bIsOccupied = false;
    CarMesh->SetVisibility(false);
    SpawnedCustomers.Empty();
    ReturnedCustomersCount = 0;
    SpawnedCustomersCount = 0;

    // Start the timer for the next car spawn attempt
    GetWorld()->GetTimerManager().SetTimer(SpawnTimerHandle, this, &AParkingSpace::TrySpawnCar, SpawnInterval, true);
}

void AParkingSpace::OnStoreStatusChanged(bool bIsOpen)
{
    if (!bIsOpen)
    {
        // Stop the spawn timer when the store closes
        GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
    }
    else
    {
        // Restart the spawn timer when the store opens
        GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AParkingSpace::TrySpawnCar, SpawnInterval, true);
    }
}
