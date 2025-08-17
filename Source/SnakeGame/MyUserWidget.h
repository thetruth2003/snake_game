#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "MyUserWidget.generated.h"

UCLASS()
class SNAKEGAME_API UMyUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta=(BindWidget))
	UTextBlock* ScoreText;
	
	UPROPERTY(meta=(BindWidget))
	UTextBlock* LevelText;
	
	UPROPERTY(meta=(BindWidget, OptionalWidget))
	UTextBlock* ScoreP1Text;
	
	UPROPERTY(meta=(BindWidget, OptionalWidget))
	UTextBlock* ScoreP2Text;
	
	UFUNCTION(BlueprintCallable, Category="UI")
	void SetScore(int32 InScore);

	UFUNCTION(BlueprintCallable, Category="UI")
	void SetLevel(int32 InLevel);

	UFUNCTION(BlueprintCallable, Category="UI")
	void SetPlayerScores(int32 InP1Score, int32 InP2Score);
};