// AICustomerPawn.cpp
#include "AICustomerPawn.h"
#include "Checkout.h"
#include "Shelf.h"
#include "Product.h"
#include "ShoppingBag.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "AIController.h"

AAICustomerPawn::AAICustomerPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    ShoppingBag = CreateDefaultSubobject<UShoppingBag>(TEXT("ShoppingBag"));
    CurrentState = ECustomerState::Idle;
    MaxProductsToCollect = FMath::RandRange(5, 10);
    TargetShelf = nullptr;
    AIController = nullptr;
    ItemsToPickUp = 0;
}

void AAICustomerPawn::BeginPlay()
{
    Super::BeginPlay();
    AIController = Cast<AAIController>(GetController());
    if (AIController)
    {
        AIController->ReceiveMoveCompleted.AddDynamic(this, &AAICustomerPawn::OnMoveCompleted);
    }
    UpdateState();
}

void AAICustomerPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AAICustomerPawn::UpdateState()
{
    switch (CurrentState)
    {
    case ECustomerState::Idle:
        if (ShoppingBag->GetProductCount() < MaxProductsToCollect)
        {
            FindAndMoveToShelf();
        }
        else
        {
            FindAndMoveToCheckout();
        }
        break;

    case ECustomerState::MovingToShelf:
    case ECustomerState::PickingUpProduct:
    case ECustomerState::MovingToCheckout:
        // These states are handled by OnMoveCompleted
        break;

    case ECustomerState::CheckingOut:
        if (TargetCheckout && TargetCheckout->IsCustomerBeingProcessed(this))
        {
            // Wait for the checkout to complete
            GetWorldTimerManager().SetTimer(StateTimerHandle, this, &AAICustomerPawn::UpdateState, 0.5f, false);
        }
        else
        {
            // Move to the next position in the queue or leave if done
            int32 QueuePosition = TargetCheckout ? TargetCheckout->GetCustomerQueuePosition(this) : -1;
            if (QueuePosition >= 0)
            {
                MoveToCheckoutPosition(QueuePosition);
            }
            else
            {
                CurrentState = ECustomerState::Leaving;
                LeaveStore();
            }
        }
        break;

    case ECustomerState::Roaming:
        if (!GetWorldTimerManager().IsTimerActive(StateTimerHandle))
        {
            FindAndMoveToCheckout();
        }
        break;

    case ECustomerState::Leaving:
        // Do nothing, waiting for LeaveStore to complete
        break;
    }
}

void AAICustomerPawn::FindAndMoveToShelf()
{
    AShelf* NewTargetShelf = FindRandomStockedShelf();
    if (IsValid(NewTargetShelf))
    {
        TargetShelf = NewTargetShelf;
        CurrentState = ECustomerState::MovingToShelf;

        if (AIController)
        {
            SimpleMoveTo(TargetShelf->GetActorLocation());
            UE_LOG(LogTemp, Display, TEXT("AI %s: Moving to shelf %s"), *GetName(), *TargetShelf->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("AI %s: AIController is null!"), *GetName());
            CurrentState = ECustomerState::Idle;
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AI %s: No stocked shelves found. Waiting..."), *GetName());
        CurrentState = ECustomerState::Idle;

        GetWorld()->GetTimerManager().SetTimer(StateTimerHandle, this, &AAICustomerPawn::UpdateState, 2.0f, false);
    }
}

void AAICustomerPawn::TryPickUpProduct()
{
    if (!TargetShelf)
    {
        UE_LOG(LogTemp, Warning, TEXT("AI %s: No target shelf set."), *GetName());
        FinishShopping();
        return;
    }

    if (ItemsToPickUp == 0)
    {
        ItemsToPickUp = 1;
        UE_LOG(LogTemp, Display, TEXT("AI %s: Decided to pick up %d items from shelf %s."),
            *GetName(), ItemsToPickUp, *TargetShelf->GetName());
    }

    if (ItemsToPickUp > 0 && TargetShelf->GetProductCount() > 0)
    {
        AProduct* PickedProduct = TargetShelf->RemoveRandomProduct();
        if (PickedProduct)
        {
            FProductData ProductData = PickedProduct->GetProductData();
            ShoppingBag->AddProduct(ProductData);
            UE_LOG(LogTemp, Display, TEXT("AI %s: Picked up product %s from shelf %s. Total products: %d"),
                *GetName(), *ProductData.Name, *TargetShelf->GetName(), ShoppingBag->GetProductCount());
            PickedProduct->Destroy();

            ItemsToPickUp--;
        }

        if (ItemsToPickUp > 0)
        {
            // Schedule next pickup in 1 second
            GetWorld()->GetTimerManager().SetTimer(PickupTimerHandle, this, &AAICustomerPawn::TryPickUpProduct, 1.0f, false);
        }
        else
        {
            FinishShopping();
        }
    }
    else
    {
        FinishShopping();
    }
}

void AAICustomerPawn::FindAndMoveToCheckout()
{
    TArray<AActor*> FoundCheckouts;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACheckout::StaticClass(), FoundCheckouts);

    UE_LOG(LogTemp, Display, TEXT("AI %s: Found %d checkouts"), *GetName(), FoundCheckouts.Num());

    if (FoundCheckouts.Num() > 0)
    {
        // Randomly shuffle the checkouts
        for (int32 i = 0; i < FoundCheckouts.Num() - 1; i++)
        {
            int32 j = FMath::RandRange(i, FoundCheckouts.Num() - 1);
            FoundCheckouts.Swap(i, j);
        }

        // Try to find a checkout with available queue spots
        for (AActor* CheckoutActor : FoundCheckouts)
        {
            ACheckout* Checkout = Cast<ACheckout>(CheckoutActor);
            if (Checkout && !Checkout->IsQueueFull())
            {
                TargetCheckout = Checkout;
                TargetCheckout->AddCustomerToQueue(this);
                FVector NextQueuePosition = TargetCheckout->GetNextQueuePosition();

                SimpleMoveTo(NextQueuePosition);

                CurrentState = ECustomerState::MovingToCheckout;
                UE_LOG(LogTemp, Display, TEXT("AI %s: Moving to checkout %s"), *GetName(), *TargetCheckout->GetName());
                return;
            }
        }

        // If all queue spots are full, start roaming
        UE_LOG(LogTemp, Warning, TEXT("AI %s: All checkouts are full, starting to roam."), *GetName());
        StartRoaming();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AI %s: No checkouts found. Waiting..."), *GetName());
        GetWorldTimerManager().SetTimer(StateTimerHandle, this, &AAICustomerPawn::UpdateState, 2.0f, false);
    }
}

void AAICustomerPawn::ProcessCheckout()
{
    // The checkout process is now handled by the ACheckout class
    // We just need to wait until the checkout calls our OnCheckoutComplete method
    if (TargetCheckout)
    {
        UE_LOG(LogTemp, Display, TEXT("AI %s: Waiting for checkout to complete."), *GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AI %s: No target checkout set."), *GetName());
        CurrentState = ECustomerState::Idle;
        UpdateState();
    }
}

void AAICustomerPawn::LeaveStore()
{
    UE_LOG(LogTemp, Display, TEXT("AI %s: Leaving store."), *GetName());
    UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetNavigationSystem(GetWorld());
    if (NavSystem)
    {
        FNavLocation ExitLocation;
        if (NavSystem->GetRandomPointInNavigableRadius(FVector::ZeroVector, 10000.0f, ExitLocation))
        {
            SimpleMoveTo(ExitLocation.Location);
        }
    }

    // Destroy the pawn after a delay
    FTimerHandle DestroyTimerHandle;
    GetWorldTimerManager().SetTimer(DestroyTimerHandle, this, &AAICustomerPawn::DestroySelf, 10.0f, false);
}

void AAICustomerPawn::OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
    if (Result == EPathFollowingResult::Success)
    {
        switch (CurrentState)
        {
        case ECustomerState::MovingToShelf:
            CurrentState = ECustomerState::PickingUpProduct;
            TryPickUpProduct();
            break;

        case ECustomerState::MovingToCheckout:
            if (TargetCheckout)
            {
                int32 QueuePosition = TargetCheckout->GetCustomerQueuePosition(this);
                if (QueuePosition == 0)
                {
                    CurrentState = ECustomerState::CheckingOut;
                }
                else
                {
                    // Wait in the queue
                    GetWorldTimerManager().SetTimer(StateTimerHandle, this, &AAICustomerPawn::UpdateState, 0.5f, false);
                }
            }
            break;

        case ECustomerState::Roaming:
            MoveToRandomLocation();
            break;
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AI %s: Move failed with result: %s"), *GetName(), *UEnum::GetValueAsString(Result));

        // If movement failed, try to unstuck the AI
        FVector NewLocation = GetActorLocation() + FVector(FMath::RandRange(-100.f, 100.f), FMath::RandRange(-100.f, 100.f), 0.f);
        SetActorLocation(NewLocation, true);

        // Reset the state to Idle and update
        CurrentState = ECustomerState::Idle;
        UpdateState();
    }
}

void AAICustomerPawn::SimpleMoveTo(const FVector& Destination)
{
    if (AIController)
    {
        AIController->MoveToLocation(Destination, 50.0f, true, true, true, false, nullptr, true);
    }
}

AShelf* AAICustomerPawn::FindRandomStockedShelf()
{
    UE_LOG(LogTemp, Display, TEXT("AI %s: Finding random stocked shelf."), *GetName());

    if (!GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("AI %s: World is null in FindRandomStockedShelf!"), *GetName());
        return nullptr;
    }

    TArray<AActor*> FoundShelves;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AShelf::StaticClass(), FoundShelves);

    UE_LOG(LogTemp, Display, TEXT("AI %s: Found %d total shelves."), *GetName(), FoundShelves.Num());

    TArray<AShelf*> StockedShelves;

    for (AActor* Actor : FoundShelves)
    {
        if (!IsValid(Actor))
        {
            UE_LOG(LogTemp, Warning, TEXT("AI %s: Found invalid actor in shelf list."), *GetName());
            continue;
        }

        AShelf* Shelf = Cast<AShelf>(Actor);
        if (IsValid(Shelf))
        {
            int32 ProductCount = Shelf->GetProductCount();
            if (ProductCount > 0)
            {
                StockedShelves.Add(Shelf);
                UE_LOG(LogTemp, Display, TEXT("AI %s: Found stocked shelf %s with %d products."), *GetName(), *Shelf->GetName(), ProductCount);
            }
            else
            {
                UE_LOG(LogTemp, Display, TEXT("AI %s: Shelf %s has no products."), *GetName(), *Shelf->GetName());
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AI %s: Failed to cast actor to AShelf."), *GetName());
        }
    }

    if (StockedShelves.Num() > 0)
    {
        int32 RandomIndex = FMath::RandRange(0, StockedShelves.Num() - 1);
        AShelf* ChosenShelf = StockedShelves[RandomIndex];
        UE_LOG(LogTemp, Display, TEXT("AI %s: Chose random stocked shelf %s."), *GetName(), *ChosenShelf->GetName());
        return ChosenShelf;
    }

    UE_LOG(LogTemp, Warning, TEXT("AI %s: No stocked shelves found."), *GetName());
    return nullptr;
}

void AAICustomerPawn::FinishShopping()
{
    // Clear the pickup timer if it's still active
    if (GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(PickupTimerHandle))
    {
        GetWorld()->GetTimerManager().ClearTimer(PickupTimerHandle);
    }

    UE_LOG(LogTemp, Display, TEXT("AI %s: Finished shopping at shelf %s. Total products: %d"),
        *GetName(), *TargetShelf->GetName(), ShoppingBag->GetProductCount());
    CurrentState = ECustomerState::Idle;
    UpdateState();
}

void AAICustomerPawn::StartRoaming()
{
    CurrentState = ECustomerState::Roaming;
    UE_LOG(LogTemp, Display, TEXT("AI %s: All checkouts are full. Starting to roam."), *GetName());
    MoveToRandomLocation();
}

void AAICustomerPawn::MoveToRandomLocation()
{
    UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetNavigationSystem(GetWorld());
    if (NavSystem)
    {
        FNavLocation RandomLocation;
        if (NavSystem->GetRandomReachablePointInRadius(GetActorLocation(), 1000.0f, RandomLocation))
        {
            SimpleMoveTo(RandomLocation.Location);
        }
    }

    // Set a timer to try moving to a checkout again after some time
    GetWorldTimerManager().SetTimer(StateTimerHandle, this, &AAICustomerPawn::FindAndMoveToCheckout, 5.0f, false);
}

void AAICustomerPawn::DestroySelf()
{
    UE_LOG(LogTemp, Display, TEXT("AI %s: Destroying self."), *GetName());
    Destroy();
}

void AAICustomerPawn::OnCheckoutComplete()
{
    UE_LOG(LogTemp, Display, TEXT("AI %s: Checkout complete. Items in bag: %d. Leaving store."),
        *GetName(), ShoppingBag->GetProductCount());

    // Clear the shopping bag
    ShoppingBag->EmptyBag();

    // The customer has been processed, now they can leave
    CurrentState = ECustomerState::Leaving;
    LeaveStore();
}

void AAICustomerPawn::MoveToCheckoutPosition(int32 QueuePosition)
{
    if (TargetCheckout)
    {
        FVector TargetLocation = TargetCheckout->GetQueuePosition(QueuePosition);
        SimpleMoveTo(TargetLocation);
    }
}

void AAICustomerPawn::MoveToActor(AActor* TargetActor)
{
    if (AIController && TargetActor)
    {
        SimpleMoveTo(TargetActor->GetActorLocation());
    }
}