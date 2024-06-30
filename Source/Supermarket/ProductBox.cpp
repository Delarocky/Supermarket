// ProductBox.cpp
#include "ProductBox.h"

AProductBox::AProductBox()
{
    PrimaryActorTick.bCanEverTick = false;

    BoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxMesh"));
    RootComponent = BoxMesh;

    MaxProducts = 20;
    ProductSpacing = FVector(10.0f, 10.0f, 10.0f);
}

void AProductBox::BeginPlay()
{
    Super::BeginPlay();
}

void AProductBox::FillBox()
{
    if (!ProductClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("ProductClass is not set for ProductBox"));
        return;
    }

    int32 InitialProductCount = Products.Num();

    while (Products.Num() < MaxProducts)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        AProduct* NewProduct = GetWorld()->SpawnActor<AProduct>(ProductClass, GetActorLocation(), GetActorRotation(), SpawnParams);
        if (NewProduct)
        {
            Products.Add(NewProduct);
            NewProduct->AttachToComponent(BoxMesh, FAttachmentTransformRules::KeepRelativeTransform);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to spawn product for ProductBox"));
            break;
        }
    }

    if (Products.Num() > InitialProductCount)
    {
        ArrangeProducts();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No new products were added to the ProductBox"));
    }
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
    FVector BoxExtent = BoxMesh->Bounds.BoxExtent;
    FVector StartLocation = -BoxExtent + ProductSpacing;

    // Ensure we don't divide by zero
    int32 ProductsPerRow = (ProductSpacing.X > 0) ? FMath::FloorToInt(BoxExtent.X * 2 / ProductSpacing.X) : 1;
    int32 ProductsPerColumn = (ProductSpacing.Y > 0) ? FMath::FloorToInt(BoxExtent.Y * 2 / ProductSpacing.Y) : 1;

    // Ensure we have at least one product per row/column
    ProductsPerRow = FMath::Max(1, ProductsPerRow);
    ProductsPerColumn = FMath::Max(1, ProductsPerColumn);

    for (int32 i = 0; i < Products.Num(); ++i)
    {
        int32 Row = i / ProductsPerRow;
        int32 Column = i % ProductsPerRow;
        int32 Layer = Row / ProductsPerColumn;

        FVector RelativeLocation = StartLocation + FVector(
            Column * ProductSpacing.X,
            (Row % ProductsPerColumn) * ProductSpacing.Y,
            Layer * ProductSpacing.Z
        );

        Products[i]->SetActorRelativeLocation(RelativeLocation);
    }
}