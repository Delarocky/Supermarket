#include "StorageRack.h"

AStorageRack::AStorageRack()
{
    PrimaryActorTick.bCanEverTick = true;

    RackMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RackMesh"));
    RootComponent = RackMesh;

    QuantityText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("QuantityText"));
    QuantityText->SetupAttachment(RootComponent);
    QuantityText->SetHorizontalAlignment(EHTA_Center);
    QuantityText->SetVerticalAlignment(EVRTA_TextCenter);
    QuantityText->SetWorldSize(72.0f);

    MaxCapacity = 100;
    CurrentStock = 0;
}

void AStorageRack::BeginPlay()
{
    Super::BeginPlay();
    UpdateQuantityText();
}

void AStorageRack::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateQuantityText();
}

AProductBox* AStorageRack::CreateProductBox(int32 Quantity)
{
    if (!CanCreateProductBox() || !ProductType)
    {
        UE_LOG(LogTemp, Error, TEXT("StorageRack: Cannot create product box. CanCreateProductBox: %s, ProductType: %s"),
            CanCreateProductBox() ? TEXT("True") : TEXT("False"),
            ProductType ? *ProductType->GetName() : TEXT("None"));
        return nullptr;
    }

    AProduct* ProductDefault = ProductType.GetDefaultObject();
    if (!ProductDefault)
    {
        UE_LOG(LogTemp, Error, TEXT("StorageRack: Failed to get default object for ProductType"));
        return nullptr;
    }

    FProductData ProductData = ProductDefault->GetProductData();

    int32 BoxQuantity = FMath::Min(Quantity, GetCurrentStock());
    CurrentStock -= BoxQuantity;

    UE_LOG(LogTemp, Log, TEXT("StorageRack: Creating box with %d products. Current stock: %d"), BoxQuantity, GetCurrentStock());

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    FVector SpawnLocation = GetActorLocation() + FVector(100.0f, 0.0f, 0.0f);
    FRotator SpawnRotation = GetActorRotation();

    AProductBox* NewBox = GetWorld()->SpawnActor<AProductBox>(ProductData.ProductBoxClass, SpawnLocation, SpawnRotation, SpawnParams);

    if (NewBox)
    {
        NewBox->SetProductClass(ProductType);
        NewBox->MaxProducts = ProductData.MaxProductsInBox;
        NewBox->ProductSpacing = ProductData.BoxGridSpacing;
        NewBox->GridSize = ProductData.BoxGridSize;
        NewBox->ProductOffsetInBox = ProductData.ProductOffsetInBox;

        NewBox->FillBox(ProductType, BoxQuantity);  // Pass BoxQuantity here

        UE_LOG(LogTemp, Log, TEXT("StorageRack: Created ProductBox with %d products. Box settings: MaxProducts=%d, GridSize=%s"),
            NewBox->GetProductCount(), NewBox->MaxProducts, *NewBox->GridSize.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("StorageRack: Failed to spawn ProductBox"));
    }

    UpdateQuantityText();
    return NewBox;
}

bool AStorageRack::ReplenishStock(int32 Quantity)
{
    int32 AvailableSpace = GetAvailableSpace();
    int32 ActualReplenishAmount = FMath::Min(Quantity, AvailableSpace);

    CurrentStock += ActualReplenishAmount;
    UpdateQuantityText();
    return ActualReplenishAmount > 0;
}

int32 AStorageRack::GetAvailableSpace() const
{
    return MaxCapacity - CurrentStock;
}

bool AStorageRack::CanCreateProductBox() const
{
    return CurrentStock > 0;
}

void AStorageRack::UpdateQuantityText()
{
    if (QuantityText && ProductType)
    {
        FString ProductName = ProductType.GetDefaultObject()->GetProductName();
        FString DisplayText = FString::Printf(TEXT("%s: %d/%d"), *ProductName, CurrentStock, MaxCapacity);
        QuantityText->SetText(FText::FromString(DisplayText));
    }
}

