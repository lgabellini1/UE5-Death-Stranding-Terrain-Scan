#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RecordingController.generated.h"

class UInputMappingContext;
class UInputAction;

/**
 *  Simple controller class with recording support. Allows switching
 *  between the player and a free camera for recording.
 */
UCLASS()
class DSTERRAINSCAN_API ARecordingController : public APlayerController
{
	GENERATED_BODY()

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> TogglePawnAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> RecordAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> ScanAction;

	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recording", meta = (AllowPrivateAccess = "true"))
	int32 TargetFPS = 60;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recording", meta = (AllowPrivateAccess = "true",
		EditCondition="!bUseViewportDimensions"))
	int32 Width = 1920;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recording", meta = (AllowPrivateAccess = "true",
		EditCondition="!bUseViewportDimensions"))
	int32 Height = 1080;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recording", meta = (AllowPrivateAccess = "true"))
	bool bUseViewportDimensions = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recording", meta = (AllowPrivateAccess = "true"))
	int32 Bitrate = 20000000;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recording", meta = (AllowPrivateAccess = "true"))
	int32 EncodingQuality = 75;

protected:
	virtual void SetupInputComponent() override;

	virtual void OnPossess(APawn* InPawn) override;

public:
	void TogglePawn();

	void StartStopRecording();

private:
	UPROPERTY()
	APawn* CharacterPawn;

	UPROPERTY()
	APawn* FreeflyPawn;
};
