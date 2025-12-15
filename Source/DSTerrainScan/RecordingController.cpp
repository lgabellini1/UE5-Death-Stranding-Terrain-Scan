#include "RecordingController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "RuntimeVideoRecorder.h"
#include "ScannerCharacter.h"
#include "GameFramework/DefaultPawn.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"

void ARecordingController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->ClearAllMappings();
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
}

void ARecordingController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// Toggle pawns
		EnhancedInputComponent->BindAction(TogglePawnAction, ETriggerEvent::Started, this, &ARecordingController::TogglePawn);

		// Recording
		EnhancedInputComponent->BindAction(RecordAction, ETriggerEvent::Started, this, &ARecordingController::StartStopRecording);
	}
}

void ARecordingController::TogglePawn()
{
	APawn* CurrentPawn = GetPawn();
	
	AGameModeBase* GameMode = UGameplayStatics::GetGameMode(CurrentPawn);
	if (!GameMode) return;

	auto CharacterPawnClass = GameMode->DefaultPawnClass;

	if (CurrentPawn->IsA(CharacterPawnClass))
	{
		CharacterPawn = CurrentPawn;
		
		if (!FreeflyPawn)
		{
			FreeflyPawn = GetWorld()->SpawnActor<ADefaultPawn>();
			FreeflyPawn->SetActorTransform(CharacterPawn->GetActorTransform());
		}

		Possess(FreeflyPawn);
		UE_LOG(LogTemp, Display, TEXT("Possessing freefly."));
	}
	else
	{
		Possess(CharacterPawn);
		UE_LOG(LogTemp, Display, TEXT("Possessing character."));
	}
}

void ARecordingController::StartStopRecording()
{
	URuntimeVideoRecorder* RuntimeVideoRecorder = GEngine->GetEngineSubsystem<URuntimeVideoRecorder>();
	
	if (!RuntimeVideoRecorder) return;

	const wchar_t* DebugMsg;
	
	if (RuntimeVideoRecorder->IsRecordingInProgress())
	{
		RuntimeVideoRecorder->StopRecording_NativeAPI();
		
		DebugMsg = TEXT("Recording stopped!");
	}
	else
	{
		FRuntimeEncoderSettings Settings;
		Settings.VideoBitrate = Bitrate;
		Settings.Profile = ERuntimeEncoderProfile::Profile_High;
		Settings.TargetQuality = EncodingQuality;
		
		int32 ActualWidth = bUseViewportDimensions ? -1 : Width;
		int32 ActualHeight = bUseViewportDimensions ? -1 : Height;
		
		RuntimeVideoRecorder->StartRecording(TEXT("%auto%"), TargetFPS, ActualWidth, ActualHeight, Settings);
		
		DebugMsg = TEXT("Recording started!");
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Cyan, DebugMsg);
	}
}
