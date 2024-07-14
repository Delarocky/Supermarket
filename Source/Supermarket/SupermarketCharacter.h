// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "ProductBox.h"
#include "Shelf.h"
#include "Blueprint/UserWidget.h"  // Add this include
#include "MoneyDisplayWidget.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/PostProcessVolume.h"
#include "BuildModeCameraActor.h"
#include "StoreManager.h"
#include "StoreStatusWidget.h"
#include "ParkingSpace.h"
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

    

    // Add this function declaration if you want to be able to update the transform at runtime
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void UpdateProductBoxTransform();

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
  
    // Add these properties
    UPROPERTY(EditAnywhere, Category = "Movement")
    float MoveObjectHoldTime;

    FTransform LastBuildModeCameraTransform;
public:
    ASupermarketCharacter();

    FVector ClampLocationToBuildingArea(const FVector& Location);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build Mode")
    UBoxComponent* BuildingArea;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build Mode")
    APostProcessVolume* HighlightVolume;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build Mode")
    UMaterialInterface* HighlightMaterial;

    UFUNCTION(BlueprintCallable, Category = "Build Mode")
    void ToggleObjectMovement();

    UFUNCTION(BlueprintCallable, Category = "Build Mode")
    void RotateObjectLeft();

    UFUNCTION(BlueprintCallable, Category = "Build Mode")
    void RotateObjectRight();
   
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Build Mode")
    bool bIsBuildModeActive;

    UPROPERTY(EditDefaultsOnly, Category = "Build Mode")
    TSubclassOf<ABuildModeCameraActor> BuildModeCameraClass;

    UPROPERTY(EditDefaultsOnly, Category = "Build Mode")
    TSubclassOf<UUserWidget> BuildModeWidgetClass;

    UFUNCTION(BlueprintCallable, Category = "Build Mode")
    void ToggleBuildMode();

    UFUNCTION(BlueprintCallable, Category = "Build Mode")
    void MoveInBuildMode(const FInputActionValue& Value);

    UFUNCTION(BlueprintCallable, Category = "Build Mode")
    void RotateInBuildMode(const FInputActionValue& Value);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    class UInputMappingContext* SupermarketMappingContext;
protected:
    virtual void BeginPlay();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build Mode")
    float RotationAngle;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Build Mode")
    AActor* SelectedObject;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Build Mode")
    bool bIsMovingObject;

    UFUNCTION()
    void OnLeftMouseButtonPressed();

    UFUNCTION()
    void OnLeftMouseButtonReleased();

    UFUNCTION()
    void MoveSelectedObject();

    UFUNCTION()
    void HighlightHoveredObject(AActor* HoveredActor);

public:

    UFUNCTION(BlueprintCallable, Category = "Build Mode")
    void SetBuildingAreaSize(const FVector& NewSize);

    UFUNCTION(BlueprintCallable, Category = "Build Mode")
    void SetBuildingAreaLocation(const FVector& NewLocation);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    FVector ProductBoxOffset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    FRotator ProductBoxRotation;

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
    UFUNCTION(BlueprintImplementableEvent, Category = "Tablet")
    void OnTabletClicked(const FVector2D& ClickPosition);

protected:


    /** Called for movement input */
    void Move(const FInputActionValue& Value);
    void SetCameraRotationEnabled(bool bEnable);

    /** Called for tablet click input */
    void OnTabletClickInput();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* TabletClickAction;

    UPROPERTY(BlueprintReadWrite, Category = "Tablet")
    bool bTabletViewage;
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
    UPROPERTY()
    UMaterialInterface* OriginalMaterial;

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
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    class UInputAction* MoveObjectAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    class UInputAction* RotateLeftAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    class UInputAction* RotateRightAction;
    /** Store Status Toggle Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* ToggleStoreStatusAction;

    /** Toggle Store Status */
    void ToggleStoreStatusInput(const FInputActionValue& Value);
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    TSubclassOf<UStoreStatusWidget> StoreStatusWidgetClass;
public:
    UFUNCTION(BlueprintCallable, Category = "Tablet")
    void UpdateTabletTransform();
    /** Returns Mesh1P subobject **/
   
    /** Returns FirstPersonCameraComponent subobject **/
    UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float CameraTransitionTime;
    UPROPERTY()
    UMoneyDisplayWidget* MoneyDisplayWidget;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputMappingContext* BuildModeMappingContext;
    UFUNCTION(BlueprintCallable, Category = "Build Mode")
    bool IsInBuildMode() const { return bIsBuildModeActive; }
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build Mode")
    float BuildModeRotationSpeed = 0.1f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build Mode")
    float BuildModeMoveSpeed = 1000.0f;  // You can adjust this default value as needed
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Build Mode")
    bool bIsRightMouseButtonPressed;

    UFUNCTION(BlueprintCallable, Category = "Build Mode")
    void OnRightMouseButtonPressed();

    UFUNCTION(BlueprintCallable, Category = "Build Mode")
    void OnRightMouseButtonReleased();
    UFUNCTION(BlueprintCallable, Category = "Build Mode")
    void StartObjectMovement();

    UFUNCTION(BlueprintCallable, Category = "Build Mode")
    void StopObjectMovement();

    UFUNCTION(BlueprintCallable, Category = "Build Mode")
    void UpdateObjectPosition();
    UFUNCTION(BlueprintCallable, Category = "Build Mode")
    bool CanObjectBeMoved(AActor* Actor);
    UFUNCTION(BlueprintCallable, Category = "Store Management")
    void ToggleStoreStatus();
    UFUNCTION(BlueprintCallable, Category = "UI")
    void CreateAndShowStoreStatusWidget();
    FCriticalSection StoreStatusLock;
    void SetInitialSpawnLocation(const FVector& Location) { InitialSpawnLocation = Location; }
    void SetAssignedParkingSpace(AParkingSpace* ParkingSpace) { AssignedParkingSpace = ParkingSpace; }
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    bool IsHoldingProductBox(AProductBox* Box) const;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build Mode")
    UMaterialInterface* ValidPlacementMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build Mode")
    UMaterialInterface* InvalidPlacementMaterial;
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
    void CreateMoneyDisplayWidget();
    FTimerHandle MoveObjectTimerHandle;
    
    FVector ObjectOriginalLocation;
    FRotator ObjectOriginalRotation;
    UPROPERTY(EditAnywhere, Category = "Movement")
    float MaxMoveDistance;
    
    bool bIsHoldingObject;
    FVector InitialGrabOffset;
    UPROPERTY()
    ABuildModeCameraActor* BuildModeCamera;
    UPROPERTY()

    UUserWidget* BuildModeWidget;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))

    class UInputAction* BuildModeAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* BuildModeRotateAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* RightMouseButtonPressedAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* RightMouseButtonReleasedAction;
    void SetupBuildModeInputs();
    void UpdateMovementState();
    void SetupInputMappingContexts();

    FVector GetMouseWorldPosition();
    AActor* GetActorUnderCursor();
    bool IsActorInBuildingArea(AActor* Actor);
    FVector2D InitialMouseOffset;
    FVector2D InitialMousePosition;
    FVector InitialObjectPosition;
    FPlane MovementPlane;
    FVector2D LastMousePosition;
    FVector CameraRight;
    FVector CameraForward;
    FVector InitialHitPoint;
    FVector ClickOffset;
    FTransform InitialObjectTransform;
    UPROPERTY()
    AStoreManager* StoreManager;
    FVector InitialSpawnLocation;
    AParkingSpace* AssignedParkingSpace;
    UPROPERTY()
    UStoreStatusWidget* StoreStatusWidget;

    FVector OriginalObjectPosition;
    FRotator OriginalObjectRotation;
    bool bIsValidPlacement;

    void UpdateObjectPlacement();
    void ResetObjectToOriginalPosition();
    void ApplyMaterialToActor(AActor* Actor, UMaterialInterface* Material);

};