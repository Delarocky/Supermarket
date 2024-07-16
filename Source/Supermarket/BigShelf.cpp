// BigShelf.cpp
#include "BigShelf.h"
#include "Shelf.h"

ABigShelf::ABigShelf()
{
    PrimaryActorTick.bCanEverTick = false;

    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
    RootComponent = RootSceneComponent;

    ShelfSpacing = FVector(0.0f, 100.0f, 0.0f); // Adjust as needed
}

void ABigShelf::BeginPlay()
{
    Super::BeginPlay();
    InitializeShelves();
}

void ABigShelf::CreateShelves()
{
    if (!ShelfClass)
    {
        UE_LOG(LogTemp, Error, TEXT("ShelfClass is not set in ABigShelf"));
        return;
    }

    for (int32 i = 0; i < 6; ++i)
    {
        FVector Location = GetActorLocation() + ShelfSpacing * i;
        FRotator Rotation = GetActorRotation();
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;

        AShelf* NewShelf = GetWorld()->SpawnActor<AShelf>(ShelfClass, Location, Rotation, SpawnParams);
        if (NewShelf)
        {
            NewShelf->AttachToComponent(RootSceneComponent, FAttachmentTransformRules::KeepWorldTransform);
            Shelves.Add(NewShelf);
        }
    }
}

void ABigShelf::InitializeShelves()
{
    CreateShelves();

    for (AShelf* Shelf : Shelves)
    {
        if (Shelf)
        {
            Shelf->UpdateProductSpawnPointRotation();
            Shelf->SetupAccessPoint();
            Shelf->InitializeShelf();
        }
    }
}

AShelf* ABigShelf::GetShelf(int32 Index) const
{
    if (Shelves.IsValidIndex(Index))
    {
        return Shelves[Index];
    }
    return nullptr;
}

void ABigShelf::StartStockingAllShelves(TSubclassOf<AProduct> ProductToStock)
{
    for (AShelf* Shelf : Shelves)
    {
        if (Shelf)
        {
            Shelf->StartStockingShelf(ProductToStock);
        }
    }
}

void ABigShelf::StopStockingAllShelves()
{
    for (AShelf* Shelf : Shelves)
    {
        if (Shelf)
        {
            Shelf->StopStockingShelf();
        }
    }
}

int32 ABigShelf::GetTotalProductCount() const
{
    int32 TotalCount = 0;
    for (const AShelf* Shelf : Shelves)
    {
        if (Shelf)
        {
            TotalCount += Shelf->GetProductCount();
        }
    }
    return TotalCount;
}

bool ABigShelf::AreAllShelvesFullyStocked() const
{
    for (const AShelf* Shelf : Shelves)
    {
        if (Shelf && !Shelf->IsFullyStocked())
        {
            return false;
        }
    }
    return true;
}