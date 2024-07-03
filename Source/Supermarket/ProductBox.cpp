// ProductBox.cpp
#include "ProductBox.h"
#include "Camera/CameraComponent.h"

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
    CameraOffset = FVector(50.0f, 0.0f, -50.0f);
}

void AProductBox::BeginPlay()
{
    Super::BeginPlay();

    if (ProductClass)
    {
        FillBox(ProductClass);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ProductClass is not set in BeginPlay for ProductBox %s"), *GetName());
    }
}

void AProductBox::FillBox(TSubclassOf<AProduct> ProductToFill)
{
    if (!ProductToFill)
    {
        UE_LOG(LogTemp, Warning, TEXT("ProductToFill is not set for ProductBox"));
        return;
    }

    SetProductClass(ProductToFill);

    if (!ProductClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("ProductClass is not set for ProductBox"));
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
            UE_LOG(LogTemp, Warning, TEXT("Failed to spawn product for ProductBox"));
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
    int32 ProductsPerRow = 3;
    int32 ProductsPerColumn = 3;
    int32 ProductsPerLayer = ProductsPerRow * ProductsPerColumn;

    for (int32 i = 0; i < Products.Num(); ++i)
    {
        int32 Layer = i / ProductsPerLayer;
        int32 IndexInLayer = i % ProductsPerLayer;
        int32 Row = IndexInLayer / ProductsPerRow;
        int32 Column = IndexInLayer % ProductsPerRow;

        FVector RelativeLocation = FVector(
            Column * ProductSpacing.X,
            Row * ProductSpacing.Y,
            Layer * ProductSpacing.Z
        );

        Products[i]->SetActorRelativeLocation(RelativeLocation);
    }
}

void AProductBox::AttachToCamera(UCameraComponent* Camera)
{
    if (Camera)
    {
        AttachedCamera = Camera;
        AttachToComponent(Camera, FAttachmentTransformRules::KeepRelativeTransform);
        SetActorRelativeLocation(CameraOffset);
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
        SetActorRelativeLocation(CameraOffset);
    }
}

AProductBox* AProductBox::SpawnProductBox(UObject* WorldContextObject, TSubclassOf<AProduct> ProductToSpawn, int32 Quantity, FVector SpawnLocation)
{
    if (!WorldContextObject || !ProductToSpawn)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid WorldContextObject or ProductToSpawn in SpawnProductBox"));
        return nullptr;
    }

    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid World in SpawnProductBox"));
        return nullptr;
    }

    // Spawn the ProductBox
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    AProductBox* NewProductBox = World->SpawnActor<AProductBox>(AProductBox::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParams);

    if (NewProductBox)
    {
        // Set the product class and max products
        NewProductBox->SetProductClass(ProductToSpawn);
        NewProductBox->MaxProducts = FMath::Clamp(Quantity, 1, 100);  // Clamp quantity between 1 and 100

        // Manually fill the box with products
        for (int32 i = 0; i < NewProductBox->MaxProducts; ++i)
        {
            FVector ProductLocation = NewProductBox->ProductSpawnPoint->GetComponentLocation();
            FRotator ProductRotation = NewProductBox->ProductSpawnPoint->GetComponentRotation();

            FActorSpawnParameters ProductSpawnParams;
            ProductSpawnParams.Owner = NewProductBox;
            ProductSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

            AProduct* NewProduct = World->SpawnActor<AProduct>(ProductToSpawn, ProductLocation, ProductRotation, ProductSpawnParams);
            if (NewProduct)
            {
                NewProductBox->Products.Add(NewProduct);
                NewProduct->AttachToComponent(NewProductBox->ProductSpawnPoint, FAttachmentTransformRules::KeepRelativeTransform);
            }
        }

        // Arrange the products within the box
        NewProductBox->ArrangeProducts();

        UE_LOG(LogTemp, Display, TEXT("Spawned ProductBox with %d %s at location %s"),
            NewProductBox->Products.Num(),
            *ProductToSpawn->GetName(),
            *SpawnLocation.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to spawn ProductBox"));
    }

    return NewProductBox;
}


void AProductBox::SetProductClass(TSubclassOf<AProduct> NewProductClass)
{
    ProductClass = NewProductClass;
}