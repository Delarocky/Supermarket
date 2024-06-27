// Shelf.cpp
#include "Shelf.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/KismetMathLibrary.h"
#include "NavigationSystem.h"
#include "AIController.h"

AShelf::AShelf()
{
    PrimaryActorTick.bCanEverTick = false;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    ProductSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ProductSpawnPoint"));
    ProductSpawnPoint->SetupAttachment(RootComponent);

    ShelfMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShelfMesh"));
    ShelfMesh->SetupAttachment(RootComponent);

    MaxProducts = 25; // 1x3x5 grid
    ProductSpacing = FVector(20.0f, 20.0f, 2.67071f); // Adjust as needed
 

    bIsStocking = false;

    AccessPoint = CreateDefaultSubobject<USceneComponent>(TEXT("AccessPoint"));
    AccessPoint->SetupAttachment(RootComponent);
    AccessPointOffset = FVector(100.0f, 0.0f, 0.0f);
}




void AShelf::SetupAccessPoint()
{
    if (ShelfMesh && AccessPoint)
    {
        FVector ShelfWorldLocation = ShelfMesh->GetComponentLocation();
        FRotator ShelfWorldRotation = ShelfMesh->GetComponentRotation();

        // Position the access point in front of the shelf
        FVector ForwardVector = ShelfWorldRotation.Vector();
        FVector RightVector = FRotationMatrix(ShelfWorldRotation).GetUnitAxis(EAxis::Y);

        FVector AccessPointLocation = ShelfWorldLocation + ForwardVector * AccessPointOffset.X + RightVector * AccessPointOffset.Y + FVector(0.0f, 0.0f, AccessPointOffset.Z);
        AccessPoint->SetWorldLocation(AccessPointLocation);
    }
}
FVector AShelf::GetAccessPointLocation() const
{
    return AccessPoint->GetComponentLocation();
}


void AShelf::BeginPlay()
{
    Super::BeginPlay();
    UpdateProductSpawnPointRotation();

    FVector Extent = ShelfMesh->Bounds.BoxExtent;
    SetupAccessPoint();
}

void AShelf::UpdateProductSpawnPointRotation()
{
    if (ShelfMesh && ProductSpawnPoint)
    {
        // Get the up vector of the shelf mesh
        FVector ShelfUpVector = ShelfMesh->GetUpVector();

        // Calculate the rotation needed to align the ProductSpawnPoint with the shelf's orientation
        FRotator NewRotation = UKismetMathLibrary::MakeRotFromZX(ShelfUpVector, ShelfMesh->GetForwardVector());

        // Set the new rotation for the ProductSpawnPoint
        ProductSpawnPoint->SetWorldRotation(NewRotation);

        // Optionally, log the new rotation for debugging
        UE_LOG(LogTemp, Log, TEXT("Updated ProductSpawnPoint rotation to: %s"), *NewRotation.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ShelfMesh or ProductSpawnPoint is null in UpdateProductSpawnPointRotation"));
    }
}


void AShelf::RotateShelf(FRotator NewRotation)
{
    SetActorRotation(NewRotation);
    UpdateProductSpawnPointRotation();

    // Reposition all existing products
    for (AProduct* Product : Products)
    {
        if (Product)
        {
            FVector RelativeLocation = Product->GetActorLocation() - ProductSpawnPoint->GetComponentLocation();
            RelativeLocation = ProductSpawnPoint->GetComponentRotation().UnrotateVector(RelativeLocation);

            FVector NewLocation = ProductSpawnPoint->GetComponentLocation() +
                ProductSpawnPoint->GetComponentRotation().RotateVector(RelativeLocation);

            Product->SetActorLocation(NewLocation);
            Product->SetActorRotation(ProductSpawnPoint->GetComponentRotation());
        }
    }
}


bool AShelf::AddProduct(const FVector& RelativeLocation)
{
    if (Products.Num() < MaxProducts && IsSpotEmpty(RelativeLocation))
    {
        FVector SpawnLocation = ProductSpawnPoint->GetComponentLocation() +
            ProductSpawnPoint->GetComponentRotation().RotateVector(RelativeLocation);

        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;

        if (DefaultProductClass)
        {
            AProduct* NewProduct = GetWorld()->SpawnActor<AProduct>(
                DefaultProductClass,
                SpawnLocation,
                ProductSpawnPoint->GetComponentRotation(),
                SpawnParams
            );

            if (NewProduct)
            {
                Products.Add(NewProduct);
                NewProduct->AttachToComponent(ProductSpawnPoint, FAttachmentTransformRules::KeepWorldTransform);
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

    int32 currentProductCount = Products.Num();
    if (currentProductCount < MaxProducts)
    {
        int32 row = currentProductCount / 5;  // Assuming 5 products per row
        int32 column = currentProductCount % 5;

        FVector RelativeLocation = FVector(
            column * ProductSpacing.X,
            row * ProductSpacing.Y,
            ProductSpacing.Z  // Height above the shelf
        );

        if (AddProduct(RelativeLocation))
        {
            UE_LOG(LogTemp, Display, TEXT("Shelf %s: Added product at relative location %s. Total products: %d"),
                *GetName(), *RelativeLocation.ToString(), Products.Num());

            // Schedule next product spawn
            GetWorld()->GetTimerManager().SetTimer(StockingTimerHandle, this, &AShelf::StockNextProduct, 0.3f, false);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Shelf %s: Failed to add product at relative location %s"),
                *GetName(), *RelativeLocation.ToString());
            bIsStocking = false;
        }
    }
    else
    {
       // UE_LOG(LogTemp, Display, TEXT("Shelf %s: Finished stocking. Total products: %d"), *GetName(), Products.Num());
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

AProduct* AShelf::RemoveNextProduct()
{
    if (Products.Num() > 0)
    {
        AProduct* RemovedProduct = Products[0];
        Products.RemoveAt(0);

        UE_LOG(LogTemp, Display, TEXT("Shelf %s: Removed product %s. Remaining products: %d"),
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
    //UE_LOG(LogTemp, Display, TEXT("Shelf %s: Current product count: %d"), *GetName(), Products.Num());
    return Products.Num();
}

bool AShelf::IsFullyStocked() const
{
    return Products.Num() >= MaxProducts;
}
