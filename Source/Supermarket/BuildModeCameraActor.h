#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraComponent.h"
#include "BuildModeCameraActor.generated.h"

UCLASS()
class SUPERMARKET_API ABuildModeCameraActor : public AActor
{
    GENERATED_BODY()

public:
    ABuildModeCameraActor();

    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
    UCameraComponent* CameraComponent;

    UFUNCTION(BlueprintCallable, Category = "Build Mode")
    void MoveCamera(const FVector2D& InputVector);

    UFUNCTION(BlueprintCallable, Category = "Build Mode")
    void RotateCamera(const FVector2D& InputVector);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build Mode")
    float MoveSpeed = 1000.0f;

    
    float RotateSpeed = 10.0f;

    USceneComponent* PivotPoint;
    
    float CameraDistance = 1000.0f;
   
    float CameraHeight = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build Mode")
    float CameraPitch = -45.0f;
private:
    bool bIsRotating;
    void UpdateCameraPosition();
};