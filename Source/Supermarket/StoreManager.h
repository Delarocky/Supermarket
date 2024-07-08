// StoreManager.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HAL/CriticalSection.h"
#include "StoreManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStoreStatusChanged, bool, bIsOpen);

UCLASS()
class SUPERMARKET_API AStoreManager : public AActor
{
    GENERATED_BODY()

public:
    AStoreManager();

    UFUNCTION(BlueprintCallable, Category = "Store Management")
    void SetStoreOpen(bool bIsOpen);

    UFUNCTION(BlueprintCallable, Category = "Store Management")
    bool IsStoreOpen() const { return bStoreOpen; }

    UPROPERTY(BlueprintAssignable, Category = "Store Management")
    FOnStoreStatusChanged OnStoreStatusChanged;

    UFUNCTION(BlueprintCallable, Category = "Store Management")
    static AStoreManager* GetInstance(UWorld* World);

private:
    UPROPERTY(VisibleAnywhere, Category = "Store Management")
    bool bStoreOpen;
    mutable FCriticalSection StatusLock;
    static AStoreManager* Instance;
};