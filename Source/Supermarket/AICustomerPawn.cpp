// AICustomerPawn.cpp
#include "AICustomerPawn.h"
#include "Shelf.h"
#include "Product.h"
#include "Checkout.h"
#include "ShoppingBag.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "AIController.h"
#include "TimerManager.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/RotatingMovementComponent.h"

AAICustomerPawn::AAICustomerPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    ShoppingBag = CreateDefaultSubobject<UShoppingBag>(TEXT("ShoppingBag"));
}

void AAICustomerPawn::BeginPlay()
{
    Super::BeginPlay();
    AIController = Cast<AAIController>(GetController());
    if (AIController)
    {
        AIController->ReceiveMoveCompleted.AddDynamic(this, &AAICustomerPawn::OnMoveCompleted);
    }
    TotalItemsToPickUp = FMath::RandRange(3, 10);
    UE_LOG(LogTemp, Display, TEXT("AI %s: Will pick up a total of %d items."), *GetName(), TotalItemsToPickUp);
    DecideNextAction();
}

void AAICustomerPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsRotating)
    {
        ElapsedTime += DeltaTime;
        float Alpha = ElapsedTime / RotationTime;

        FRotator CurrentRotation = GetActorRotation();
        CurrentRotation.Pitch = 0.0f;
        CurrentRotation.Roll = 0.0f;

        FRotator NewRotation = FMath::Lerp(CurrentRotation, TargetRotation, Alpha);

        SetActorRotation(NewRotation);

        if (Alpha >= 1.0f)
        {
            bIsRotating = false;
            SetActorRotation(TargetRotation);
            TryPickUpProduct();
        }
    }
}

void AAICustomerPawn::DecideNextAction()
{
    if (!IsValid(this) || !GetWorld())
    {
        return;
    }

    if (ShoppingBag->GetProductCount() < TotalItemsToPickUp)
    {
        MoveToRandomShelf();
    }
    else if (ShoppingBag->GetProductCount() > 0)
    {
        MoveToCheckout();
    }
    else
    {
        LeaveStore();
    }
}

void AAICustomerPawn::MoveToRandomShelf()
{
    AShelf* TargetShelf = FindRandomStockedShelf();
    if (TargetShelf)
    {
        CurrentShelf = TargetShelf;
        FVector TargetLocation = TargetShelf->GetAccessPointLocation();
        MoveToLocation(TargetLocation);
        UE_LOG(LogTemp, Display, TEXT("AI %s: Moving to shelf %s at access point %s"),
            *GetName(), *TargetShelf->GetName(), *TargetLocation.ToString());
    }
    else
    {
       // UE_LOG(LogTemp, Warning, TEXT("AI %s: No accessible stocked shelves found. Waiting..."), *GetName());
        GetWorld()->GetTimerManager().SetTimer(ActionTimerHandle, this, &AAICustomerPawn::DecideNextAction, 1.0f, false);
    }
}

void AAICustomerPawn::MoveToCheckout()
{
    ACheckout* TargetCheckout = FindOpenCheckout();
    if (TargetCheckout)
    {
        MoveToLocation(TargetCheckout->GetNextQueuePosition());
        UE_LOG(LogTemp, Display, TEXT("AI %s: Moving to checkout %s"), *GetName(), *TargetCheckout->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AI %s: No open checkouts found. Waiting..."), *GetName());
        GetWorld()->GetTimerManager().SetTimer(ActionTimerHandle, this, &AAICustomerPawn::DecideNextAction, 2.0f, false);
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
            MoveToLocation(ExitLocation.Location);
        }
    }
    GetWorld()->GetTimerManager().SetTimer(ActionTimerHandle, this, &AAICustomerPawn::DestroySelf, 10.0f, false);
}

void AAICustomerPawn::MoveToLocation(const FVector& Destination)
{
    if (AIController)
    {
        UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
        if (NavSys)
        {
            FNavLocation NavLocation;
            if (NavSys->ProjectPointToNavigation(Destination, NavLocation, FVector(100, 100, 100)))
            {
                AIController->MoveToLocation(NavLocation.Location, 1.0f, true, true, false, false, nullptr, true);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("AI %s: Failed to find valid navigation point. Choosing new action."), *GetName());
                DecideNextAction();
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("AI %s: Navigation system not found."), *GetName());
            DecideNextAction();
        }
    }
}


void AAICustomerPawn::TurnToFaceShelf()
{
    if (!CurrentShelf)
    {
        UE_LOG(LogTemp, Warning, TEXT("AI %s: Cannot turn to face shelf, CurrentShelf is null"), *GetName());
        DecideNextAction();
        return;
    }

    FVector AILocation = GetActorLocation();
    FVector ShelfLocation = CurrentShelf->GetActorLocation();
    TargetRotation = UKismetMathLibrary::FindLookAtRotation(AILocation, ShelfLocation);

    // Only consider yaw rotation to avoid twitching on X and Y axes
    TargetRotation.Pitch = 0.0f;
    TargetRotation.Roll = 0.0f;

    // Calculate the shortest rotation
    FRotator CurrentRotation = GetActorRotation();
    CurrentRotation.Pitch = 0.0f;
    CurrentRotation.Roll = 0.0f;

    DeltaRotation = UKismetMathLibrary::NormalizedDeltaRotator(TargetRotation, CurrentRotation);

    // Check if the AI is already facing the shelf within 1 degree
    if (FMath::Abs(DeltaRotation.Yaw) <= 10.0f)
    {
        // If within 1 degree, don't turn, just pick up the item
        //UE_LOG(LogTemp, Warning, TEXT("AI %s: Already facing the shelf within 1 degree"), *GetName());
        TryPickUpProduct();
        return;
    }

    // Calculate the time needed for rotation (assuming 180 degrees per second)
    RotationTime = FMath::Abs(DeltaRotation.Yaw) / 180.0f;
    ElapsedTime = 0.0f;

    bIsRotating = true;
}


void AAICustomerPawn::OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
    if (!IsValid(this))
    {
        return;
    }

    if (Result == EPathFollowingResult::Success)
    {
        if (ShoppingBag->GetProductCount() < TotalItemsToPickUp)
        {
            if (CurrentShelf)
            {
                TurnToFaceShelf();
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("AI %s: CurrentShelf is null"), *GetName());
                DecideNextAction();
            }
        }
        else if (ShoppingBag->GetProductCount() > 0)
        {
            ProcessCheckout();
        }
        else
        {
            DestroySelf();
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AI %s: Move failed with result: %s"), *GetName(), *UEnum::GetValueAsString(Result));
        CurrentShelf = nullptr;
        DecideNextAction();
    }
}


void AAICustomerPawn::TryPickUpProduct()
{
    if (CurrentShelf && CurrentShelf->GetProductCount() > 0)
    {
        AProduct* PickedProduct = CurrentShelf->RemoveRandomProduct();
        if (PickedProduct)
        {
            FProductData ProductData = PickedProduct->GetProductData();
            ShoppingBag->AddProduct(ProductData);

            UE_LOG(LogTemp, Display, TEXT("AI %s: Picked up product %s from shelf %s. Total products: %d/%d"),
                *GetName(), *ProductData.Name, *CurrentShelf->GetName(), ShoppingBag->GetProductCount(), TotalItemsToPickUp);

            PickedProduct->Destroy();

            // Schedule the next product pickup after 1 second
            FTimerHandle NextPickupTimerHandle;
            GetWorld()->GetTimerManager().SetTimer(NextPickupTimerHandle, this, &AAICustomerPawn::DecideNextAction, 1.0f, false);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AI %s: Failed to pick up product from shelf %s"), *GetName(), *CurrentShelf->GetName());
            DecideNextAction();
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AI %s: No current shelf or shelf is empty"), *GetName());
        CurrentShelf = nullptr;
        DecideNextAction();
    }
}

void AAICustomerPawn::ProcessCheckout()
{
    ACheckout* RandomCheckout = FindOpenCheckout();
    if (RandomCheckout)
    {
        RandomCheckout->ProcessCustomer(this);
        UE_LOG(LogTemp, Display, TEXT("AI %s: Processing checkout at %s."), *GetName(), *RandomCheckout->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AI %s: No open checkout found."), *GetName());
        DecideNextAction();
    }
}

void AAICustomerPawn::OnCheckoutComplete()
{
    UE_LOG(LogTemp, Display, TEXT("AI %s: Checkout complete. Items in bag: %d. Leaving store."),
        *GetName(), ShoppingBag->GetProductCount());
    ShoppingBag->EmptyBag();
    LeaveStore();
}

AShelf* AAICustomerPawn::FindRandomStockedShelf()
{
    TArray<AActor*> FoundShelves;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AShelf::StaticClass(), FoundShelves);
    TArray<AShelf*> AccessibleStockedShelves;

    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys)
    {
        UE_LOG(LogTemp, Error, TEXT("Navigation system not found"));
        return nullptr;
    }

    for (AActor* Actor : FoundShelves)
    {
        AShelf* Shelf = Cast<AShelf>(Actor);
        if (IsValid(Shelf) && Shelf->GetProductCount() > 0)
        {
            FVector ShelfLocation = Shelf->GetActorLocation();
            FNavLocation NavLocation;
            if (NavSys->ProjectPointToNavigation(ShelfLocation, NavLocation, FVector(100, 100, 100)))
            {
                AccessibleStockedShelves.Add(Shelf);
            }
        }
    }

    if (AccessibleStockedShelves.Num() > 0)
    {
        int32 RandomIndex = FMath::RandRange(0, AccessibleStockedShelves.Num() - 1);
        return AccessibleStockedShelves[RandomIndex];
    }

    return nullptr;
}

ACheckout* AAICustomerPawn::FindOpenCheckout()
{
    TArray<AActor*> FoundCheckouts;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACheckout::StaticClass(), FoundCheckouts);

    for (AActor* CheckoutActor : FoundCheckouts)
    {
        ACheckout* Checkout = Cast<ACheckout>(CheckoutActor);
        if (IsValid(Checkout) && !Checkout->IsQueueFull())
        {
            return Checkout;
        }
    }

    return nullptr;
}

void AAICustomerPawn::DestroySelf()
{
    UE_LOG(LogTemp, Display, TEXT("AI %s: Destroying self."), *GetName());
    Destroy();
}
