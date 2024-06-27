// Product.cpp
#include "Product.h"
#include "UObject/ConstructorHelpers.h"

AProduct::AProduct()
{
    PrimaryActorTick.bCanEverTick = false;

    ProductMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProductMesh"));
    RootComponent = ProductMesh;

    // Load a default static mesh (cube)
    static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (DefaultMeshAsset.Succeeded())
    {
        ProductMesh->SetStaticMesh(DefaultMeshAsset.Object);
    }

    // Set default values
    ProductData.Name = "Default Product";
    ProductData.Price = 5.0f;
    ProductData.Scale = FVector(0.5f, 0.5f, 0.5f);

    ProductMesh->SetWorldScale3D(ProductData.Scale);
}

void AProduct::BeginPlay()
{
    Super::BeginPlay();
}

void AProduct::InitializeProduct(const FProductData& InProductData)
{
    ProductData = InProductData;
    SetActorScale3D(ProductData.Scale); // Ensure scale is set when initialized
}

FString AProduct::GetProductName() const
{
    return ProductData.Name;
}

float AProduct::GetPrice() const
{
    return ProductData.Price;
}

FProductData AProduct::GetProductData() const
{
    return ProductData;
}

#if WITH_EDITOR
void AProduct::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.Property != nullptr)
    {
        FName PropertyName = PropertyChangedEvent.Property->GetFName();

        if (PropertyName == GET_MEMBER_NAME_CHECKED(FProductData, Scale))
        {
            ProductMesh->SetWorldScale3D(ProductData.Scale);
        }
        else if (PropertyName == GET_MEMBER_NAME_CHECKED(AProduct, ProductData))
        {
            ProductMesh->SetWorldScale3D(ProductData.Scale);
            FString NewActorLabel = FString::Printf(TEXT("%s ($%.2f)"), *ProductData.Name, ProductData.Price);
            SetActorLabel(NewActorLabel);
        }
    }
}
#endif