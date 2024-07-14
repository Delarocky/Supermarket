#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "SceneBoxComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SUPERMARKET_API USceneBoxComponent : public UBoxComponent
{
    GENERATED_BODY()

public:
    USceneBoxComponent();

    // Function to check if this scene box overlaps with any other scene boxes
    bool CheckOverlap() const;
};