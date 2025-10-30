#include "ScannerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "ScannerControllerComponent.h"
#include "ScannerIconsControllerComponent.h"
#include "FootprintControllerComponent.h"
#include "DrawDebugHelpers.h"

AScannerCharacter::AScannerCharacter()
{
	// Sets this actor to tick every frame.
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Character moves in direction of input...
	GetCharacterMovement()->bOrientRotationToMovement = true;
	// ...at this rotation rate.
	GetCharacterMovement()->RotationRate = FRotator{0.0f, 500.0f, 0.0f};

	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // Camera-character distance	
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); 
	FollowCamera->bUsePawnControlRotation = false;

	ScannerController = CreateDefaultSubobject<UScannerControllerComponent>(TEXT("ScannerController"));

	ScannerIconsController = CreateDefaultSubobject<UScannerIconsControllerComponent>(TEXT("IconsController"));

	FootprintController = CreateDefaultSubobject<UFootprintControllerComponent>(TEXT("FootprintController"));

	/* Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	are set in the derived blueprint asset named ScannerCharacter (to avoid direct content references in C++). */
}

void AScannerCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AScannerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AScannerCharacter::OnFootStep(EFootstepType FootstepType)
{
	FootprintController->HandleFootstep(FootstepType);
}

void AScannerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AScannerCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AScannerCharacter::Look);

		// Scanning
		EnhancedInputComponent->BindAction(ScanAction, ETriggerEvent::Triggered, this, &AScannerCharacter::Scan);
	}
}

void AScannerCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem
			= ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AScannerCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller)
	{
		FRotator Rotation = Controller->GetControlRotation();
		FRotator YawRotation{0, Rotation.Yaw, 0};
		
		FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AScannerCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AScannerCharacter::Scan(const FInputActionValue&)
{
	if (IsValid(ScannerController) && IsValid(ScannerIconsController))
	{
		if (ScannerController->GetCurrentFrameScannerState().AnimationState != EScannerAnimationState::Inactive)
		{
			return;
		}
		
		ScannerController->StartScannerLifecycle();
		ScannerIconsController->StartIconsLifecycle();
		FootprintController->StartFootprintsLifecycle();
	}
}
