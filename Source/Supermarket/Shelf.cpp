// Shelf.cpp
#include "Shelf.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/KismetMathLibrary.h"
#include "NavigationSystem.h"
#include "SceneBoxComponent.h"
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
    AccessPoint2 = CreateDefaultSubobject<USceneComponent>(TEXT("AccessPoint2"));
    AccessPoint2->SetupAttachment(RootComponent);
    AccessPoint3 = CreateDefaultSubobject<USceneComponent>(TEXT("AccessPoint3"));
    AccessPoint3->SetupAttachment(RootComponent);

    bStartFullyStocked = false; // Set default value
    CurrentProductClass = nullptr;

    //PlacementBox = CreateDefaultSubobject<USceneBoxComponent>(TEXT("PlacementBox"));
   // PlacementBox->SetupAttachment(RootComponent);
   // PlacementBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);

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

    StockingSpline = CreateDefaultSubobject<USplineComponent>(TEXT("StockingSpline"));
    StockingSpline->SetupAttachment(RootComponent);
}




void AShelf::SetupAccessPoint()
{
    if (ShelfMesh && AccessPoint && AccessPoint2 && AccessPoint3)
    {
        FVector ShelfWorldLocation = ShelfMesh->GetComponentLocation();
        FRotator ShelfWorldRotation = ShelfMesh->GetComponentRotation();

        FVector ForwardVector = ShelfWorldRotation.Vector();
        FVector RightVector = FRotationMatrix(ShelfWorldRotation).GetUnitAxis(EAxis::Y);

        // Setup AccessPoint
        FVector AccessPointLocation = ShelfWorldLocation + ForwardVector * AccessPointOffset.X + RightVector * AccessPointOffset.Y + FVector(0.0f, 0.0f, AccessPointOffset.Z);
        AccessPoint->SetWorldLocation(AccessPointLocation);

        // Setup AccessPoint2
        FVector AccessPoint2Location = ShelfWorldLocation + ForwardVector * AccessPoint2Offset.X + RightVector * AccessPoint2Offset.Y + FVector(0.0f, 0.0f, AccessPoint2Offset.Z);
        AccessPoint2->SetWorldLocation(AccessPoint2Location);

        // Setup AccessPoint3
        FVector AccessPoint3Location = ShelfWorldLocation + ForwardVector * AccessPoint3Offset.X + RightVector * AccessPoint3Offset.Y + FVector(0.0f, 0.0f, AccessPoint3Offset.Z);
        AccessPoint3->SetWorldLocation(AccessPoint3Location);
    }
}

FVector AShelf::GetAccessPointLocation() const
{
    return AccessPoint->GetComponentLocation();
}

TArray<FVector> AShelf::GetAllAccessPointLocations() const
{
    TArray<FVector> Locations;
    Locations.Add(AccessPoint->GetComponentLocation());
    Locations.Add(AccessPoint2->GetComponentLocation());
    Locations.Add(AccessPoint3->GetComponentLocation());
    return Locations;
}

void AShelf::BeginPlay()
{
    Super::BeginPlay();
    UpdateProductSpawnPointRotation();

    if (ShelfMesh)
    {
        OriginalMaterial = ShelfMesh->GetMaterial(0);
    }

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
        ////UE_LOGLogTemp, Log, TEXT("Updated ProductSpawnPoint rotation to: %s"), *NewRotation.ToString());
    }
    else
    {
        ////UE_LOGLogTemp, Warning, TEXT("ShelfMesh or ProductSpawnPoint is null in UpdateProductSpawnPointRotation"));
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
    if (bStartFullyStocked && ProductClass)
    {
        // Update ProductSpawnPoint based on ProductOffsetOnShelf
        ProductSpawnPoint->SetRelativeLocation(ProductOffsetOnShelf);

        // Stock the shelf to its maximum capacity or MaxProductsOnShelf, whichever is smaller
        int32 StockLimit = FMath::Min(MaxProducts, MaxProductsOnShelf);
        while (Products.Num() < StockLimit)
        {
            int32 currentProductCount = Products.Num();
            int32 row = currentProductCount / 5;  // Assuming 5 products per row
            int32 column = currentProductCount % 5;

            FVector RelativeLocation = FVector(
                column * ProductSpacing.X,
                row * ProductSpacing.Y,
                0  // Height is now handled by ProductSpawnPoint
            );

            AddProduct(RelativeLocation);
        }
        ////UE_LOGLogTemp, Display, TEXT("Shelf %s: Initialized with %d products"), *GetName(), Products.Num());
    }
    else
    {
        ////UE_LOGLogTemp, Display, TEXT("Shelf %s: Initialized without initial stock"), *GetName());
    }
}

bool AShelf::AddProduct(const FVector& RelativeLocation)
{
    if (Products.Num() < MaxProducts && ProductClass && ProductBox && ProductSpawnPoint)
    {
        // Check if the ProductBox has the correct product type
        if (ProductBox->GetProductClass() != ProductClass)
        {
           ////UE_LOGLogTemp, Warning, TEXT("Product in box does not match shelf's product type"));
            return false;
        }

        // Remove a product from the ProductBox
        AProduct* NewProduct = ProductBox->RemoveProduct();
        if (!NewProduct)
        {
           ////UE_LOGLogTemp, Warning, TEXT("Failed to remove product from ProductBox"));
            return false;
        }

        // Get the ProductSpawnPoint's transform
        FVector SpawnLocation = ProductSpawnPoint->GetComponentLocation();
        FRotator SpawnRotation = ProductSpawnPoint->GetComponentRotation();

        // Apply the product offset and grid position
        FVector TotalOffset = ProductOffsetOnShelf + RelativeLocation;
        SpawnLocation += SpawnRotation.RotateVector(TotalOffset);

        // Set the new location and rotation for the product
        NewProduct->SetActorLocation(SpawnLocation);
        NewProduct->SetActorRotation(SpawnRotation);

        // Adjust the product's position based on its mesh to align with the bottom
        UStaticMeshComponent* ProductMesh = NewProduct->FindComponentByClass<UStaticMeshComponent>();
        if (ProductMesh)
        {
            FVector MeshBounds = ProductMesh->Bounds.BoxExtent;
            FVector BottomOffset = FVector(0, 0, MeshBounds.Z);
            FVector AdjustedLocation = SpawnLocation + SpawnRotation.RotateVector(BottomOffset);
            NewProduct->SetActorLocation(AdjustedLocation);
        }

        Products.Add(NewProduct);
        NewProduct->AttachToComponent(ProductSpawnPoint, FAttachmentTransformRules::KeepWorldTransform);

        // Make the product visible and enable collision
        NewProduct->SetActorHiddenInGame(false);
        NewProduct->SetActorEnableCollision(true);

       ////UE_LOGLogTemp, Display, TEXT("Added product to shelf. Total products: %d"), Products.Num());

        return true;
    }

    return false;
}


bool AShelf::StartStockingShelf(TSubclassOf<AProduct> ProductToStock)
{
    if (!ProductToStock)
    {
       ////UE_LOGLogTemp, Warning, TEXT("Invalid ProductToStock provided"));
        return false;
    }

    if (bIsStocking)
    {
       ////UE_LOGLogTemp, Warning, TEXT("Shelf is already being stocked"));
        return false;
    }

    // Check if the shelf is empty or if the current product matches the new product
    if (Products.Num() == 0 || ProductClass == ProductToStock)
    {
        // Set the current product and update shelf settings
        SetCurrentProduct(ProductToStock);

        // Set the ProductClass and start stocking
        ProductClass = ProductToStock;
        bIsStocking = true;
        ContinueStocking();

       ////UE_LOGLogTemp, Display, TEXT("Started stocking shelf with product type: %s"), *ProductClass->GetName());
        return true;
    }
    else
    {
       ////UE_LOGLogTemp, Warning, TEXT("Cannot stock different product type. Shelf is not empty and dedicated to %s"),
       //     ProductClass ? *ProductClass->GetName() : TEXT("None"));
        return false;
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

    ////UE_LOGLogTemp, Display, TEXT("IsSpotEmpty: Location %s is %s"), *WorldLocation.ToString(), bIsEmpty ? TEXT("empty") : TEXT("not empty"));

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
    //////UE_LOGLogTemp, Display, TEXT("Shelf %s: Current product count: %d"), *GetName(), Products.Num());
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
    if (!bIsStocking || !ProductClass || !ProductBox)
    {
        ////UE_LOGLogTemp, Warning, TEXT("Cannot continue stocking. IsStocking: %d, ProductClass: %s, ProductBox: %s"),
        //    bIsStocking, ProductClass ? *ProductClass->GetName() : TEXT("None"), ProductBox ? TEXT("Valid") : TEXT("Invalid"));
        return;
    }

    if (Products.Num() < MaxProducts && ProductBox->GetProductCount() > 0)
    {
        if (!CurrentMovingProduct) // Only start moving a new product if there isn't one already moving
        {
            AProduct* NextProduct = ProductBox->GetNextProduct(); // Get the next product without removing it
            if (NextProduct)
            {
                UpdateStockingSpline();
                MoveProductAlongSpline(NextProduct);
            }
            else
            {
                bIsStocking = false;
            }
        }
    }
    else
    {
        bIsStocking = false;
    }
}

FVector AShelf::GetNextProductLocation() const
{
    return ProductSpawnPoint->GetComponentLocation() +
        ProductSpawnPoint->GetComponentRotation().RotateVector(CalculateProductPosition(Products.Num()));
}


void AShelf::UpdateStockingSpline()
{
    if (!StockingSpline || !ProductBox) return;

    StockingSpline->ClearSplinePoints();

    // Get the next product from the box without removing it
    AProduct* NextProduct = ProductBox->GetNextProduct();
    if (!NextProduct) return;

    // Start point (product's current location in the box)
    FVector StartPoint = NextProduct->GetActorLocation();
    StockingSpline->AddSplinePoint(StartPoint, ESplineCoordinateSpace::World);

    // Calculate the end point (center of where the product will land on the shelf)
    FVector EndPoint = GetNextProductLocation();

    // Adjust EndPoint to be at the center of the product
    UStaticMeshComponent* ProductMesh = NextProduct->FindComponentByClass<UStaticMeshComponent>();
    if (ProductMesh)
    {
        // Get the original (unscaled) bounds of the mesh
        FVector OriginalBounds = ProductMesh->GetStaticMesh()->GetBoundingBox().GetSize();

        // Get the current scale of the product
        FVector CurrentScale = NextProduct->GetActorScale3D();

        // Calculate the actual size of the product after scaling
        FVector ActualSize = OriginalBounds * CurrentScale;

        // Adjust the end point to be at the center of the product
        EndPoint += ProductSpawnPoint->GetComponentRotation().RotateVector(FVector(0, 0, ActualSize.Z * 0.5f));
    }

    // Mid point (arc above the shelf)
    FVector MidPoint = (StartPoint + EndPoint) * 0.5f + FVector(0, 0, 50);

    StockingSpline->AddSplinePoint(MidPoint, ESplineCoordinateSpace::World);
    StockingSpline->AddSplinePoint(EndPoint, ESplineCoordinateSpace::World);
}

void AShelf::MoveProductAlongSpline(AProduct* Product)
{
    if (!Product || !StockingSpline) return;

    CurrentMovingProduct = Product;
    SplineProgress = 0.0f;

    // Remove the product from the box
    if (ProductBox)
    {
        ProductBox->RemoveProduct();
    }

    // Detach the product from any parent
    Product->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

    // Start the movement timer
    GetWorld()->GetTimerManager().SetTimer(ProductMovementTimer, this, &AShelf::UpdateProductPosition, 0.016f, true);
}

void AShelf::UpdateProductPosition()
{
    if (!CurrentMovingProduct || !StockingSpline) return;

    SplineProgress += 0.08f; // Adjust this value to control speed

    if (SplineProgress >= 1.0f)
    {
        // Movement complete
        GetWorld()->GetTimerManager().ClearTimer(ProductMovementTimer);
        FinalizeProductPlacement(CurrentMovingProduct);
    }
    else
    {
        FVector NewLocation = StockingSpline->GetLocationAtTime(SplineProgress, ESplineCoordinateSpace::World);

        // Keep the product's original up vector (assuming Z is up)
        FVector UpVector = FVector::UpVector;

        // Get the product's forward vector (direction of movement)
        FVector ForwardVector = StockingSpline->GetDirectionAtTime(SplineProgress, ESplineCoordinateSpace::World);
        ForwardVector.Z = 0; // Ensure it's parallel to the ground
        ForwardVector.Normalize();

        // Calculate the right vector
        FVector RightVector = FVector::CrossProduct(ForwardVector, UpVector);

        // Construct the rotation matrix
        FRotator NewRotation = UKismetMathLibrary::MakeRotationFromAxes(ForwardVector, RightVector, UpVector);

        CurrentMovingProduct->SetActorLocation(NewLocation);
        CurrentMovingProduct->SetActorRotation(NewRotation);
    }
}

void AShelf::FinalizeProductPlacement(AProduct* Product)
{
    if (!Product || !ProductSpawnPoint) return;

    FVector RelativeLocation = CalculateProductPosition(Products.Num());
    FVector FinalLocation = ProductSpawnPoint->GetComponentLocation() +
        ProductSpawnPoint->GetComponentRotation().RotateVector(RelativeLocation);
    FRotator FinalRotation = ProductSpawnPoint->GetComponentRotation();

    UStaticMeshComponent* ProductMesh = Product->FindComponentByClass<UStaticMeshComponent>();
    if (ProductMesh)
    {
        FVector OriginalBounds = ProductMesh->GetStaticMesh()->GetBoundingBox().GetSize();
        FVector CurrentScale = Product->GetActorScale3D();
        FVector ActualSize = OriginalBounds * CurrentScale;
        FVector BottomCenterOffset = FVector(0, 0, ActualSize.Z * 0.5f);
        FinalLocation += ProductSpawnPoint->GetComponentRotation().RotateVector(BottomCenterOffset);
    }

    Product->SetActorLocation(FinalLocation);
    Product->SetActorRotation(FinalRotation);
    Product->AttachToComponent(ProductSpawnPoint, FAttachmentTransformRules::KeepWorldTransform);
    Products.Add(Product);

    CurrentMovingProduct = nullptr;
    ContinueStocking();
}

int32 AShelf::GetCurrentStock() const
{
    return Products.Num();
}

int32 AShelf::GetMaxStock() const
{
    return MaxProducts;
}

void AShelf::SetCurrentProduct(TSubclassOf<AProduct> NewProductClass)
{
    if (NewProductClass != CurrentProductClass)
    {
        CurrentProductClass = NewProductClass;
        UpdateShelfSettings();
    }
}

void AShelf::UpdateShelfSettings()
{
    if (CurrentProductClass)
    {
        if (FProductShelfSettings* Settings = ProductSettings.Find(CurrentProductClass))
        {
            ApplyProductSettings(*Settings);
        }
        else
        {
           ////UE_LOGLogTemp, Warning, TEXT("No settings found for product class %s. Using default settings."), *CurrentProductClass->GetName());
            // Apply default settings if no specific settings are found
            ApplyProductSettings(FProductShelfSettings());
        }
    }
}

void AShelf::ApplyProductSettings(const FProductShelfSettings& Settings)
{
    ProductSpacing = Settings.ProductSpacing;
    ProductOffsetOnShelf = Settings.ProductOffset;
    MaxProducts = Settings.MaxProducts;
    GridSize = Settings.GridSize;

    if (ProductSpawnPoint)
    {
        ProductSpawnPoint->SetRelativeLocation(ProductOffsetOnShelf);
    }

    int32 TotalGridSize = GridSize.X * GridSize.Y * GridSize.Z;
    MaxProducts = FMath::Min(MaxProducts, TotalGridSize);

    // Get the product's physical dimensions
    if (CurrentProductClass)
    {
        AProduct* TempProduct = GetWorld()->SpawnActor<AProduct>(CurrentProductClass);
        if (TempProduct)
        {
            UStaticMeshComponent* MeshComponent = TempProduct->FindComponentByClass<UStaticMeshComponent>();
            if (MeshComponent)
            {
                FVector Bounds = MeshComponent->Bounds.BoxExtent * 2; // Full dimensions
                ProductDimensions = Bounds;
            }
            TempProduct->Destroy();
        }
    }

    RearrangeProducts();

    while (Products.Num() > MaxProducts)
    {
        AProduct* RemovedProduct = Products.Pop();
        if (RemovedProduct)
        {
            RemovedProduct->Destroy();
        }
    }

    UpdateProductSpawnPointRotation();
}

FVector AShelf::CalculateProductPosition(int32 Index) const
{
    int32 X = Index % GridSize.X;
    int32 Y = (Index / GridSize.X) % GridSize.Y;
    int32 Z = Index / (GridSize.X * GridSize.Y);

    return FVector(
        X * (ProductDimensions.X + ProductSpacing.X),
        Y * (ProductDimensions.Y + ProductSpacing.Y),
        Z * (ProductDimensions.Z + ProductSpacing.Z)
    );
}

void AShelf::RearrangeProducts()
{
    for (int32 i = 0; i < Products.Num(); ++i)
    {
        if (Products[i])
        {
            FVector NewRelativeLocation = CalculateProductPosition(i);
            Products[i]->SetActorRelativeLocation(NewRelativeLocation);
        }
    }
}