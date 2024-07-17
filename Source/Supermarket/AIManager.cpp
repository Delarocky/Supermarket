// AIManager.cpp
#include "AIManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AAIManager::AAIManager()
{
    PrimaryActorTick.bCanEverTick = true;
    MaxCustomers = 50; // Adjust as needed
    MaxRestockers = 10; // Adjust as needed
    UpdateInterval = 0.5f; // Update every half second
}

void AAIManager::BeginPlay()
{
    Super::BeginPlay();
    GetWorldTimerManager().SetTimer(UpdateTimerHandle, this, &AAIManager::UpdateAIAgents, UpdateInterval, true);
}

void AAIManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AAIManager::SpawnCustomer(TSubclassOf<AAICustomerPawn> CustomerClass, const FVector& SpawnLocation)
{
    if (Customers.Num() < MaxCustomers)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        AAICustomerPawn* NewCustomer = GetWorld()->SpawnActor<AAICustomerPawn>(CustomerClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
        if (NewCustomer)
        {
            Customers.Add(NewCustomer);
            NewCustomer->StartShopping();
        }
    }
}

void AAIManager::SpawnRestocker(TSubclassOf<ARestockerAI> RestockerClass, const FVector& SpawnLocation)
{
    if (Restockers.Num() < MaxRestockers)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        ARestockerAI* NewRestocker = GetWorld()->SpawnActor<ARestockerAI>(RestockerClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
        if (NewRestocker)
        {
            Restockers.Add(NewRestocker);
            NewRestocker->StartRestocking();
        }
    }
}

void AAIManager::RemoveCustomer(AAICustomerPawn* Customer)
{
    Customers.Remove(Customer);
    if (Customer)
    {
        Customer->Destroy();
    }
}

void AAIManager::RemoveRestocker(ARestockerAI* Restocker)
{
    Restockers.Remove(Restocker);
    if (Restocker)
    {
        Restocker->Destroy();
    }
}

void AAIManager::UpdateAIAgents()
{
    // Update customers
    for (int32 i = Customers.Num() - 1; i >= 0; --i)
    {
        if (!IsValid(Customers[i]))
        {
            Customers.RemoveAt(i);
        }
    }

    // Update restockers
    for (int32 i = Restockers.Num() - 1; i >= 0; --i)
    {
        if (!IsValid(Restockers[i]))
        {
            Restockers.RemoveAt(i);
        }
    }

    OptimizeAIBehavior();
}

void AAIManager::OptimizeAIBehavior()
{
    // Implement optimization strategies here
    // For example:
    // 1. Prioritize updating AI closer to the player
    // 2. Reduce update frequency for distant AI
    // 3. Batch similar AI operations

    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
    if (!PlayerController)
    {
        return;
    }

    FVector PlayerLocation = PlayerController->GetPawn()->GetActorLocation();

    // Sort AI agents by distance to player
    Customers.Sort([PlayerLocation](const AAICustomerPawn& A, const AAICustomerPawn& B) {
        return FVector::DistSquared(A.GetActorLocation(), PlayerLocation) < FVector::DistSquared(B.GetActorLocation(), PlayerLocation);
        });

    Restockers.Sort([PlayerLocation](const ARestockerAI& A, const ARestockerAI& B) {
        return FVector::DistSquared(A.GetActorLocation(), PlayerLocation) < FVector::DistSquared(B.GetActorLocation(), PlayerLocation);
        });

    // Update AI based on distance (example: update closer AI more frequently)
    int32 HighPriorityCount = FMath::Min(Customers.Num(), 10); // Update 10 closest customers every frame
    for (int32 i = 0; i < HighPriorityCount; ++i)
    {
        // Perform high-priority updates for close customers
        // e.g., Customers[i]->PerformDetailedUpdate();
    }

    // Update restockers less frequently
    static int32 RestockerUpdateIndex = 0;
    if (Restockers.Num() > 0)
    {
        // Update one restocker per frame
        // Restockers[RestockerUpdateIndex]->PerformUpdate();
        RestockerUpdateIndex = (RestockerUpdateIndex + 1) % Restockers.Num();
    }
}