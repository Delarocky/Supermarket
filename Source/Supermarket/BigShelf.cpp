// BigShelf.cpp
#include "BigShelf.h"
#include "Engine/World.h"

ABigShelf::ABigShelf()
{
    PrimaryActorTick.bCanEverTick = false;

    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
    RootComponent = RootSceneComponent;

   

    // Create shelves in the constructor
    CreateShelves();
}


void ABigShelf::BeginPlay()
{
    Super::BeginPlay();

    // Create the root scene component if it doesn't exist
    if (!RootSceneComponent)
    {
        RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
        RootComponent = RootSceneComponent;
    }

    // Create shelves
    CreateShelves();

    // Initialize shelves for gameplay
    for (AShelf* Shelf : Shelves)
    {
        if (Shelf)
        {
            Shelf->UpdateProductSpawnPointRotation();
            Shelf->SetupAccessPoint();
            Shelf->InitializeShelf();
        }
    }
    PreloadShelvesWithProduct();
}

// aligns the shelves in a 4x2 grid vertically
void ABigShelf::CreateShelves()
{
    ClearShelves();

    if (!ShelfClass)
    {
        //UE_LOG(LogTemp, Error, TEXT("ShelfClass is not set in ABigShelf"));
        return;
    }

    for (int32 Row = 0; Row < GridSize.Y; ++Row)
    {
        for (int32 Column = 0; Column < GridSize.X; ++Column)
        {
            FVector Location = FVector(Column * ShelfSpacing.X, 0.0f, -Row * ShelfSpacing.Z);
            FRotator Rotation = FRotator::ZeroRotator;
            FActorSpawnParameters SpawnParams;
            SpawnParams.Owner = this;

            AShelf* NewShelf = GetWorld()->SpawnActor<AShelf>(ShelfClass, Location, Rotation, SpawnParams);
            if (NewShelf)
            {
                NewShelf->AttachToComponent(RootSceneComponent, FAttachmentTransformRules::KeepRelativeTransform);
                Shelves.Add(NewShelf);
            }
        }
    }
}

// Makes any current shelves are cleared
void ABigShelf::ClearShelves()
{
    for (AShelf* Shelf : Shelves)
    {
        if (Shelf)
        {
            Shelf->Destroy();
        }
    }
    Shelves.Empty();
}



// IF set in the editor, you can preload the bigshelf with a select product for testing, on full release set this to none.
void ABigShelf::PreloadShelvesWithProduct()
{
    if (PreloadProductClass)
    {
        for (AShelf* Shelf : Shelves)
        {
            if (Shelf)
            {
                Shelf->SetProductClass(PreloadProductClass);
            }
        }
    }
}

#if WITH_EDITOR
void ABigShelf::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.Property)
    {
        FName PropertyName = PropertyChangedEvent.Property->GetFName();

        // Recreate shelves if GridSize, ShelfSpacing, or ShelfClass is changed
        if (PropertyName == GET_MEMBER_NAME_CHECKED(ABigShelf, GridSize) ||
            PropertyName == GET_MEMBER_NAME_CHECKED(ABigShelf, ShelfSpacing) ||
            PropertyName == GET_MEMBER_NAME_CHECKED(ABigShelf, ShelfClass))
        {
            CreateShelves();
        }
    }
}
#endif