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
    CurrentProductClass = nullptr;
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
    if (Products.Num() < MaxProducts && CurrentProductClass)
    {
        FVector SpawnLocation = ProductSpawnPoint->GetComponentLocation() +
            ProductSpawnPoint->GetComponentRotation().RotateVector(RelativeLocation);

        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;

        AProduct* NewProduct = GetWorld()->SpawnActor<AProduct>(
            CurrentProductClass,
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

    return false;
}



void AShelf::StartStockingShelf(TSubclassOf<AProduct> ProductToStock)
{
    if (!bIsStocking && ProductToStock)
    {
        CurrentProductClass = ProductToStock;
        bIsStocking = true;
        ContinueStocking();
    }
}


void AShelf::StopStockingShelf()
{
    bIsStocking = false;
    GetWorld()->GetTimerManager().ClearTimer(ContinuousStockingTimerHandle);
}

void AShelf::StockNextProduct()
{
    if (!bIsStocking || !CurrentProductClass)
    {
        return;
    }

    int32 currentProductCount = Products.Num();
    if (currentProductCount < MaxProducts)
    {
        int32 row = currentProductCount / 5;
        int32 column = currentProductCount % 5;

        FVector RelativeLocation = FVector(
            column * ProductSpacing.X,
            row * ProductSpacing.Y,
            ProductSpacing.Z
        );

        if (AddProduct(RelativeLocation))
        {
            GetWorld()->GetTimerManager().SetTimer(StockingTimerHandle, this, &AShelf::StockNextProduct, 0.3f, false);
        }
        else
        {
            bIsStocking = false;
        }
    }
    else
    {
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
        AProduct* RemovedProduct = Products.Last();
        Products.RemoveAt(Products.Num() - 1);
        RemovedProduct->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        return RemovedProduct;
    }
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

int32 AShelf::GetRemainingCapacity() const
{
    return FMath::Max(0, MaxProducts - GetProductCount());
}

void AShelf::ContinueStocking()
{
    if (!bIsStocking || !CurrentProductClass)
    {
        return;
    }

    int32 currentProductCount = Products.Num();
    if (currentProductCount < MaxProducts)
    {
        int32 row = currentProductCount / 5;
        int32 column = currentProductCount % 5;

        FVector RelativeLocation = FVector(
            column * ProductSpacing.X,
            row * ProductSpacing.Y,
            ProductSpacing.Z
        );

        if (AddProduct(RelativeLocation))
        {
            GetWorld()->GetTimerManager().SetTimer(ContinuousStockingTimerHandle, this, &AShelf::ContinueStocking, 0.3f, false);
        }
        else
        {
            bIsStocking = false;
        }
    }
    else
    {
        bIsStocking = false;
    }
}