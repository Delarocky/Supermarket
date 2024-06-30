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
    ProductSpacing = FVector(10.0f, 10.0f, 10.0f);
    bIsAttachedToCamera = false;
    CameraOffset = FVector(50.0f, 0.0f, -50.0f);
}

void AProductBox::BeginPlay()
{
    Super::BeginPlay();
    FillBox();
}

void AProductBox::FillBox()
{
    if (!ProductClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("ProductClass is not set for ProductBox"));
        return;
    }

    while (Products.Num() < MaxProducts)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
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
        ArrangeProducts();
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