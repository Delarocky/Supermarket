// StoreStatusWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StoreStatusWidget.generated.h"

class AStoreManager;

UCLASS()
class SUPERMARKET_API UStoreStatusWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Store Status")
    void SetStoreOpen(bool bIsOpen);

    UFUNCTION(BlueprintCallable, Category = "Store Status")
    bool IsStoreOpen();

    UFUNCTION(BlueprintCallable, Category = "Store Status")
    void ToggleStoreStatus();

protected:
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* StatusText;

    

    virtual void NativeConstruct() override;

private:
    UFUNCTION()
    void OnToggleButtonClicked();

    UPROPERTY()
    AStoreManager* StoreManager;

    UFUNCTION()
    void InitializeWidget();
};