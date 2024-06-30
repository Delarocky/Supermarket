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
    AccessPointOffset = FVector(0.0f, 0.0f, 0.0f);

    bStartFullyStocked = false; // Set default value
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
    InitializeShelf();
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

void AShelf::InitializeShelf()
{
    if (bStartFullyStocked)
    {
        // Stock the shelf to its maximum capacity
        while (Products.Num() < MaxProducts)
        {
            int32 currentProductCount = Products.Num();
            int32 row = currentProductCount / 5;  // Assuming 5 products per row
            int32 column = currentProductCount % 5;

            FVector RelativeLocation = FVector(
                column * ProductSpacing.X,
                row * ProductSpacing.Y,
                ProductSpacing.Z  // Height above the shelf
            );

            AddProduct(RelativeLocation);
        }
        UE_LOG(LogTemp, Display, TEXT("Shelf %s: Initialized as fully stocked with %d products"), *GetName(), Products.Num());
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("Shelf %s: Initialized without initial stock"), *GetName());
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
            // Spawn the product
            AProduct* NewProduct = GetWorld()->SpawnActor<AProduct>(
                DefaultProductClass,
                SpawnLocation,
                ProductSpawnPoint->GetComponentRotation(),
                SpawnParams
            );

            if (NewProduct)
            {
                // Get the product's mesh
                UStaticMeshComponent* ProductMesh = NewProduct->FindComponentByClass<UStaticMeshComponent>();
                if (ProductMesh)
                {
                    // Calculate the offset to align the bottom of the product with the spawn point
                    FVector MeshBounds = ProductMesh->Bounds.BoxExtent;
                    FVector BottomOffset = FVector(0, 0, MeshBounds.Z);

                    // Adjust the spawn location to align the bottom of the product with the spawn point
                    FVector AdjustedLocation = SpawnLocation + ProductSpawnPoint->GetComponentRotation().RotateVector(BottomOffset);
                    NewProduct->SetActorLocation(AdjustedLocation);

                    // Set the rotation to match the ProductSpawnPoint
                    NewProduct->SetActorRotation(ProductSpawnPoint->GetComponentRotation());
                }

                Products.Add(NewProduct);
                NewProduct->AttachToComponent(ProductSpawnPoint, FAttachmentTransformRules::KeepWorldTransform);

                UE_LOG(LogTemp, Display, TEXT("Added product %s at index %d"), *NewProduct->GetName(), Products.Num() - 1);
                return true;
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("DefaultProductClass is not set in AShelf"));
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Shelf %s: Failed to add product at relative location %s"), *GetName(), *RelativeLocation.ToString());
    return false;
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
    FVector WorldLocation = ProductSpawnPoint->GetComponentLocation() +
        ProductSpawnPoint->GetComponentRotation().RotateVector(RelativeLocation);

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    FCollisionShape CollisionShape = FCollisionShape::MakeBox(FVector(1.0f, 1.0f, 1.0f)); // Adjust the box size as needed

    bool bIsEmpty = !GetWorld()->OverlapBlockingTestByChannel(
        WorldLocation,
        FQuat::Identity,
        ECC_WorldStatic,
        CollisionShape,
        QueryParams
    );

    UE_LOG(LogTemp, Display, TEXT("IsSpotEmpty: Location %s is %s"), *WorldLocation.ToString(), bIsEmpty ? TEXT("empty") : TEXT("not empty"));

    return bIsEmpty;
}

AProduct* AShelf::RemoveNextProduct()
{
    if (Products.Num() > 0)
    {
        // Remove the last product in the array
        AProduct* RemovedProduct = Products.Last();
        Products.RemoveAt(Products.Num() - 1);

        if (RemovedProduct)
        {
            UE_LOG(LogTemp, Display, TEXT("Shelf %s: Removed product %s. Remaining products: %d"),
                *GetName(), *RemovedProduct->GetProductName(), Products.Num());

            // Detach the product from the shelf
            RemovedProduct->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

            // Optionally, hide or destroy the visual representation of the product
            RemovedProduct->SetActorHiddenInGame(true); // Example: Hide the product visually

            return RemovedProduct;
        }
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
