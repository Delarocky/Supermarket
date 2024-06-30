// Copyright Epic Games, Inc. All Rights Reserved.

#include "SupermarketCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Shelf.h" // Make sure this include is present

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ASupermarketCharacter

ASupermarketCharacter::ASupermarketCharacter()
{
    // Set size for collision capsule
    GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

    // Create a CameraComponent    
    FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
    FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
    FirstPersonCameraComponent->bUsePawnControlRotation = true;

    // Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
    Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
    Mesh1P->SetOnlyOwnerSee(true);
    Mesh1P->SetupAttachment(FirstPersonCameraComponent);
    Mesh1P->bCastDynamicShadow = false;
    Mesh1P->CastShadow = false;
    //Mesh1P->SetRelativeRotation(FRotator(0.9f, -19.19f, 5.2f));
    Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));
    HeldProductBox = nullptr;
    CurrentTargetShelf = nullptr;
    bIsInteracting = false;

}

void ASupermarketCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

 
}

void ASupermarketCharacter::BeginPlay()
{
    // Call the base class  
    Super::BeginPlay();

    //Add Input Mapping Context
    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }
}

//////////////////////////////////////////////////////////////////////////// Input

void ASupermarketCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    // Set up action bindings
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        // Jumping
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

        // Moving
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASupermarketCharacter::Move);

        // Looking
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASupermarketCharacter::Look);
        EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ASupermarketCharacter::OnInteract);
        EnhancedInputComponent->BindAction(DropAction, ETriggerEvent::Triggered, this, &ASupermarketCharacter::DropProductBox);
        EnhancedInputComponent->BindAction(StockAction, ETriggerEvent::Started, this, &ASupermarketCharacter::StartStocking);
        EnhancedInputComponent->BindAction(StockAction, ETriggerEvent::Completed, this, &ASupermarketCharacter::StopStocking);
    }
    else
    {
        UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
    }
}

void ASupermarketCharacter::StartStocking()
{
    if (HeldProductBox)  // Only start stocking if holding a product box
    {
        bIsStocking = true;
        GetWorldTimerManager().SetTimer(StockingTimerHandle, this, &ASupermarketCharacter::CheckShelfInView, 0.1f, true);
    }
}

void ASupermarketCharacter::StopStocking()
{
    bIsStocking = false;
    GetWorldTimerManager().ClearTimer(StockingTimerHandle);
    if (CurrentTargetShelf)
    {
        CurrentTargetShelf->StopStockingShelf();
        CurrentTargetShelf = nullptr;
    }
}

void ASupermarketCharacter::CheckShelfInView()
{
    if (!bIsStocking)
    {
        return;
    }

    FVector Start = FirstPersonCameraComponent->GetComponentLocation();
    FVector End = Start + FirstPersonCameraComponent->GetForwardVector() * 300.0f;

    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams))
    {
        AShelf* HitShelf = Cast<AShelf>(HitResult.GetActor());
        if (HitShelf)
        {
            if (HitShelf != CurrentTargetShelf)
            {
                if (CurrentTargetShelf)
                {
                    CurrentTargetShelf->StopStockingShelf();
                }
                CurrentTargetShelf = HitShelf;
                if (HeldProductBox)
                {
                    CurrentTargetShelf->StartStockingShelf(HeldProductBox->GetProductClass());
                }
            }
        }
        else
        {
            if (CurrentTargetShelf)
            {
                CurrentTargetShelf->StopStockingShelf();
                CurrentTargetShelf = nullptr;
            }
        }
    }
    else
    {
        if (CurrentTargetShelf)
        {
            CurrentTargetShelf->StopStockingShelf();
            CurrentTargetShelf = nullptr;
        }
    }
}

void ASupermarketCharacter::Move(const FInputActionValue& Value)
{
    // input is a Vector2D
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // add movement 
        AddMovementInput(GetActorForwardVector(), MovementVector.Y);
        AddMovementInput(GetActorRightVector(), MovementVector.X);
    }
}

void ASupermarketCharacter::Look(const FInputActionValue& Value)
{
    // input is a Vector2D
    FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // add yaw and pitch input to controller
        AddControllerYawInput(LookAxisVector.X);
        AddControllerPitchInput(LookAxisVector.Y);
    }
}


void ASupermarketCharacter::OnInteract()
{
    if (HeldProductBox)
    {
        // If already holding a box, do nothing (dropping is handled by 'G' key)
        return;
    }

    // Try to pick up a box
    FVector Start = FirstPersonCameraComponent->GetComponentLocation();
    FVector End = Start + FirstPersonCameraComponent->GetForwardVector() * 300.0f;

    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams))
    {
        if (AProductBox* ProductBox = Cast<AProductBox>(HitResult.GetActor()))
        {
            PickUpProductBox(ProductBox);
        }
    }
}

bool ASupermarketCharacter::GetHasRifle()
{
    return bHasRifle;
}

void ASupermarketCharacter::SetHasRifle(bool bNewHasRifle)
{
    bHasRifle = bNewHasRifle;
}

void ASupermarketCharacter::PickUpProductBox(AProductBox* ProductBox)
{
    if (ProductBox && !HeldProductBox)
    {
        HeldProductBox = ProductBox;

        // Disable physics simulation
        UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(HeldProductBox->GetRootComponent());
        if (PrimitiveComponent)
        {
            PrimitiveComponent->SetSimulatePhysics(false);
        }

        HeldProductBox->AttachToCamera(FirstPersonCameraComponent);
        bIsInteracting = true;
    }
}

void ASupermarketCharacter::DropProductBox()
{
    if (HeldProductBox)
    {
        HeldProductBox->DetachFromCamera();

        // Re-enable physics simulation
        UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(HeldProductBox->GetRootComponent());
        if (PrimitiveComponent)
        {
            PrimitiveComponent->SetSimulatePhysics(true);
        }

        HeldProductBox = nullptr;
        bIsInteracting = false;
    }
}



void ASupermarketCharacter::InteractWithShelf(AShelf* Shelf)
{
    if (Shelf && HeldProductBox)
    {
        TSubclassOf<AProduct> BoxProductClass = HeldProductBox->GetProductClass();
        TSubclassOf<AProduct> ShelfProductClass = Shelf->GetCurrentProductClass();

        if (!ShelfProductClass || ShelfProductClass == BoxProductClass)
        {
            // Start stocking the shelf
            Shelf->StartStockingShelf(BoxProductClass);

            // Remove a product from the box
            AProduct* RemovedProduct = HeldProductBox->RemoveProduct();
            if (RemovedProduct)
            {
                RemovedProduct->Destroy(); // Destroy the product as it's now on the shelf
            }

            // If the box is empty, destroy it
            if (HeldProductBox->GetProductCount() == 0)
            {
                HeldProductBox->Destroy();
                HeldProductBox = nullptr;
            }
        }
        else
        {
            // Clear the shelf and start stocking with the new product
            while (Shelf->RemoveNextProduct())
            {
                // Remove all products from the shelf
            }
            Shelf->StartStockingShelf(BoxProductClass);

            // Remove a product from the box
            AProduct* RemovedProduct = HeldProductBox->RemoveProduct();
            if (RemovedProduct)
            {
                RemovedProduct->Destroy(); // Destroy the product as it's now on the shelf
            }

            // If the box is empty, destroy it
            if (HeldProductBox->GetProductCount() == 0)
            {
                HeldProductBox->Destroy();
                HeldProductBox = nullptr;
            }
        }
    }
}
