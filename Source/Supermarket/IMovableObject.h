// IMovableObject.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IMovableObject.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UMovableObject : public UInterface
{
    GENERATED_BODY()
};

class SUPERMARKET_API IMovableObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Movable Object")
    void StartMoving();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Movable Object")
    void StopMoving();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Movable Object")
    void RotateLeft();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Movable Object")
    void RotateRight();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Movable Object")
    bool IsValidPlacement() const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Movable Object")
    void UpdateOutline(bool bIsValidPlacement);
};