// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "ProductBox.h"
#include "Shelf.h"
#include "SupermarketCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config = Game)
class ASupermarketCharacter : public ACharacter
{
    GENERATED_BODY()

    /** MappingContext */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputMappingContext* DefaultMappingContext;

    /** Interact Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* InteractAction;

    /** Stop Interact Input Action */


    /** Pawn mesh: 1st person view (arms; seen only by self) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
    USkeletalMeshComponent* Mesh1P;

    /** First person camera */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
    UCameraComponent* FirstPersonCameraComponent;

    /** Jump Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputAction* JumpAction;

    /** Move Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputAction* MoveAction;

public:
    ASupermarketCharacter();

protected:
    virtual void BeginPlay();

public:
    /** Look Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* LookAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* DropAction;


    /** Bool for AnimBP to switch to another animation set */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon)
    bool bHasRifle;

    /** Setter to set the bool */
    UFUNCTION(BlueprintCallable, Category = Weapon)
    void SetHasRifle(bool bNewHasRifle);

    /** Getter for the bool */
    UFUNCTION(BlueprintCallable, Category = Weapon)
    bool GetHasRifle();
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void StartStocking();

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void StopStocking();
protected:
    /** Called for movement input */
    void Move(const FInputActionValue& Value);

    /** Called for looking input */
    void Look(const FInputActionValue& Value);

    /** Called for interaction input */
    void OnInteract();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* StockAction;


protected:
    // APawn interface
    virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
    // End of APawn interface
    virtual void Tick(float DeltaTime) override;
public:
    /** Returns Mesh1P subobject **/
    USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
    /** Returns FirstPersonCameraComponent subobject **/
    UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

private:
    UPROPERTY()
    AShelf* CurrentTargetShelf;
    bool bIsStocking;

    void CheckShelfInView();

    FTimerHandle StockingTimerHandle;
    UPROPERTY()
    AProductBox* HeldProductBox;
    void PopulateShelves();
    bool bIsInteracting;
    void InteractWithShelf(AShelf* Shelf);
    void DropProductBox();
    void PickUpProductBox(AProductBox* ProductBox);
};