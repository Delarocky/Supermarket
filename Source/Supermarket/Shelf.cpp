// Shelf.cpp
#include "Shelf.h"
#include "Engine/World.h"
#include "TimerManager.h"

AShelf::AShelf()
{
    PrimaryActorTick.bCanEverTick = false;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    ProductSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ProductSpawnPoint"));
    ProductSpawnPoint->SetupAttachment(RootComponent);

    ShelfMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShelfMesh"));
    ShelfMesh->SetupAttachment(RootComponent);

    MaxProducts = 15; // 1x3x5 grid
    ProductSpacing = FVector(20.0f, 20.0f, 20.0f); // Adjust as needed

    bIsStocking = false;
}

void AShelf::BeginPlay()
{
    Super::BeginPlay();
    InitializeShelfStructure();
}

void AShelf::InitializeShelfStructure()
{
    // This function can be used to set up any additional initialization
}

bool AShelf::AddProduct(const FVector& RelativeLocation)
{
    if (Products.Num() < MaxProducts && IsSpotEmpty(RelativeLocation))
    {
        FVector SpawnLocation = ProductSpawnPoint->GetComponentLocation() + RelativeLocation;

        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;

        if (DefaultProductClass)
        {
            AProduct* NewProduct = GetWorld()->SpawnActor<AProduct>(DefaultProductClass, SpawnLocation, ProductSpawnPoint->GetComponentRotation(), SpawnParams);

            if (NewProduct)
            {
                Products.Add(NewProduct);
                NewProduct->AttachToComponent(ProductSpawnPoint, FAttachmentTransformRules::KeepWorldTransform);
                NewProduct->SetActorLocation(SpawnLocation);
                return true;
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("DefaultProductClass is not set in AShelf"));
        }
    }
    return false;
}

AProduct* AShelf::RemoveProduct(const FVector& RelativeLocation)
{
    FVector SpawnPointLocation = ProductSpawnPoint->GetComponentLocation();
    for (int32 i = 0; i < Products.Num(); ++i)
    {
        if (Products[i])
        {
            FVector ProductRelativeLocation = Products[i]->GetActorLocation() - SpawnPointLocation;
            if (ProductRelativeLocation.Equals(RelativeLocation, 1.0f))
            {
                AProduct* RemovedProduct = Products[i];
                Products.RemoveAt(i);
                return RemovedProduct;
            }
        }
    }
    return nullptr;
}

void AShelf::StartStockingShelf()
{
    //UE_LOG(LogTemp, Display, TEXT("Shelf %s: Starting to stock shelf."), *GetName());
    if (!bIsStocking)
    {
        bIsStocking = true;
        StockNextProduct();
    }
}

void AShelf::StopStockingShelf()
{
    //UE_LOG(LogTemp, Display, TEXT("Shelf %s: Stopping stocking process."), *GetName());
    bIsStocking = false;
    GetWorld()->GetTimerManager().ClearTimer(StockingTimerHandle);
}

void AShelf::StockNextProduct()
{
    if (!bIsStocking)
    {
        return;
    }

    bool productAdded = false;
    for (int32 x = 0; x < 5 && !productAdded; ++x)
    {
        for (int32 y = 0; y < 3 && !productAdded; ++y)
        {
            FVector RelativeLocation = FVector(x * ProductSpacing.X, y * ProductSpacing.Y, 0);
            if (AddProduct(RelativeLocation))
            {
                productAdded = true;
                //UE_LOG(LogTemp, Display, TEXT("Shelf %s: Added product at location %s."), *GetName(), *RelativeLocation.ToString());
                break;
            }
        }
    }

    if (productAdded && Products.Num() < MaxProducts)
    {
        GetWorld()->GetTimerManager().SetTimer(StockingTimerHandle, this, &AShelf::StockNextProduct, 0.3f, false);
    }
    else
    {
        //UE_LOG(LogTemp, Display, TEXT("Shelf %s: Finished stocking. Total products: %d"), *GetName(), Products.Num());
        bIsStocking = false;
    }
}

bool AShelf::IsSpotEmpty(const FVector& RelativeLocation) const
{
    FVector SpawnPointLocation = ProductSpawnPoint->GetComponentLocation();
    for (const AProduct* Product : Products)
    {
        if (Product)
        {
            FVector ProductRelativeLocation = Product->GetActorLocation() - SpawnPointLocation;
            if (ProductRelativeLocation.Equals(RelativeLocation, 1.0f))
            {
                return false;
            }
        }
    }
    return true;
}

AProduct* AShelf::RemoveRandomProduct()
{
    if (Products.Num() > 0)
    {
        int32 RandomIndex = FMath::RandRange(0, Products.Num() - 1);
        AProduct* RemovedProduct = Products[RandomIndex];
        Products.RemoveAt(RandomIndex);
        UE_LOG(LogTemp, Display, TEXT("Shelf %s: Removed random product %s. Remaining products: %d"),
            *GetName(), *RemovedProduct->GetName(), Products.Num());

        // Detach the product from the shelf
        RemovedProduct->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

        return RemovedProduct;
    }
    UE_LOG(LogTemp, Warning, TEXT("Shelf %s: Attempted to remove product, but shelf is empty."), *GetName());
    return nullptr;
}


int32 AShelf::GetProductCount() const
{
    UE_LOG(LogTemp, Display, TEXT("Shelf %s: Current product count: %d"), *GetName(), Products.Num());
    return Products.Num();
}

bool AShelf::IsFullyStocked() const
{
    return Products.Num() >= MaxProducts;
}