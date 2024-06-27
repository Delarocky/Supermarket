// Checkout.cpp
#include "Checkout.h"
#include "AICustomerPawn.h"
#include "ShoppingBag.h"
#include "AIController.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Product.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"

ACheckout::ACheckout()
{
    PrimaryActorTick.bCanEverTick = true;

    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
    RootComponent = RootSceneComponent;

    ConveyorBelt = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ConveyorBelt"));
    ConveyorBelt->SetupAttachment(RootComponent);

    TotalProcessedText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TotalProcessedText"));
    TotalProcessedText->SetupAttachment(RootComponent);
    TotalProcessedText->SetHorizontalAlignment(EHTA_Center);
    TotalProcessedText->SetVerticalAlignment(EVRTA_TextCenter);
    TotalProcessedText->SetWorldSize(20.0f);

    ScannerLocation = CreateDefaultSubobject<USceneComponent>(TEXT("ScannerLocation"));
    ScannerLocation->SetupAttachment(RootComponent);

    for (int32 i = 0; i < 10; ++i)
    {
        FString ComponentName = FString::Printf(TEXT("QueuePosition%d"), i);
        USceneComponent* QueuePosition = CreateDefaultSubobject<USceneComponent>(*ComponentName);
        QueuePosition->SetupAttachment(RootSceneComponent);
        QueuePositions.Add(QueuePosition);
    }

    TotalProcessed = 0.0f;
    ProcessingTime = 1.0f;
    CurrentProcessingTime = 0.0f;
    CurrentProductIndex = 0;

    GridSizeX = 3;
    GridSizeY = 3;
    GridSpacing = 50.0f;
}

void ACheckout::BeginPlay()
{
    Super::BeginPlay();
    SetupQueuePositions();
    UpdateTotalProcessedText();
}

void ACheckout::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (CurrentlyProcessingCustomer && CurrentProducts.Num() > 0)
    {
        // Check if the customer is close enough to the first queue position
        FVector CustomerLocation = CurrentlyProcessingCustomer->GetActorLocation();
        FVector FirstQueuePosition = QueuePositions[0]->GetComponentLocation();
        float Distance = FVector::Dist(CustomerLocation, FirstQueuePosition);

        if (Distance <= 200.0f)
        {
            CurrentProcessingTime += DeltaTime;
            if (CurrentProcessingTime >= ProcessingTime)
            {
                ProcessNextProduct();
                CurrentProcessingTime = 0.0f;
            }
        }
        else
        {
            // Customer is not close enough, pause processing
            CurrentProcessingTime = 0.0f;
        }
    }
    else if (CustomerQueue.Num() > 0 && !CurrentlyProcessingCustomer)
    {
        AAICustomerPawn* NextCustomer = CustomerQueue[0];
        FVector CustomerLocation = NextCustomer->GetActorLocation();
        FVector FirstQueuePosition = QueuePositions[0]->GetComponentLocation();
        float Distance = FVector::Dist(CustomerLocation, FirstQueuePosition);

        if (Distance <= 200.0f)
        {
            ProcessCustomer(NextCustomer);
        }
    }
}

void ACheckout::SetupQueuePositions()
{
    for (int32 i = 0; i < QueuePositions.Num(); ++i)
    {
        QueuePositions[i]->SetRelativeLocation(FVector(-75.0f * i, 0.0f, 0.0f));
    }
}

FVector ACheckout::GetNextQueuePosition() const
{
    if (CustomerQueue.Num() < QueuePositions.Num())
    {
        return QueuePositions[CustomerQueue.Num()]->GetComponentLocation();
    }
    return GetActorLocation();
}

void ACheckout::AddCustomerToQueue(AAICustomerPawn* Customer)
{
    if (Customer && !CustomerQueue.Contains(Customer))
    {
        CustomerQueue.Add(Customer);
        UpdateQueue();
    }
}

void ACheckout::RemoveCustomerFromQueue(AAICustomerPawn* Customer)
{
    if (Customer)
    {
        CustomerQueue.Remove(Customer);
        UpdateQueue();
    }
}

void ACheckout::ProcessCustomer(AAICustomerPawn* Customer)
{
    if (Customer && CustomerQueue.Contains(Customer) && Customer == CustomerQueue[0])
    {
        CurrentlyProcessingCustomer = Customer;
        UShoppingBag* ShoppingBag = Customer->GetShoppingBag();
        if (ShoppingBag)
        {
            TArray<FProductData> Products = ShoppingBag->GetProducts();
            SpawnProductsOnGrid(Products);
            CurrentProductIndex = 0;
            ProcessNextProduct();
        }
    }
}

void ACheckout::UpdateQueue()
{
    for (int32 i = 0; i < CustomerQueue.Num(); ++i)
    {
        if (i < QueuePositions.Num() && CustomerQueue[i])
        {
            AAIController* AIController = Cast<AAIController>(CustomerQueue[i]->GetController());
            if (AIController)
            {
                FAIMoveRequest MoveRequest;
                MoveRequest.SetGoalLocation(QueuePositions[i]->GetComponentLocation());
                MoveRequest.SetAcceptanceRadius(50.0f);
                AIController->MoveTo(MoveRequest);
            }
        }
    }
}

bool ACheckout::IsQueueFull() const
{
    return CustomerQueue.Num() >= QueuePositions.Num();
}

void ACheckout::UpdateTotalProcessedText()
{
    FString Text = FString::Printf(TEXT("Current Total: $%.2f"), TotalProcessed);
    TotalProcessedText->SetText(FText::FromString(Text));
}

int32 ACheckout::GetCustomerQueuePosition(const AAICustomerPawn* Customer) const
{
    return CustomerQueue.Find(const_cast<AAICustomerPawn*>(Customer));
}

bool ACheckout::IsCustomerBeingProcessed(const AAICustomerPawn* Customer) const
{
    return CurrentlyProcessingCustomer == Customer;
}

FVector ACheckout::GetQueuePosition(int32 Index) const
{
    if (Index >= 0 && Index < QueuePositions.Num())
    {
        return QueuePositions[Index]->GetComponentLocation();
    }
    return GetActorLocation();
}

bool ACheckout::IsCustomerFirstInQueue(const AAICustomerPawn* Customer) const
{
    return CustomerQueue.Num() > 0 && CustomerQueue[0] == Customer;
}

void ACheckout::FinishProcessingCustomer()
{
    if (CurrentlyProcessingCustomer)
    {
        UE_LOG(LogTemp, Display, TEXT("Checkout %s: Finished processing customer. Total: $%.2f, Processed Items: %d"),
            *GetName(), TotalProcessed, CurrentProductIndex);

        RemoveCustomerFromQueue(CurrentlyProcessingCustomer);
        CurrentlyProcessingCustomer->OnCheckoutComplete();
        CurrentlyProcessingCustomer = nullptr;

        // Reset for the next customer
        TotalProcessed = 0.0f;
        UpdateTotalProcessedText();
        CurrentProductIndex = 0;
        CurrentProducts.Empty();
    }
}

void ACheckout::ProcessNextProduct()
{
    if (CurrentProductIndex < CurrentProducts.Num())
    {
        AProduct* Product = CurrentProducts[CurrentProductIndex];
        if (Product)
        {
            // Move the product to the scanner location
            Product->SetActorLocation(ScannerLocation->GetComponentLocation());

            // Add the price to the total
            TotalProcessed += Product->GetPrice();
            UpdateTotalProcessedText();

            // Log the scanned item
            UE_LOG(LogTemp, Display, TEXT("Checkout %s: Scanned item %s. Price: $%.2f, Total: $%.2f, Item %d of %d"),
                *GetName(), *Product->GetProductName(), Product->GetPrice(), TotalProcessed, CurrentProductIndex + 1, CurrentProducts.Num());

            // Destroy the product after scanning
            Product->Destroy();
        }
        CurrentProductIndex++;

        // Schedule the next product processing
        GetWorld()->GetTimerManager().SetTimer(ProcessingTimerHandle, this, &ACheckout::ProcessNextProduct, ProcessingTime, false);
    }
    else
    {
        FinishProcessingCustomer();
    }
}

void ACheckout::SpawnProductsOnGrid(const TArray<FProductData>& Products)
{
    // Clear any existing products
    for (AProduct* Product : CurrentProducts)
    {
        if (Product)
        {
            Product->Destroy();
        }
    }
    CurrentProducts.Empty();

    // Spawn new products on the grid
    FVector GridOrigin = ScannerLocation->GetComponentLocation();
    for (int32 i = 0; i < Products.Num() && i < GridSizeX * GridSizeY; ++i)
    {
        int32 Row = i / GridSizeX;
        int32 Col = i % GridSizeX;

        FVector SpawnLocation = GridOrigin + FVector(Col * GridSpacing, Row * GridSpacing, 50.0f);
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        AProduct* NewProduct = GetWorld()->SpawnActor<AProduct>(AProduct::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParams);
        if (NewProduct)
        {
            NewProduct->InitializeProduct(Products[i]);
            CurrentProducts.Add(NewProduct);

            // Ensure the product's mesh retains its material
            UStaticMeshComponent* MeshComponent = NewProduct->FindComponentByClass<UStaticMeshComponent>();
            if (MeshComponent && MeshComponent->GetStaticMesh())
            {
                const TArray<FStaticMaterial>& StaticMaterials = MeshComponent->GetStaticMesh()->GetStaticMaterials();
                for (int32 MatIndex = 0; MatIndex < StaticMaterials.Num(); ++MatIndex)
                {
                    UMaterialInterface* Material = StaticMaterials[MatIndex].MaterialInterface;
                    if (Material)
                    {
                        MeshComponent->SetMaterial(MatIndex, Material);
                    }
                }
            }

            UE_LOG(LogTemp, Display, TEXT("Checkout %s: Spawned product %s at location %s"),
                *GetName(), *NewProduct->GetProductName(), *SpawnLocation.ToString());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Checkout %s: Failed to spawn product at location %s"),
                *GetName(), *SpawnLocation.ToString());
        }
    }

    UE_LOG(LogTemp, Display, TEXT("Checkout %s: Spawned %d products on grid"), *GetName(), CurrentProducts.Num());
}