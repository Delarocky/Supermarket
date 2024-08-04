// RestockerManager.cpp
#include "RestockerManager.h"
#include "Kismet/GameplayStatics.h"

TMap<AShelf*, ARestockerAI*> ARestockerManager::ReservedShelves;
TMap<AProductBox*, ARestockerAI*> ARestockerManager::ReservedProductBoxes;
FCriticalSection ARestockerManager::ReservationLock;

ARestockerManager::ARestockerManager()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ARestockerManager::BeginPlay()
{
    Super::BeginPlay();

    // Find all RestockerAI instances in the level
    TArray<AActor*> FoundRestockers;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARestockerAI::StaticClass(), FoundRestockers);
    for (AActor* Actor : FoundRestockers)
    {
        ARestockerAI* Restocker = Cast<ARestockerAI>(Actor);
        if (Restocker)
        {
            AvailableRestockers.Add(Restocker);
        }
    }

    // Schedule updates instead of doing everything at once
    GetWorld()->GetTimerManager().SetTimer(UpdateTimerHandle, this, &ARestockerManager::ScheduledUpdate, 1.0f, true);
}

void ARestockerManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ARestockerManager::AssignTaskToRestocker(ARestockerAI* Restocker)
{
    if (!Restocker->IsAvailable())
    {
        return;
    }

    AShelf* ShelfToRestock = FindShelfToRestock();
    if (ShelfToRestock && IsShelfNeedingRestock(ShelfToRestock))
    {
        AProductBox* MatchingProductBox = FindMatchingProductBox(ShelfToRestock->GetCurrentProductClass());
        if (MatchingProductBox)
        {
            if (ReserveShelf(ShelfToRestock, Restocker) && ReserveProductBox(MatchingProductBox, Restocker))
            {
                Restocker->AssignTask(ShelfToRestock, MatchingProductBox);
                AvailableRestockers.Remove(Restocker);
            }
        }
    }
}

void ARestockerManager::UpdateShelvesNeedingRestock()
{
    ShelvesNeedingRestock.Empty();
    TArray<AActor*> FoundShelves;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AShelf::StaticClass(), FoundShelves);
    for (AActor* Actor : FoundShelves)
    {
        AShelf* Shelf = Cast<AShelf>(Actor);
        if (Shelf && IsShelfNeedingRestock(Shelf))
        {
            ShelvesNeedingRestock.Add(Shelf);
        }
    }
}

void ARestockerManager::UpdateAvailableProductBoxes()
{
    AvailableProductBoxes.Empty();
    TArray<AActor*> FoundProductBoxes;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AProductBox::StaticClass(), FoundProductBoxes);
    for (AActor* Actor : FoundProductBoxes)
    {
        AProductBox* ProductBox = Cast<AProductBox>(Actor);
        if (ProductBox && !ProductBox->GetAttachParentActor())
        {
            AvailableProductBoxes.Add(ProductBox);
        }
    }
}

AShelf* ARestockerManager::FindShelfToRestock()
{
    for (AShelf* Shelf : ShelvesNeedingRestock)
    {
        if (!ReservedShelves.Contains(Shelf) && IsShelfNeedingRestock(Shelf))
        {
            return Shelf;
        }
    }
    return nullptr;
}

AProductBox* ARestockerManager::FindMatchingProductBox(TSubclassOf<AProduct> ProductClass)
{
    for (AProductBox* ProductBox : AvailableProductBoxes)
    {
        if (ProductBox->GetProductClass() == ProductClass && !ReservedProductBoxes.Contains(ProductBox))
        {
            return ProductBox;
        }
    }
    return nullptr;
}

bool ARestockerManager::IsShelfSufficientlyStocked(AShelf* Shelf)
{
    if (!Shelf)
    {
        return true;
    }

    int32 CurrentStock = Shelf->GetProductCount();
    int32 MaxStock = Shelf->GetMaxStock();

    if (MaxStock == 0)
    {
        return true; // Avoid division by zero
    }

    // Consider the shelf sufficiently stocked if it's at least 95% full
    float StockPercentage = static_cast<float>(CurrentStock) / MaxStock * 100.0f;
    return StockPercentage >= 95.0f;
}

void ARestockerManager::PeriodicUpdate()
{
    UpdateShelvesNeedingRestock();
    UpdateAvailableProductBoxes();

    for (ARestockerAI* Restocker : AvailableRestockers)
    {
        AssignTaskToRestocker(Restocker);
    }
}

bool ARestockerManager::ReserveShelf(AShelf* Shelf, ARestockerAI* Restocker)
{
    FScopeLock Lock(&ReservationLock);
    if (!ReservedShelves.Contains(Shelf))
    {
        ReservedShelves.Add(Shelf, Restocker);
        return true;
    }
    return false;
}

void ARestockerManager::ReleaseShelf(AShelf* Shelf)
{
    FScopeLock Lock(&ShelfAssignmentLock);
    ShelvesBeingRestocked.Remove(Shelf);
}

bool ARestockerManager::ReserveProductBox(AProductBox* ProductBox, ARestockerAI* Restocker)
{
    FScopeLock Lock(&ReservationLock);
    if (!ReservedProductBoxes.Contains(ProductBox))
    {
        ReservedProductBoxes.Add(ProductBox, Restocker);
        return true;
    }
    return false;
}

void ARestockerManager::ReleaseProductBox(AProductBox* ProductBox)
{
    FScopeLock Lock(&ReservationLock);
    ReservedProductBoxes.Remove(ProductBox);
}

void ARestockerManager::OnRestockerTaskComplete(ARestockerAI* Restocker)
{
    if (Restocker)
    {
        AvailableRestockers.AddUnique(Restocker);

        // Release the shelf and product box associated with this restocker
        for (auto It = ReservedShelves.CreateIterator(); It; ++It)
        {
            if (It.Value() == Restocker)
            {
                ReleaseShelf(It.Key());
                It.RemoveCurrent();
                break;
            }
        }

        for (auto It = ReservedProductBoxes.CreateIterator(); It; ++It)
        {
            if (It.Value() == Restocker)
            {
                It.RemoveCurrent();
                break;
            }
        }

        // Instead of immediately assigning a new task, schedule it for the next frame
        FTimerHandle UnusedHandle;
        GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ARestockerManager::AssignTasksToAvailableRestockers);
    }
}


void ARestockerManager::ScheduledUpdate()
{
    // Spread out operations over multiple frames
    UpdateShelvesNeedingRestock();
    FTimerHandle ProductBoxUpdateTimer;
    GetWorld()->GetTimerManager().SetTimer(ProductBoxUpdateTimer, this, &ARestockerManager::UpdateAvailableProductBoxes, 0.1f, false);
    FTimerHandle AssignTasksTimer;
    GetWorld()->GetTimerManager().SetTimer(AssignTasksTimer, this, &ARestockerManager::AssignTasks, 0.2f, false);
}

void ARestockerManager::AssignTasks()
{
    TArray<ARestockerAI*> RestockersToAssign = AvailableRestockers;
    for (ARestockerAI* Restocker : RestockersToAssign)
    {
        if (AvailableRestockers.Contains(Restocker))
        {
            AssignTaskToRestocker(Restocker);
        }
    }
}


AShelf* ARestockerManager::FindShelfNeedingProduct(TSubclassOf<AProduct> ProductClass, ARestockerAI* Restocker)
{
    FScopeLock Lock(&ShelfAssignmentLock);

    for (AShelf* Shelf : ShelvesNeedingRestock)
    {
        if (Shelf->GetCurrentProductClass() == ProductClass &&
            !ReservedShelves.Contains(Shelf) &&
            !ShelvesBeingRestocked.Contains(Shelf))
        {
            ShelvesBeingRestocked.Add(Shelf, Restocker);
            return Shelf;
        }
    }
    return nullptr;
}

bool ARestockerManager::IsShelfNeedingRestock(AShelf* Shelf) const
{
    if (!Shelf)
    {
        return false;
    }

    int32 CurrentStock = Shelf->GetProductCount();
    int32 MaxStock = Shelf->GetMaxStock();

    // Consider the shelf as needing restock if it's less than 90% full
    return (static_cast<float>(CurrentStock) / MaxStock) < 0.9f;
}

void ARestockerManager::MarkRestockerAsAvailable(ARestockerAI* Restocker)
{
    if (Restocker)
    {
        AvailableRestockers.AddUnique(Restocker);
    }
}

void ARestockerManager::AssignTasksToAvailableRestockers()
{
    TArray<ARestockerAI*> RestockersToAssign = AvailableRestockers;
    for (ARestockerAI* Restocker : RestockersToAssign)
    {
        if (AvailableRestockers.Contains(Restocker))
        {
            AssignTaskToRestocker(Restocker);
        }
    }
}