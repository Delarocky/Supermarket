// ProductBox.cpp
#include "ProductBox.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"

AProductBox::AProductBox()
{
    PrimaryActorTick.bCanEverTick = true;

    BoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxMesh"));
    RootComponent = BoxMesh;

    ProductSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ProductSpawnPoint"));
    ProductSpawnPoint->SetupAttachment(RootComponent);

    MaxProducts = 20;
    ProductSpacing = FVector(20.0f, 20.0f, 20.0f);
    bIsAttachedToCamera = false;
    // Initialize attachment properties
    CameraOffset = FVector(0.0f, 0.0f, 0.0f);
    CameraRotation = FRotator(0.0f, 0.0f, 0.0f);
}

void AProductBox::BeginPlay()
{
    Super::BeginPlay();
    /*
    if (ProductClass)
    {
        FillBox(ProductClass);
    }
    else
    {
        //UE_LOG(LogTemp, Warning, TEXT("ProductClass is not set in BeginPlay for ProductBox %s"), *GetName());
    }*/
}

void AProductBox::FillBox(TSubclassOf<AProduct> ProductToFill)
{
    if (!ProductToFill)
    {
        //UE_LOG(LogTemp, Warning, TEXT("ProductToFill is not set for ProductBox"));
        return;
    }

    SetProductClass(ProductToFill);

    if (!ProductClass)
    {
        //UE_LOG(LogTemp, Warning, TEXT("ProductClass is not set for ProductBox"));
        return;
    }

    // Clear existing products
    for (AProduct* Product : Products)
    {
        if (Product)
        {
            Product->Destroy();
        }
    }
    Products.Empty();

    // Fill the box with new products
    while (Products.Num() < MaxProducts)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AProduct* NewProduct = GetWorld()->SpawnActor<AProduct>(ProductClass, ProductSpawnPoint->GetComponentLocation(), ProductSpawnPoint->GetComponentRotation(), SpawnParams);
        if (NewProduct)
        {
            Products.Add(NewProduct);
            NewProduct->AttachToComponent(ProductSpawnPoint, FAttachmentTransformRules::KeepRelativeTransform);
        }
        else
        {
            //UE_LOG(LogTemp, Warning, TEXT("Failed to spawn product for ProductBox"));
            break;
        }
    }

    ArrangeProducts();
}

AProduct* AProductBox::RemoveProduct()
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


void AProductBox::ArrangeProducts()
{
    if (Products.Num() == 0 || !ProductSpawnPoint)
    {
        //UE_LOG(LogTemp, Warning, TEXT("No products or ProductSpawnPoint is null in ArrangeProducts"));
        return;
    }

    // Get the product data from the first product
    AProduct* FirstProduct = Products[0];
    if (!FirstProduct) return;
    FProductData ProductData = FirstProduct->GetProductData();

    // Get the bounds of the first product to use as a reference
    FVector ProductExtent = FVector::ZeroVector;
    UStaticMeshComponent* MeshComponent = FirstProduct->FindComponentByClass<UStaticMeshComponent>();
    if (MeshComponent)
    {
        ProductExtent = MeshComponent->Bounds.BoxExtent;
    }

    // Use the ProductSpawnPoint's location as the starting point
    FVector SpawnStart = ProductSpawnPoint->GetComponentLocation();
    FRotator SpawnRotation = ProductSpawnPoint->GetComponentRotation();

    // Calculate the offset to move the bottom center to the spawn point
    FVector CornerOffset = FVector(+ProductExtent.X, +ProductExtent.Y, +ProductExtent.Z);

    // Apply the ProductOffsetInBox to the spawn start
    SpawnStart += SpawnRotation.RotateVector(ProductOffsetInBox);

    //UE_LOG(LogTemp, Display, TEXT("Starting product arrangement at %s with rotation %s"),
    //    *SpawnStart.ToString(), *SpawnRotation.ToString());

    int32 ProductIndex = 0;
    for (int32 Z = 0; Z < ProductData.BoxGridSize.Z && ProductIndex < Products.Num(); ++Z)
    {
        for (int32 Y = 0; Y < ProductData.BoxGridSize.Y && ProductIndex < Products.Num(); ++Y)
        {
            for (int32 X = 0; X < ProductData.BoxGridSize.X && ProductIndex < Products.Num(); ++X)
            {
                FVector Offset = FVector(
                    X * (ProductData.BoxGridSpacing.X + 2 * ProductExtent.X),
                    Y * (ProductData.BoxGridSpacing.Y + 2 * ProductExtent.Y),
                    Z * (ProductData.BoxGridSpacing.Z + 2 * ProductExtent.Z)
                );

                // Apply the corner offset and the rotation of the ProductSpawnPoint to the offset
                FVector RotatedOffset = SpawnRotation.RotateVector(Offset + CornerOffset);
                FVector ProductLocation = SpawnStart + RotatedOffset;

                // Set the world location and rotation of the product
                Products[ProductIndex]->SetActorLocationAndRotation(ProductLocation, SpawnRotation);

                // Apply the product scale
                Products[ProductIndex]->SetActorScale3D(ProductData.Scale);

                //UE_LOG(LogTemp, Verbose, TEXT("Placed product %d at %s"), ProductIndex, *ProductLocation.ToString());

                ProductIndex++;
            }
        }
    }

    //UE_LOG(LogTemp, Display, TEXT("Arranged %d products in a %s grid with spacing %s"),
    //    ProductIndex, *ProductData.BoxGridSize.ToString(), *ProductData.BoxGridSpacing.ToString());
}

void AProductBox::AttachToCamera(UCameraComponent* Camera)
{
    if (Camera)
    {
        AttachedCamera = Camera;

        // Get the root component of the ProductBox
        USceneComponent* RootComp = GetRootComponent();
        if (RootComp)
        {
            // Detach from any previous parent
            RootComp->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

            // Calculate the new world location and rotation
            FVector NewLocation = Camera->GetComponentLocation() + Camera->GetComponentRotation().RotateVector(CameraOffset);
            FRotator NewRotation = Camera->GetComponentRotation() + CameraRotation;

            // Set the new world transform
            RootComp->SetWorldLocationAndRotation(NewLocation, NewRotation);

            // Now attach to the camera
            FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepWorld, true);
            RootComp->AttachToComponent(Camera, AttachmentRules);
        }

        bIsAttachedToCamera = true;
    }
}

void AProductBox::DetachFromCamera()
{
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    bIsAttachedToCamera = false;
    AttachedCamera = nullptr;
}

// Add to Tick function if not already present
void AProductBox::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsAttachedToCamera && AttachedCamera)
    {
        USceneComponent* RootComp = GetRootComponent();
        if (RootComp)
        {
            FVector NewLocation = AttachedCamera->GetComponentLocation() + AttachedCamera->GetComponentRotation().RotateVector(CameraOffset);
            FRotator NewRotation = AttachedCamera->GetComponentRotation() + CameraRotation;
            RootComp->SetWorldLocationAndRotation(NewLocation, NewRotation);
        }
    }
}

AProductBox* AProductBox::SpawnProductBox(UObject* WorldContextObject, TSubclassOf<AProductBox> ProductBoxClass, TSubclassOf<AProduct> ProductToSpawn, FVector SpawnLocation)
{
    if (!WorldContextObject || !ProductBoxClass || !ProductToSpawn)
    {
        //UE_LOG(LogTemp, Error, TEXT("Invalid parameters in SpawnProductBox"));
        return nullptr;
    }

    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
    if (!World)
    {
        //UE_LOG(LogTemp, Error, TEXT("Invalid World in SpawnProductBox"));
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    AProductBox* NewProductBox = World->SpawnActor<AProductBox>(ProductBoxClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);

    if (NewProductBox)
    {
        NewProductBox->SetProductClass(ProductToSpawn);

        // Get the product data from the product class
        if (AProduct* ProductCDO = ProductToSpawn.GetDefaultObject())
        {
            FProductData ProductData = ProductCDO->GetProductData();

            // Set up the product box using the product data
            NewProductBox->MaxProducts = ProductData.MaxProductsInBox;
            NewProductBox->ProductSpacing = ProductData.BoxGridSpacing;
            NewProductBox->GridSize = ProductData.BoxGridSize;

            if (NewProductBox->BoxMesh)
            {
                NewProductBox->BoxMesh->SetWorldScale3D(FVector(1.0f));
            }

            NewProductBox->FillBox(ProductToSpawn);

            //UE_LOG(LogTemp, Display, TEXT("Spawned ProductBox with %d %s at location %s"),
            //    NewProductBox->GetProductCount(),
             //   *ProductToSpawn->GetName(),
             //   *SpawnLocation.ToString());
        }
    }
    else
    {
        //UE_LOG(LogTemp, Error, TEXT("Failed to spawn ProductBox"));
    }

    return NewProductBox;
}

void AProductBox::SetProductClass(TSubclassOf<AProduct> NewProductClass)
{
    ProductClass = NewProductClass;
}

void AProductBox::AttachToComponent(USceneComponent* Parent)
{
    if (Parent)
    {
        USceneComponent* RootComp = GetRootComponent();
        if (RootComp)
        {
            FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
            RootComp->AttachToComponent(Parent, AttachmentRules);
        }
    }
}

AProduct* AProductBox::GetNextProduct() const
{
    if (Products.Num() > 0)
    {
        return Products.Last();
    }
    return nullptr;
}

int32 AProductBox::GetProductCountt() const
{
    return Products.Num();
}