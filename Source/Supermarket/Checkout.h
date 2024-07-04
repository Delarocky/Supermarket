// ACheckout.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Checkout.generated.h"

class USceneComponent;
class AAICustomerPawn;
class AProduct;
class USkeletalMeshComponent;
class UAnimMontage;
class UStaticMeshComponent;
class UTextRenderComponent;
class UAudioComponent;
class ACashierAI;
class UBoxComponent;

UCLASS()
class SUPERMARKET_API ACheckout : public AActor
{
    GENERATED_BODY()

public:
    ACheckout();
    UFUNCTION(BlueprintCallable)
    void ProcessCustomer(AAICustomerPawn* Customer);

    UFUNCTION(BlueprintCallable)
    void ScanItem(AProduct* Product);

    UFUNCTION(BlueprintCallable)
    void FinishTransaction();

    UFUNCTION(BlueprintCallable)
    void ScanNextItem();

    UFUNCTION(BlueprintCallable)
    bool TryEnterQueue(AAICustomerPawn* Customer);

    UFUNCTION(BlueprintCallable)
    void CustomerLeft(AAICustomerPawn* Customer);

    UFUNCTION(BlueprintCallable)
    bool ProcessPayment(float Amount);

    UFUNCTION(BlueprintCallable)
    void DisplayTotal(float Amount);

    UFUNCTION(BlueprintCallable)
    void ResetCheckout();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Checkout")
    USceneComponent* GridStartPoint;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Checkout")
    USceneComponent* ScanPoint;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Checkout")
    float ItemMoveSpeed = 300.0f; // Units per second
    UFUNCTION(BlueprintCallable)
    void SetCashier(ACashierAI* NewCashier);

    UFUNCTION(BlueprintCallable)
    void RemoveCashier();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Checkout")
    UBoxComponent* PlayerDetectionArea;
    UFUNCTION(BlueprintCallable, Category = "Checkout")
    FVector GetCashierPosition() const;
    UFUNCTION(BlueprintCallable, Category = "Checkout")
    bool HasCashier() const { return CurrentCashier != nullptr; }

    UFUNCTION(BlueprintCallable, Category = "Checkout")
    bool IsBeingServiced() const { return bPlayerPresent || HasCashier(); }
protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
    USkeletalMeshComponent* CheckoutMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Display")
    UStaticMeshComponent* DisplayMonitor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Display")
    UTextRenderComponent* TotalText;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio")
    UAudioComponent* PaymentSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    UAnimMontage* ScanItemAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    UAnimMontage* FinishTransactionAnimation;
    UPROPERTY(EditAnywhere, Category = "Queue")
    int32 MaxQueueSize = 10;
    UPROPERTY(EditAnywhere, Category = "Queue")
    TArray<USceneComponent*> QueuePositions;
private:
    UPROPERTY(EditAnywhere, Category = "Checkout")
    FVector CashierPositionOffset = FVector(0, -100, 0);  // Adjust this offset as needed
    UPROPERTY()
    TArray<AProduct*> ScannedItems;
    UPROPERTY(EditAnywhere, Category = "Queue", meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float RotationSpeed = 10.0f;
    TMap<AAICustomerPawn*, FRotator> CustomerTargetRotations;
    FTimerHandle RotationUpdateTimerHandle;
    void SetCustomerTargetRotation(AAICustomerPawn* Customer, int32 CustomerIndex);
    void StartRotationUpdate();
    void UpdateCustomerRotations();
    UPROPERTY()
    TArray<AProduct*> ProductsToScan;

    UPROPERTY()
    TArray<AProduct*> ItemsOnCounter;

    float TotalAmount;
    int32 CurrentItemIndex;
    bool bIsProcessingCustomer;

    UPROPERTY(EditAnywhere, Category = "Checkout")
    float ProcessingDistance = 100.0f;



    UPROPERTY()
    TArray<AAICustomerPawn*> CustomersInQueue;

 

    UPROPERTY(EditAnywhere, Category = "Display")
    FVector DisplayOffset = FVector(40,-82,100);

    UPROPERTY(EditAnywhere, Category = "Display")
    FVector TextOffset = FVector(104,80,40);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Checkout", meta = (AllowPrivateAccess = "true"))
    float TimeBetweenScans = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Checkout", meta = (AllowPrivateAccess = "true"))
    bool bDebugMode = true;

    UPROPERTY(EditAnywhere, Category = "Checkout", meta = (AllowPrivateAccess = "true"))
    FVector2D GridSize = FVector2D(4, 6);

    UPROPERTY(EditAnywhere, Category = "Checkout", meta = (AllowPrivateAccess = "true"))
    FVector2D GridSpacing = FVector2D(15.0f, 15.0f);

    UPROPERTY(EditAnywhere, Category = "Checkout", meta = (AllowPrivateAccess = "true"))
    FVector ItemStandingRotation = FVector(0.0f, 0.0f, 90.0f);

    FTimerHandle ScanItemTimerHandle;
    FTimerHandle UpdateQueueTimerHandle;
    FTimerHandle MoveItemTimerHandle;

    void SetupUpdateQueueTimer();
    void UpdateQueue();
    void PlaceItemsOnCounter();
    void MoveNextItemToScanPosition();
    void RemoveScannedItem();
    void DebugLogQueueState();
    void DebugLogScanState();
    void DebugLog(const FString& Message);

    UFUNCTION()
    void UpdateItemPosition(AProduct* Item, FVector StartLocation, FVector EndLocation, float ElapsedTime, float Duration);

    UPROPERTY()
    ACashierAI* CurrentCashier;

    UFUNCTION()
    void OnPlayerEnterDetectionArea(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnPlayerExitDetectionArea(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    bool bPlayerPresent;
    bool CanProcessCustomers() const;
};