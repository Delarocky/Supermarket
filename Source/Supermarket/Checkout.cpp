// Checkout.cpp
#include "Checkout.h"
#include "AICustomerPawn.h"
#include "Product.h"
#include "ShoppingBag.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "SupermarketGameState.h"
#include "Kismet/GameplayStatics.h"
#include "CashierAI.h"
#include "SupermarketCharacter.h"

ACheckout::ACheckout()
{
    PrimaryActorTick.bCanEverTick = true;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    CheckoutMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CheckoutMesh"));
    CheckoutMesh->SetupAttachment(RootComponent);

    DisplayMonitor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DisplayMonitor"));
    DisplayMonitor->SetupAttachment(RootComponent);

    TotalText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TotalText"));
    TotalText->SetupAttachment(DisplayMonitor);

    PaymentSound = CreateDefaultSubobject<UAudioComponent>(TEXT("PaymentSound"));
    PaymentSound->SetupAttachment(RootComponent);

    GridStartPoint = CreateDefaultSubobject<USceneComponent>(TEXT("GridStartPoint"));
    GridStartPoint->SetupAttachment(RootComponent);

    ScanPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ScanPoint"));
    ScanPoint->SetupAttachment(RootComponent);

    PlayerDetectionArea = CreateDefaultSubobject<UBoxComponent>(TEXT("PlayerDetectionArea"));
    PlayerDetectionArea->SetupAttachment(RootComponent);
    PlayerDetectionArea->SetBoxExtent(FVector(100.0f, 100.0f, 100.0f));
    PlayerDetectionArea->SetCollisionProfileName(TEXT("OverlapAll"));
    PlayerDetectionArea->OnComponentBeginOverlap.AddDynamic(this, &ACheckout::OnPlayerEnterDetectionArea);
    PlayerDetectionArea->OnComponentEndOverlap.AddDynamic(this, &ACheckout::OnPlayerExitDetectionArea);

    bPlayerPresent = false;
    CurrentCashier = nullptr;

   

  

    // Load materials
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> ValidMat(TEXT("/Game/M_ValidPlacement"));
    if (ValidMat.Succeeded())
    {
        ValidPlacementMaterial = ValidMat.Object;
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> InvalidMat(TEXT("/Game/M_InvalidPlacement"));
    if (InvalidMat.Succeeded())
    {
        InvalidPlacementMaterial = InvalidMat.Object;
    }

    for (int32 i = 0; i < MaxQueueSize; ++i)
    {
        FString CompName = FString::Printf(TEXT("QueuePosition_%d"), i);
        USceneComponent* QueuePos = CreateDefaultSubobject<USceneComponent>(*CompName);
        QueuePos->SetupAttachment(RootComponent);
        QueuePos->SetRelativeLocation(FVector(-100.0f * i, 0, 0));
        QueuePositions.Add(QueuePos);
    }

    TotalAmount = 0.0f;
    CurrentItemIndex = 0;
}



void ACheckout::BeginPlay()
{
    Super::BeginPlay();

    if (DisplayMonitor)
    {
        DisplayMonitor->SetRelativeLocation(DisplayOffset);
    }

    if (CheckoutMesh)
    {
        OriginalMaterial = CheckoutMesh->GetMaterial(0);
    }


    if (TotalText)
    {
        TotalText->SetRelativeLocation(TextOffset);
        TotalText->SetText(FText::FromString("Total: $0.00"));
        TotalText->SetTextRenderColor(FColor::Green);
    }

    ResetCheckout();
}

void ACheckout::SetupUpdateQueueTimer()
{
    GetWorld()->GetTimerManager().SetTimer(UpdateQueueTimerHandle, this, &ACheckout::UpdateQueue, 0.5f, true);
}

bool ACheckout::ProcessPayment(float Amount)
{
    DisplayTotal(Amount);
    if (PaymentSound)
    {
        PaymentSound->Play();
    }
    return true;
}

void ACheckout::DisplayTotal(float Amount)
{
    if (TotalText)
    {
        FString TotalString = FString::Printf(TEXT("Total: $%.2f"), Amount);
        TotalText->SetText(FText::FromString(TotalString));
    }
    DebugLog(FString::Printf(TEXT("Total Amount: $%.2f"), Amount));
}

bool ACheckout::TryEnterQueue(AAICustomerPawn* Customer)
{
    if (CustomersInQueue.Num() < MaxQueueSize)
    {
        CustomersInQueue.Add(Customer);
        UpdateQueue();
        return true;
    }
    return false;
}

void ACheckout::ProcessCustomer(AAICustomerPawn* Customer)
{
    DebugLog(FString::Printf(TEXT("ProcessCustomer called for %s"), *GetNameSafe(Customer)));

    if (!CanProcessCustomers())
    {
        DebugLog(TEXT("Cannot process customer: No cashier or player present"));
        return;
    }

    if (bIsProcessingCustomer)
    {
        DebugLog(TEXT("Already processing a customer. Ignoring this call."));
        return;
    }

    if (CustomersInQueue.Num() > 0 && QueuePositions.Num() > 0 && CustomersInQueue[0] == Customer)
    {
        USceneComponent* FirstQueuePosition = QueuePositions[0];

        if (FirstQueuePosition)
        {
            FVector CustomerLocation = Customer->GetActorLocation();
            FVector QueueFrontLocation = FirstQueuePosition->GetComponentLocation();

            float DistanceToQueueFront = FVector::Dist(CustomerLocation, QueueFrontLocation);

            DebugLog(FString::Printf(TEXT("Customer distance to queue front: %f, Processing distance: %f"),
                DistanceToQueueFront, ProcessingDistance));

            if (DistanceToQueueFront <= ProcessingDistance)
            {
                if (Customer && Customer->ShoppingBag)
                {
                    DebugLog(TEXT("Processing customer at front of queue."));

                    Customer->ShoppingBag->DebugPrintContents();

                    ProductsToScan = Customer->ShoppingBag->GetProducts();
                    DebugLog(FString::Printf(TEXT("Products in bag: %d"), ProductsToScan.Num()));

                    ItemsOnCounter.Empty();
                    ItemsOnCounter = ProductsToScan;

                    DebugLog(FString::Printf(TEXT("Items on counter: %d"), ItemsOnCounter.Num()));

                    PlaceItemsOnCounter();

                    CurrentItemIndex = 0;
                    TotalAmount = 0.0f;
                    ScannedItems.Empty();
                    bIsProcessingCustomer = true;
                    MoveNextItemToScanPosition();
                }
                else
                {
                    DebugLog(TEXT("Error: Customer or ShoppingBag is null"));
                }
            }
            else
            {
                DebugLog(TEXT("Customer not close enough to queue front to be processed."));
            }
        }
        else
        {
            DebugLog(TEXT("Error: First queue position is null"));
        }
    }
    else
    {
        DebugLog(TEXT("Attempted to process a customer who is not at the front of the queue"));
    }
}

void ACheckout::PlaceItemsOnCounter()
{
    FVector GridOrigin = GridStartPoint->GetComponentLocation();
    FRotator GridRotation = GridStartPoint->GetComponentRotation();
    FRotator StandingRotation = FRotator::MakeFromEuler(ItemStandingRotation) + GridRotation;

    // Get the right (Y) and forward (X) vectors of the GridStartPoint
    FVector RightVector = GridStartPoint->GetRightVector();
    FVector ForwardVector = GridStartPoint->GetForwardVector();

    int32 ItemIndex = 0;
    for (int32 Y = 0; Y < GridSize.Y && ItemIndex < ItemsOnCounter.Num(); Y++)
    {
        for (int32 X = 0; X < GridSize.X && ItemIndex < ItemsOnCounter.Num(); X++)
        {
            AProduct* Product = ItemsOnCounter[ItemIndex];
            if (Product)
            {
                // Calculate the position on the grid aligned with GridStartPoint
                FVector ItemLocation = GridOrigin +
                    RightVector * X * GridSpacing.X +
                    ForwardVector * Y * GridSpacing.Y;

                // Get the product's mesh
                UStaticMeshComponent* ProductMesh = Product->FindComponentByClass<UStaticMeshComponent>();
                if (ProductMesh)
                {
                    // Get the original (unscaled) bounds of the mesh
                    FVector OriginalBounds = ProductMesh->GetStaticMesh()->GetBoundingBox().GetSize();

                    // Get the current scale of the product
                    FVector CurrentScale = Product->GetActorScale3D();

                    // Calculate the actual size of the product after scaling
                    FVector ActualSize = OriginalBounds * CurrentScale;

                    // Calculate the offset to align the bottom center of the product with the gizmo
                    FVector BottomCenterOffset = FVector(0, 0, ActualSize.Z * 0.5f);

                    // Set the product's location, aligning its bottom center with the calculated grid point
                    Product->SetActorLocation(ItemLocation + BottomCenterOffset);
                    Product->SetActorRotation(StandingRotation);

                    // Ensure the product is visible and has collision enabled
                    Product->SetActorHiddenInGame(false);
                    Product->SetActorEnableCollision(true);
                    ProductMesh->SetVisibility(true);

                    // Debug visualization
                   // if (bDebugMode)
                   // {
                   //     DrawDebugPoint(GetWorld(), ItemLocation, 10.0f, FColor::Red, false, 5.0f);
                   //     DrawDebugBox(GetWorld(), Product->GetActorLocation(), ActualSize * 0.5f, FColor::Green, false, 5.0f);
                   //     UE_LOG(LogTemp, Display, TEXT("Product %d placed at %s, Actual Size: %s, Scale: %s"),
                   //         ItemIndex, *Product->GetActorLocation().ToString(), *ActualSize.ToString(), *CurrentScale.ToString());
                   // }
                }

                ItemIndex++;
            }
        }
    }
}

void ACheckout::MoveNextItemToScanPosition()
{
    DebugLog(TEXT("MoveNextItemToScanPosition called"));
    if (ItemsOnCounter.Num() > 0)
    {
        AProduct* NextItem = ItemsOnCounter[0];
        if (NextItem)
        {
            FVector StartLocation = NextItem->GetActorLocation();
            FVector EndLocation = ScanPoint->GetComponentLocation();

            UStaticMeshComponent* ProductMesh = NextItem->FindComponentByClass<UStaticMeshComponent>();
            if (ProductMesh)
            {
                // Get the original (unscaled) bounds of the mesh
                FVector OriginalBounds = ProductMesh->GetStaticMesh()->GetBoundingBox().GetSize();

                // Get the current scale of the product
                FVector CurrentScale = NextItem->GetActorScale3D();

                // Calculate the actual size of the product after scaling
                FVector ActualSize = OriginalBounds * CurrentScale;

                // Adjust end location to align the bottom center of the product with the ScanPoint
                EndLocation += FVector::UpVector * (ActualSize.Z * 0.5f);

                float Distance = FVector::Dist(StartLocation, EndLocation);
                float MoveSpeed = CurrentCashier ? CurrentCashier->GetInterpSpeed() : ItemMoveSpeed;
                float Duration = Distance / MoveSpeed;

                DebugLog(FString::Printf(TEXT("Moving item to scan position. Start: %s, End: %s, Speed: %f, Duration: %f, ActualSize: %s"),
                    *StartLocation.ToString(), *EndLocation.ToString(), MoveSpeed, Duration, *ActualSize.ToString()));

                FTimerDelegate TimerDelegate;
                TimerDelegate.BindUFunction(this, FName("UpdateItemPosition"), NextItem, StartLocation, EndLocation, 0.0f, Duration);
                GetWorld()->GetTimerManager().SetTimer(MoveItemTimerHandle, TimerDelegate, 0.016f, true);

               
            }
            else
            {
                DebugLog(TEXT("Product mesh not found. Cannot move item."));
                ScanNextItem();
            }
        }
    }
    else
    {
        DebugLog(TEXT("No items left on counter. Proceeding to scan next item."));
        ScanNextItem();
    }
}

void ACheckout::UpdateItemPosition(AProduct* Item, FVector StartLocation, FVector EndLocation, float ElapsedTime, float Duration)
{
    if (Item && ElapsedTime < Duration)
    {
        float Alpha = ElapsedTime / Duration;
        FVector NewLocation = FMath::Lerp(StartLocation, EndLocation, Alpha);

        Item->SetActorLocation(NewLocation);

        FTimerDelegate TimerDelegate;
        TimerDelegate.BindUFunction(this, FName("UpdateItemPosition"), Item, StartLocation, EndLocation, ElapsedTime + 0.016f, Duration);
        GetWorld()->GetTimerManager().SetTimer(MoveItemTimerHandle, TimerDelegate, 0.016f, false);
    }
    else
    {
        GetWorld()->GetTimerManager().ClearTimer(MoveItemTimerHandle);
        ScanNextItem();
    }
}

void ACheckout::ScanNextItem()
{
    DebugLog(FString::Printf(TEXT("ScanNextItem called. CurrentItemIndex: %d, ProductsToScan: %d"),
        CurrentItemIndex, ProductsToScan.Num()));

    if (CurrentItemIndex < ProductsToScan.Num())
    {
        AProduct* ProductToScan = ProductsToScan[CurrentItemIndex];
        if (ProductToScan)
        {
            DebugLog(FString::Printf(TEXT("Scanning product: %s"), *ProductToScan->GetProductName()));
            ScanItem(ProductToScan);
            CurrentItemIndex++;

            RemoveScannedItem();

            // Move the next item after a short delay
            GetWorld()->GetTimerManager().SetTimer(ScanItemTimerHandle, this, &ACheckout::MoveNextItemToScanPosition, TimeBetweenScans, false);
        }
        else
        {
            DebugLog(TEXT("Error: Product to scan is null"));
            CurrentItemIndex++;
            ScanNextItem();
        }
    }
    else
    {
        DebugLog(TEXT("All items scanned. Finishing transaction."));
        FinishTransaction();
    }
}

void ACheckout::ScanItem(AProduct* Product)
{
    DebugLog(FString::Printf(TEXT("ScanItem called for %s"), *GetNameSafe(Product)));

    if (!CanProcessCustomers())
    {
        DebugLog(TEXT("Cannot scan item: No cashier or player present"));
        return;
    }

    if (Product && ScanItemAnimation)
    {
       
        TotalAmount += Product->GetPrice();
        ScannedItems.Add(Product);
        DisplayTotal(TotalAmount);

        DebugLog(FString::Printf(TEXT("Scanned item: %s, Price: %.2f, New Total: %.2f"),
            *Product->GetProductName(), Product->GetPrice(), TotalAmount));

        float ScanDelay = CurrentCashier ? CurrentCashier->GetProcessingDelay() : TimeBetweenScans;
        GetWorld()->GetTimerManager().SetTimer(ScanItemTimerHandle, this, &ACheckout::ScanNextItem, ScanDelay, false);

        DebugLog(FString::Printf(TEXT("Next scan scheduled in %.2f seconds"), ScanDelay));
    }
    else
    {
        DebugLog(FString::Printf(TEXT("Failed to scan item. Product: %s, Animation: %s"),
            Product ? TEXT("Valid") : TEXT("Invalid"),
            ScanItemAnimation ? TEXT("Valid") : TEXT("Invalid")));
    }
}

void ACheckout::FinishTransaction()
{
    DebugLog(TEXT("FinishTransaction called"));

    bool PaymentSuccessful = ProcessPayment(TotalAmount);
    if (PaymentSuccessful)
    {
        DebugLog(FString::Printf(TEXT("Payment of $%.2f processed successfully for %d items"), TotalAmount, ScannedItems.Num()));

        // Update the total money in the game state
        if (ASupermarketGameState* GameState = GetWorld()->GetGameState<ASupermarketGameState>())
        {
            GameState->AddMoney(TotalAmount);
        }
    }
    else
    {
        DebugLog(FString::Printf(TEXT("Payment of $%.2f failed for %d items"), TotalAmount, ScannedItems.Num()));
    }

    if (CustomersInQueue.Num() > 0)
    {
        AAICustomerPawn* ProcessedCustomer = CustomersInQueue[0];
        CustomerLeft(ProcessedCustomer);

        // Destroy the products in the customer's shopping bag
        if (ProcessedCustomer && ProcessedCustomer->ShoppingBag)
        {
            ProcessedCustomer->ShoppingBag->DestroyProducts();
        }

        ProcessedCustomer->LeaveCheckout();
    }

    ScannedItems.Empty();
    ProductsToScan.Empty();
    TotalAmount = 0.0f;
    CurrentItemIndex = 0;
    DisplayTotal(0.0f);
    bIsProcessingCustomer = false;

    DebugLog(TEXT("Transaction finished. Checkout reset for next customer."));
}

void ACheckout::CustomerLeft(AAICustomerPawn* Customer)
{
    CustomersInQueue.Remove(Customer);
    CustomerTargetRotations.Remove(Customer);
    UpdateQueue();
}

void ACheckout::UpdateQueue()
{
   // DebugLog(TEXT("UpdateQueue called"));

    for (int32 i = 0; i < CustomersInQueue.Num(); ++i)
    {
        if (CustomersInQueue[i] && QueuePositions.IsValidIndex(i))
        {
            FVector TargetLocation = QueuePositions[i]->GetComponentLocation();
            CustomersInQueue[i]->MoveTo(TargetLocation);

            SetCustomerTargetRotation(CustomersInQueue[i], i);

            DebugLog(FString::Printf(TEXT("Customer %d moved to position %s"), i, *TargetLocation.ToString()));
        }
    }

    StartRotationUpdate();

    //DebugLog(FString::Printf(TEXT("Queue updated. Customers in queue: %d"), CustomersInQueue.Num()));

    if (CustomersInQueue.Num() > 0 && QueuePositions.Num() > 0)
    {
        AAICustomerPawn* FrontCustomer = CustomersInQueue[0];
        USceneComponent* FirstQueuePosition = QueuePositions[0];

        if (FrontCustomer && FirstQueuePosition)
        {
            FVector CustomerLocation = FrontCustomer->GetActorLocation();
            FVector QueueFrontLocation = FirstQueuePosition->GetComponentLocation();

            float DistanceToQueueFront = FVector::Dist(CustomerLocation, QueueFrontLocation);

            DebugLog(FString::Printf(TEXT("Front customer distance to queue: %f"), DistanceToQueueFront));

            if (DistanceToQueueFront <= ProcessingDistance && CanProcessCustomers())
            {
                DebugLog(TEXT("Processing front customer"));
                ProcessCustomer(FrontCustomer);
            }
            else
            {
                DebugLog(FString::Printf(TEXT("Front customer not ready for processing. Distance: %f, Can Process: %s"),
                    DistanceToQueueFront, CanProcessCustomers() ? TEXT("Yes") : TEXT("No")));
            }
        }
    }
}


void ACheckout::SetCustomerTargetRotation(AAICustomerPawn* Customer, int32 CustomerIndex)
{
    if (!Customer || !CheckoutMesh)
        return;

    FRotator TargetRotation;

    if (CustomerIndex == 0)
    {
        // Front customer faces the checkout
        FVector DirectionToCheckout = CheckoutMesh->GetComponentLocation() - Customer->GetActorLocation();
        DirectionToCheckout.Z = 0; // Ignore height difference
        TargetRotation = DirectionToCheckout.Rotation();
    }
    else if (CustomersInQueue.IsValidIndex(CustomerIndex - 1))
    {
        // Other customers face the customer in front
        AAICustomerPawn* CustomerInFront = CustomersInQueue[CustomerIndex - 1];
        FVector DirectionToFront = CustomerInFront->GetActorLocation() - Customer->GetActorLocation();
        DirectionToFront.Z = 0; // Ignore height difference
        TargetRotation = DirectionToFront.Rotation();
    }

    // Store the target rotation for this customer
    CustomerTargetRotations.Add(Customer, TargetRotation);
}


void ACheckout::RemoveScannedItem()
{
    if (ItemsOnCounter.Num() > 0)
    {
        AProduct* ScannedItem = ItemsOnCounter[0];
        ItemsOnCounter.RemoveAt(0);  // Remove from the start of the array

        if (ScannedItem)
        {
            ScannedItem->SetActorHiddenInGame(true);
            ScannedItem->SetActorEnableCollision(false);

            UStaticMeshComponent* ProductMesh = ScannedItem->FindComponentByClass<UStaticMeshComponent>();
            if (ProductMesh)
            {
                ProductMesh->SetVisibility(false);
            }
        }
    }
}

void ACheckout::UpdateCustomerRotations()
{
    bool AllCustomersRotated = true;

    for (auto& Pair : CustomerTargetRotations)
    {
        AAICustomerPawn* Customer = Pair.Key;
        FRotator TargetRotation = Pair.Value;

        if (Customer)
        {
            FRotator CurrentRotation = Customer->GetActorRotation();
            FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, GetWorld()->GetDeltaSeconds(), RotationSpeed);
            Customer->SetActorRotation(NewRotation);

            // Check if this customer has reached its target rotation
            if (!NewRotation.Equals(TargetRotation, 1.0f))
            {
                AllCustomersRotated = false;
            }
        }
    }

    // If all customers have reached their target rotations, stop the update timer
    if (AllCustomersRotated)
    {
        GetWorld()->GetTimerManager().ClearTimer(RotationUpdateTimerHandle);
    }
}

void ACheckout::StartRotationUpdate()
{
    // Clear any existing rotation update timer
    GetWorld()->GetTimerManager().ClearTimer(RotationUpdateTimerHandle);

    // Start a new timer to update rotations smoothly
    GetWorld()->GetTimerManager().SetTimer(RotationUpdateTimerHandle, this, &ACheckout::UpdateCustomerRotations, 0.016f, true);
}

void ACheckout::ResetCheckout()
{
    DebugLog(TEXT("Resetting checkout state"));

    for (AAICustomerPawn* Customer : CustomersInQueue)
    {
        if (Customer)
        {
            Customer->LeaveCheckout();
        }
    }
    CustomersInQueue.Empty();
    // Clear the rotation data
    CustomerTargetRotations.Empty();
    GetWorld()->GetTimerManager().ClearTimer(RotationUpdateTimerHandle);
    ScannedItems.Empty();
    ProductsToScan.Empty();
    CurrentItemIndex = 0;
    TotalAmount = 0.0f;
    bIsProcessingCustomer = false;

    GetWorldTimerManager().ClearTimer(ScanItemTimerHandle);
    GetWorldTimerManager().ClearTimer(UpdateQueueTimerHandle);
    GetWorldTimerManager().ClearTimer(MoveItemTimerHandle);

    for (AProduct* Product : ItemsOnCounter)
    {
        if (Product)
        {
            Product->SetActorHiddenInGame(true);
            Product->SetActorEnableCollision(false);
        }
    }
    ItemsOnCounter.Empty();

    DisplayTotal(0.0f);

    if (CheckoutMesh)
    {
        CheckoutMesh->Stop();
    }

    SetupUpdateQueueTimer();

    DebugLog(TEXT("Checkout reset complete"));
    DebugLogQueueState();
}

void ACheckout::DebugLogQueueState()
{
    DebugLog(TEXT("Current Queue State:"));
    DebugLog(FString::Printf(TEXT("Queue Size: %d, Max Queue Size: %d"), CustomersInQueue.Num(), MaxQueueSize));
    for (int32 i = 0; i < CustomersInQueue.Num(); ++i)
    {
        DebugLog(FString::Printf(TEXT("Customer %d: %s"), i, *CustomersInQueue[i]->GetName()));
    }
}

void ACheckout::DebugLogScanState()
{
    DebugLog(TEXT("Current Scan State:"));
    DebugLog(FString::Printf(TEXT("Total Products to Scan: %d, Current Index: %d"), ProductsToScan.Num(), CurrentItemIndex));
    DebugLog(FString::Printf(TEXT("Scanned Items: %d, Total Amount: %.2f"), ScannedItems.Num(), TotalAmount));
    DebugLog(FString::Printf(TEXT("Items on Counter: %d"), ItemsOnCounter.Num()));
}

void ACheckout::DebugLog(const FString& Message)
{
    if (bDebugMode)
    {
        UE_LOG(LogTemp, Display, TEXT("[ACheckout Debug] %s"), *Message);

        // You can also add on-screen debug messages here if needed
        // GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, Message);
    }
}

void ACheckout::SetCashier(ACashierAI* NewCashier)
{
    if (NewCashier && !CurrentCashier)
    {
        CurrentCashier = NewCashier;
        DebugLog(FString::Printf(TEXT("Cashier set: %s"), *GetNameSafe(NewCashier)));
    }
    else if (CurrentCashier)
    {
        DebugLog(TEXT("Cannot set cashier: Checkout already has a cashier"));
    }
    else
    {
        DebugLog(TEXT("Cannot set cashier: Invalid cashier provided"));
    }
}

void ACheckout::RemoveCashier()
{
    if (CurrentCashier)
    {
        CurrentCashier = nullptr;
        DebugLog(TEXT("Cashier removed"));
    }
    else
    {
        DebugLog(TEXT("Cannot remove cashier: No cashier assigned"));
    }
}

void ACheckout::OnPlayerEnterDetectionArea(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (Cast<ASupermarketCharacter>(OtherActor))
    {
        bPlayerPresent = true;
        DebugLog(TEXT("Player entered detection area"));
    }
}

void ACheckout::OnPlayerExitDetectionArea(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (Cast<ASupermarketCharacter>(OtherActor))
    {
        bPlayerPresent = false;
        DebugLog(TEXT("Player exited detection area"));
    }
}

bool ACheckout::CanProcessCustomers() const
{
    return bPlayerPresent || CurrentCashier != nullptr;
}

FVector ACheckout::GetCashierPosition() const
{
    return GetActorLocation() + GetActorRotation().RotateVector(CashierPositionOffset);
}
