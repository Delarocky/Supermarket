// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "ProductBox.h"
#include "Shelf.h"
#include "Blueprint/UserWidget.h"  // Add this include
#include "SupermarketCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UWidgetComponent;
class UUserWidget;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config = Game)
class ASupermarketCharacter : public ACharacter
{
    GENERATED_BODY()

    /** Tablet Screen Widget Component */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Tablet, meta = (AllowPrivateAccess = "true"))
    UWidgetComponent* TabletScreenWidget;

    /** Tablet Widget Class */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Tablet, meta = (AllowPrivateAccess = "true"))
    TSubclassOf<UUserWidget> TabletWidgetClass;

    /** Tablet Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* TabletAction;

    /** Tablet Mesh */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
    UStaticMeshComponent* TabletMesh;

    /** Tablet Widget */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI, meta = (AllowPrivateAccess = "true"))
    class UUserWidget* TabletWidget;


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

    /** Tablet view camera */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
    UCameraComponent* TabletCameraComponent;

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
    void Look(const FInputActionValue& Value);

    /** Tablet mode bool */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Tablet)
    bool bIsTabletMode;

    /** Toggle Tablet mode */
    UFUNCTION(BlueprintCallable, Category = Tablet)
    void ToggleTabletMode();


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
    UFUNCTION(BlueprintCallable, Category = "Tablet")
    void SetTabletScreenContent(TSubclassOf<UUserWidget> NewWidgetClass);

    /** Get the current tablet widget */
    UFUNCTION(BlueprintCallable, Category = "Tablet")
    UUserWidget* GetTabletScreenWidget() const;

protected:
    /** Called for movement input */
    void Move(const FInputActionValue& Value);
    void SetCameraRotationEnabled(bool bEnable);


    /** Called for interaction input */
    void OnInteract();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* StockAction;

    /** Tablet Offset from Hand */
  
    FVector TabletOffset;

    /** Tablet Rotation */
   
    FRotator TabletRotation;

    /** Tablet Screen Offset */
   
    FVector TabletScreenOffset;

    /** Tablet Screen Rotation */
 
    FRotator TabletScreenRotation;

protected:
    // APawn interface
    virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
    // End of APawn interface
    virtual void Tick(float DeltaTime) override;
    /** Called for tablet input */
    void OnTabletAction();

    /** Set up tablet mode */
    void SetupTabletMode(bool bEnable);

    /** Tablet camera settings */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tablet)
    FRotator TabletCameraRotation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tablet)
    float TabletCameraFOV;
    /** Start camera transition */
    void StartCameraTransition(bool bToTabletView);
public:
    UFUNCTION(BlueprintCallable, Category = "Tablet")
    void UpdateTabletTransform();
    /** Returns Mesh1P subobject **/
    USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
    /** Returns FirstPersonCameraComponent subobject **/
    UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float CameraTransitionTime;
private:
    /** Timer handle for camera transition */
    FTimerHandle CameraTransitionTimerHandle;
    UPROPERTY()
    AShelf* CurrentTargetShelf;
    bool bIsStocking;
    FRotator OriginalControllerRotation;
    void CheckShelfInView();
    bool bCameraRotationEnabled;
    FTimerHandle StockingTimerHandle;
    UPROPERTY()
    AProductBox* HeldProductBox;
    bool bIsInteracting;
    void InteractWithShelf(AShelf* Shelf);
    void DropProductBox();
    void PickUpProductBox(AProductBox* ProductBox);
    FRotator OriginalCameraRotation;
    float OriginalCameraFOV;
    /** Switch between first person and tablet cameras */
    void SwitchCamera(bool bUseTabletCamera);
    /** Update camera transition */
    void UpdateCameraTransition();
    UPROPERTY()
    FRotator InitialRotation;
    /** Camera transition properties */
    float CameraTransitionDuration;
    float CameraTransitionElapsedTime;
    bool bIsCameraTransitioning;
    FVector StartCameraLocation;
    FRotator StartCameraRotation;
    float StartCameraFOV;
    FVector TargetCameraLocation;
    FRotator TargetCameraRotation;
    float TargetCameraFOV;
    void SetupTabletScreen();
};