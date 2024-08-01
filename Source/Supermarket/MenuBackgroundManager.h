// MenuBackgroundManager.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"
#include "MenuBackgroundManager.generated.h"

USTRUCT(BlueprintType)
struct FCameraPosition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Location;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRotator Rotation;

    FCameraPosition() : Location(FVector::ZeroVector), Rotation(FRotator::ZeroRotator) {}
    FCameraPosition(FVector InLocation, FRotator InRotation) : Location(InLocation), Rotation(InRotation) {}
};

UCLASS()
class SUPERMARKET_API AMenuBackgroundManager : public APawn
{
    GENERATED_BODY()

public:
    AMenuBackgroundManager();

    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    UFUNCTION(BlueprintCallable, Category = "Menu Background")
    void AddCameraPosition(const FVector& Location, const FRotator& Rotation);

    UFUNCTION(BlueprintCallable, Category = "Menu Background")
    void StartCycling();

    UFUNCTION(BlueprintCallable, Category = "Menu Background")
    void StopCycling();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu Background")
    float DisplayDuration = 5.0f;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere)
    USceneComponent* RootSceneComponent;

    UPROPERTY(VisibleAnywhere)
    UCameraComponent* CameraComponent;

    TArray<FCameraPosition> CameraPositions;

    int32 CurrentPositionIndex;
    float TimeUntilNextSwitch;

    bool bIsCycling;

    void SwitchToNextPosition();
};