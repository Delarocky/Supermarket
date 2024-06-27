// Checkout.cpp
#include "Checkout.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"

ACheckout::ACheckout()
{
    PrimaryActorTick.bCanEverTick = true;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    CounterMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CounterMesh"));
    CounterMesh->SetupAttachment(RootComponent);

    TotalDisplay = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TotalDisplay"));
    TotalDisplay->SetupAttachment(CounterMesh);

    ItemSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ItemSpawnPoint"));
    ItemSpawnPoint->SetupAttachment(CounterMesh);

    ItemEndPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ItemEndPoint"));
    ItemEndPoint->SetupAttachment(CounterMesh);

    for (int32 i = 0; i < MaxQueueSize; ++i)
    {
        FString ComponentName = FString::Printf(TEXT("QueuePosition%d"), i);
        USceneComponent* QueuePosition = CreateDefaultSubobject<USceneComponent>(*ComponentName);
        QueuePosition->SetupAttachment(RootComponent);
        QueuePositions.Add(QueuePosition);
    }

    ScanDuration = 1.0f;
    CurrentScanTime = 0.0f;
    CurrentItemIndex = 0;
    TotalAmount = 0.0f;
}

void ACheckout::BeginPlay()
{
    Super::BeginPlay();
    TotalDisplay->SetText(FText::FromString(TEXT("Total: $0.00")));
}

void ACheckout::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (CurrentCustomer && CurrentItemIndex < ItemsOnCounter.Num())
    {
        CurrentScanTime += DeltaTime;
        float Alpha = FMath::Clamp(CurrentScanTime / ScanDuration, 0.0f, 1.0f);

        FVector StartLocation = ItemSpawnPoint->GetComponentLocation();
        FVector EndLocation = ItemEndPoint->GetComponentLocation();
        FVector NewLocation = FMath::Lerp(StartLocation, EndLocation, Alpha);
        ItemsOnCounter[CurrentItemIndex]->SetActorLocation(NewLocation);

        if (Alpha >= 1.0f)
        {
            TotalAmount += ItemsOnCounter[CurrentItemIndex]->GetPrice();
            TotalDisplay->SetText(FText::FromString(FString::Printf(TEXT("Total: $%.2f"), TotalAmount)));
            ItemsOnCounter[CurrentItemIndex]->Destroy();
            CurrentItemIndex++;
            CurrentScanTime = 0.0f;

            if (CurrentItemIndex >= ItemsOnCounter.Num())
            {
                FinishCheckout();
            }
            else
            {
                ProcessNextItem();
            }
        }
    }
}

void ACheckout::ProcessCustomer(AAICustomerPawn* Customer)
{
    if (!CurrentCustomer)
    {
        CurrentCustomer = Customer;
        UShoppingBag* ShoppingBag = Customer->GetShoppingBag();
        TArray<FProductData> Products = ShoppingBag->GetProducts();

        for (const FProductData& ProductData : Products)
        {
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

            FVector SpawnLocation = ItemSpawnPoint->GetComponentLocation();
            FRotator SpawnRotation = ItemSpawnPoint->GetComponentRotation();

            AProduct* NewProduct = GetWorld()->SpawnActor<AProduct>(AProduct::StaticClass(), SpawnLocation, SpawnRotation, SpawnParams);
            if (NewProduct)
            {
                NewProduct->InitializeProduct(ProductData);
                ItemsOnCounter.Add(NewProduct);
            }
        }

        CurrentItemIndex = 0;
        TotalAmount = 0.0f;
        ProcessNextItem();
    }
    else
    {
        CustomerQueue.Add(Customer);
        UpdateQueuePositions();
    }
}

void ACheckout::ProcessNextItem()
{
    if (CurrentItemIndex < ItemsOnCounter.Num())
    {
        CurrentScanTime = 0.0f;
        ItemsOnCounter[CurrentItemIndex]->SetActorLocation(ItemSpawnPoint->GetComponentLocation());
    }
}

void ACheckout::FinishCheckout()
{
    if (CurrentCustomer)
    {
        UE_LOG(LogTemp, Display, TEXT("Checkout complete for customer %s. Total amount: $%.2f"), *CurrentCustomer->GetName(), TotalAmount);
        CurrentCustomer->OnCheckoutComplete();
        ResetCheckout();
    }
}

void ACheckout::ResetCheckout()
{
    CurrentCustomer = nullptr;
    ItemsOnCounter.Empty();
    CurrentItemIndex = 0;
    TotalAmount = 0.0f;
    TotalDisplay->SetText(FText::FromString(TEXT("Total: $0.00")));

    if (CustomerQueue.Num() > 0)
    {
        AAICustomerPawn* NextCustomer = CustomerQueue[0];
        CustomerQueue.RemoveAt(0);
        UpdateQueuePositions();
        ProcessCustomer(NextCustomer);
    }
}

void ACheckout::UpdateQueuePositions()
{
    for (int32 i = 0; i < CustomerQueue.Num(); ++i)
    {
        if (i < QueuePositions.Num())
        {
            FVector TargetLocation = QueuePositions[i]->GetComponentLocation();
            CustomerQueue[i]->SetActorLocation(TargetLocation);
        }
    }
}

bool ACheckout::IsQueueFull() const
{
    return CustomerQueue.Num() >= MaxQueueSize;
}

FVector ACheckout::GetNextQueuePosition() const
{
    int32 QueueIndex = CustomerQueue.Num();
    if (QueueIndex < QueuePositions.Num())
    {
        return QueuePositions[QueueIndex]->GetComponentLocation();
    }
    return FVector::ZeroVector;
}