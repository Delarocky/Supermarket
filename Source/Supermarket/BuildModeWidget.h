#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BuildModeWidget.generated.h"

UCLASS()
class SUPERMARKET_API UBuildModeWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* KeybindsText;

protected:
    virtual void NativeConstruct() override;
};