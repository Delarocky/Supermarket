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
#include "Kismet/GameplayStatics.h"

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
    if (bIsProcessingCustomer)
    {
        //UE_LOG(LogTemp, Warning, TEXT("Already processing a customer. Ignoring this call."));
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

            UE_LOG(LogTemp, Display, TEXT("Customer distance to queue front: %f, Processing distance: %f"),
                DistanceToQueueFront, ProcessingDistance);

            if (DistanceToQueueFront <= ProcessingDistance)
            {
                if (Customer && Customer->ShoppingBag)
                {
                    UE_LOG(LogTemp, Display, TEXT("Processing customer at front of queue."));

                    // Debug print the contents of the shopping bag
                    Customer->ShoppingBag->DebugPrintContents();

                    ProductsToScan = Customer->ShoppingBag->GetProducts();
                    UE_LOG(LogTemp, Display, TEXT("Products in bag: %d"), ProductsToScan.Num());

                    // Clear previous items and copy new items
                    ItemsOnCounter.Empty();
                    ItemsOnCounter = ProductsToScan;

                    UE_LOG(LogTemp, Display, TEXT("Items on counter: %d"), ItemsOnCounter.Num());

                    PlaceItemsOnCounter();

                    CurrentItemIndex = 0;
                    TotalAmount = 0.0f;
                    ScannedItems.Empty();
                    bIsProcessingCustomer = true;
                    MoveNextItemToScanPosition();
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("Error: Customer or ShoppingBag is null"));
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Customer not close enough to queue front to be processed."));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Error: First queue position is null"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Attempted to process a customer who is not at the front of the queue"));
    }
}

void ACheckout::PlaceItemsOnCounter()
{
    FVector GridOrigin = GridStartPoint->GetComponentLocation();
    FRotator StandingRotation = FRotator::MakeFromEuler(ItemStandingRotation);

    int32 ItemIndex = 0;
    for (int32 Y = 0; Y < GridSize.Y && ItemIndex < ItemsOnCounter.Num(); Y++)
    {
        for (int32 X = 0; X < GridSize.X && ItemIndex < ItemsOnCounter.Num(); X++)
        {
            AProduct* Product = ItemsOnCounter[ItemIndex];
            if (Product)
            {
                // Calculate the position on the grid
                FVector ItemLocation = GridOrigin + FVector(X * GridSpacing.X, Y * GridSpacing.Y, 0);


                // Set the product's location and rotation
                Product->SetActorLocation(ItemLocation);
                Product->SetActorRotation(StandingRotation);

                // Ensure the product is visible and has collision enabled
                Product->SetActorHiddenInGame(false);
                Product->SetActorEnableCollision(true);

                // If the product has a mesh component, make sure it's visible
                UStaticMeshComponent* ProductMesh = Product->FindComponentByClass<UStaticMeshComponent>();
                if (ProductMesh)
                {
                    ProductMesh->SetVisibility(true);
                }

                ItemIndex++;
            }
        }
    }
}

void ACheckout::MoveNextItemToScanPosition()
{
    if (ItemsOnCounter.Num() > 0)
    {
        AProduct* NextItem = ItemsOnCounter[0];
        FVector StartLocation = NextItem->GetActorLocation();
        FVector EndLocation = ScanPoint->GetComponentLocation();

        float Distance = FVector::Dist(StartLocation, EndLocation);
        float Duration = Distance / ItemMoveSpeed;

        FTimerDelegate TimerDelegate;
        TimerDelegate.BindUFunction(this, FName("UpdateItemPosition"), NextItem, StartLocation, EndLocation, 0.0f, Duration);
        GetWorld()->GetTimerManager().SetTimer(MoveItemTimerHandle, TimerDelegate, 0.016f, true);
    }
    else
    {
        // All items have been scanned
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
    if (Product && ScanItemAnimation && CheckoutMesh)
    {
        CheckoutMesh->PlayAnimation(ScanItemAnimation, false);
        TotalAmount += Product->GetPrice();
        ScannedItems.Add(Product);
        DisplayTotal(TotalAmount);

        DebugLog(FString::Printf(TEXT("Scanned item: %s, Price: %.2f, New Total: %.2f"),
            *Product->GetProductName(), Product->GetPrice(), TotalAmount));
    }
    else
    {
        DebugLog(FString::Printf(TEXT("Failed to scan item. Product: %s, Animation: %s, Mesh: %s"),
            Product ? TEXT("Valid") : TEXT("Invalid"),
            ScanItemAnimation ? TEXT("Valid") : TEXT("Invalid"),
            CheckoutMesh ? TEXT("Valid") : TEXT("Invalid")));
    }
}

void ACheckout::FinishTransaction()
{
    DebugLog(TEXT("FinishTransaction called"));

    if (FinishTransactionAnimation && CheckoutMesh)
    {
        CheckoutMesh->PlayAnimation(FinishTransactionAnimation, false);
    }

    bool PaymentSuccessful = ProcessPayment(TotalAmount);
    if (PaymentSuccessful)
    {
        DebugLog(FString::Printf(TEXT("Payment of $%.2f processed successfully for %d items"), TotalAmount, ScannedItems.Num()));
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
    for (int32 i = 0; i < CustomersInQueue.Num(); ++i)
    {
        if (CustomersInQueue[i] && QueuePositions.IsValidIndex(i))
        {
            FVector TargetLocation = QueuePositions[i]->GetComponentLocation();
            CustomersInQueue[i]->MoveTo(TargetLocation);

            // Calculate and set the target rotation for each customer
            SetCustomerTargetRotation(CustomersInQueue[i], i);
        }
    }

    // Trigger the rotation update for all customers
    StartRotationUpdate();

    if (CustomersInQueue.Num() > 0 && QueuePositions.Num() > 0)
    {
        AAICustomerPawn* FrontCustomer = CustomersInQueue[0];
        USceneComponent* FirstQueuePosition = QueuePositions[0];

        if (FrontCustomer && FirstQueuePosition)
        {
            FVector CustomerLocation = FrontCustomer->GetActorLocation();
            FVector QueueFrontLocation = FirstQueuePosition->GetComponentLocation();

            float DistanceToQueueFront = FVector::Dist(CustomerLocation, QueueFrontLocation);

            if (DistanceToQueueFront <= ProcessingDistance)
            {
                ProcessCustomer(FrontCustomer);
            }
            else
            {
                DebugLog(FString::Printf(TEXT("Front customer not close enough to queue front. Distance: %f"), DistanceToQueueFront));
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
        ItemsOnCounter.RemoveAt(0);

        if (ScannedItem)
        {
            ScannedItem->SetActorHiddenInGame(true);
            ScannedItem->SetActorEnableCollision(false);

            // If the product has a mesh component, make sure it's hidden
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

