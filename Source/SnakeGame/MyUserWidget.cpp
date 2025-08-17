#include "MyUserWidget.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h" 

void UMyUserWidget::SetScore(int32 InScore)
{
	if (!IsValid(this) || !ScoreText)
	{
		UE_LOG(LogTemp, Warning, TEXT("UMyUserWidget::SetScore called but ScoreText is invalid."));
		return;
	}
	ScoreText->SetText(FText::AsNumber(InScore));
}

void UMyUserWidget::SetLevel(int32 InLevel)
{
	if (!IsValid(this) || !LevelText)
	{
		UE_LOG(LogTemp, Warning, TEXT("UMyUserWidget::SetLevel called but LevelText is invalid."));
		return;
	}
	LevelText->SetText(FText::AsNumber(InLevel));
}

void UMyUserWidget::SetPlayerScores(int32 InP1Score, int32 InP2Score)
{
	if (!IsValid(this))
	{
		UE_LOG(LogTemp, Warning, TEXT("UMyUserWidget::SetPlayerScores called but widget is invalid."));
		return;
	}

	if (ScoreP1Text)
	{
		ScoreP1Text->SetText(
			FText::Format(NSLOCTEXT("UI","P1Score","P1: {0}"),
						  FText::AsNumber(InP1Score))
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UMyUserWidget::SetPlayerScores: ScoreP1Text is null!"));
	}

	if (ScoreP2Text)
	{
		ScoreP2Text->SetText(
			FText::Format(NSLOCTEXT("UI","P2Score","P2: {0}"),
						  FText::AsNumber(InP2Score))
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UMyUserWidget::SetPlayerScores: ScoreP2Text is null!"));
	}
}