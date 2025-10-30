#include "FootprintControllerComponent.h"
#include "Engine/World.h"
#include "ScannerControllerComponent.h"
#include "ScannerIconsControllerComponent.h"
#include "Components/DecalComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialParameterCollectionInstance.h"

UFootprintControllerComponent::UFootprintControllerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.SetTickFunctionEnable(true);
}

void UFootprintControllerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!DecalMaterial || !MPC) return;
	
	if (UMaterialParameterCollectionInstance* MPCI = GetWorld()->GetParameterCollectionInstance(MPC))
	{
		MPCI->SetScalarParameterValue(TEXT("_Footprint_Highlight_Fade_Time"), HighlightFadeTime);
	}
	
	// Setup the DMIs
	LeftFootstep = UMaterialInstanceDynamic::Create(DecalMaterial, this);
	LeftFootstep->SetScalarParameterValue(TEXT("IsRight"), 0.0f);
	LeftFootstep->SetScalarParameterValue(TEXT("IsHighlighted"), 0.0f);

	LeftFootstepHighlight = UMaterialInstanceDynamic::Create(DecalMaterial, this);
	LeftFootstepHighlight->SetScalarParameterValue(TEXT("IsRight"), 0.0f);
	LeftFootstepHighlight->SetScalarParameterValue(TEXT("IsHighlighted"), 1.0f);
	
	RightFootstep = UMaterialInstanceDynamic::Create(DecalMaterial, this);
	RightFootstep->SetScalarParameterValue(TEXT("IsRight"), 1.0f);
	RightFootstep->SetScalarParameterValue(TEXT("IsHighlighted"), 0.0f);

	RightFootstepHighlight = UMaterialInstanceDynamic::Create(DecalMaterial, this);
	RightFootstepHighlight->SetScalarParameterValue(TEXT("IsRight"), 1.0f);
	RightFootstepHighlight->SetScalarParameterValue(TEXT("IsHighlighted"), 1.0f);

	// Footprints do not show up on the player character's mesh
	if (auto* PlayerCharacter = GetOwner<ACharacter>())
	{
		PlayerCharacter->GetMesh()->SetReceivesDecals(false);
	}
}

void UFootprintControllerComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	for (FFootprintData& Footprint : Footprints)
	{
		Footprint.Age += DeltaTime;
	}

	auto* PlayerCharacter = GetOwner<ACharacter>();
	auto* Scanner = PlayerCharacter->GetComponentByClass<UScannerControllerComponent>();
	auto* Icons = PlayerCharacter->GetComponentByClass<UScannerIconsControllerComponent>();
	
	float CurrentTime = GetWorld()->GetTimeSeconds();
	float ElapsedTime = CurrentTime - Scanner->GetCurrentFrameScannerState().StartTime;
	float RelativeHighlightTime = ElapsedTime - Icons->TotalEffectDuration();
	
	UMaterialParameterCollectionInstance* MPCI = GetWorld()->GetParameterCollectionInstance(MPC);
	MPCI->SetScalarParameterValue(TEXT("_Footprint_Relative_Highlight_Time"),
		FMath::Max(0.f, RelativeHighlightTime));

	// Clean array from dead footprints
	Footprints.RemoveAll([this](const FFootprintData& Footprint)
	{
		return Footprint.Age >= GetFootprintLifetime(Footprint);
	});
}

void UFootprintControllerComponent::HandleFootstep(EFootstepType FootstepType)
{
	TOptional<FFootprintData> FootprintFound = CheckFootstepCollision(FootstepType);
	
	if (FootprintFound.IsSet())
	{
		FFootprintData Footprint = FootprintFound.GetValue();
		
		auto* PlayerCharacter = GetOwner<ACharacter>();
		auto* Scanner = PlayerCharacter->GetComponentByClass<UScannerControllerComponent>();
		auto* Icons = PlayerCharacter->GetComponentByClass<UScannerIconsControllerComponent>();
		
		if (Icons->IsEffectActive() && Scanner->IsPointInsideScanArea(Footprint.Location))
		{
			Footprint.IsHighlight = true;
		}

		UMaterialInstanceDynamic* FootprintDMI = GetFootprintMaterial(Footprint);

		UDecalComponent* FootprintDecal = UGameplayStatics::SpawnDecalAtLocation(GetWorld(),
			FootprintDMI, DecalSize, Footprint.Location, Footprint.Rotation);
		
		FootprintDecal->SetFadeOut(GetFootprintLifetime(Footprint) - FadeTime, FadeTime);
		
		Footprint.Decal = FootprintDecal;

		// Save footprint in the controller's memory
		Footprints.Add(Footprint);
	}
}

void UFootprintControllerComponent::StartFootprintsLifecycle()
{
	auto* Scanner = GetOwner()->GetComponentByClass<UScannerControllerComponent>();
	
	for (FFootprintData& Footprint : Footprints)
	{
		bool bWasHighlighted = Footprint.IsHighlight;
		Footprint.IsHighlight = Scanner->IsPointInsideScanArea(Footprint.Location);
		
		// Case 1: Footprint highlighted by a previous scan, highlighted again. 
		if (bWasHighlighted && Footprint.IsHighlight)
		{
			Footprint.Age = 0.f;
		}
		// Case 2: Footprint highlighted by a previous scan, now outside. 
		// Case 3: Regular footprint now highlighted. 
		else if (bWasHighlighted || Footprint.IsHighlight)
		{
			Footprint.Age = 0.f;
			if (Footprint.Decal) Footprint.Decal->SetDecalMaterial(GetFootprintMaterial(Footprint));
		}
		// Case 4: Regular footprint still outside scan, do nothing. 
	}
}

TOptional<FFootprintData> UFootprintControllerComponent::CheckFootstepCollision(EFootstepType FootstepType) const
{
	auto* PlayerCharacter = GetOwner<ACharacter>();
	if (!PlayerCharacter) return TOptional<FFootprintData>();

	FHitResult RaycastResult;
	
	FName BoneSocketName = FootstepType == EFootstepType::Left ? FName{"foot_l_Socket"} : FName{"foot_r_Socket"};
	FVector TraceStart = PlayerCharacter->GetMesh()->GetSocketLocation(BoneSocketName) + FVector{0.0f, 0.0f, 20.0f};
	FVector TraceEnd = TraceStart - FVector{0.0f, 0.0f, 50.0f};

	// If raycast hits...
	if (GetWorld()->LineTraceSingleByChannel(RaycastResult, TraceStart, TraceEnd, ECC_Visibility))
	{
		FVector FootstepLocation = RaycastResult.Location;
		
		FVector HitNormal = RaycastResult.Normal;
		FVector PlayerForwardVector = PlayerCharacter->GetActorForwardVector();

		// Orient the decal correctly along the terrain.
		FRotator FootstepRotation = FRotationMatrix::MakeFromXZ(
			PlayerForwardVector.GetSafeNormal(), HitNormal.GetSafeNormal()).Rotator();
		FootstepRotation.Pitch -= +90.0f;
		FootstepRotation.Yaw += 90.0f;
		
		return FFootprintData{FootstepLocation, FootstepRotation, FootstepType};
	}
	else
	{
		return TOptional<FFootprintData>();
	}
}

constexpr UMaterialInstanceDynamic* UFootprintControllerComponent::GetFootprintMaterial(const FFootprintData& Footprint) const
{
	switch (Footprint.Type)
	{
		case EFootstepType::Left:
			return Footprint.IsHighlight ? LeftFootstepHighlight : LeftFootstep;
		case EFootstepType::Right:
			return Footprint.IsHighlight ? RightFootstepHighlight : RightFootstep;
		default:
			return nullptr;
	}
}

float UFootprintControllerComponent::GetFootprintLifetime(const FFootprintData& Footprint) const
{
	auto* Icons = GetOwner()->GetComponentByClass<UScannerIconsControllerComponent>();
	
	return Footprint.IsHighlight
		? Icons->TotalEffectDuration() + HighlightFadeTime + FadeTime
		: RegularFootprintLifetime + FadeTime;
}


void DEBUG_PrintTArray(const TArray<FFootprintData>& Footprints)
{
	FString Output = TEXT("[");

	for (int32 i = 0; i < Footprints.Num(); ++i)
	{
		const FFootprintData& FP = Footprints[i];

		Output += FString::Printf(TEXT("{%d%s}"), 
			i, 
			FP.IsHighlight ? TEXT(",h") : TEXT(""));

		if (i < Footprints.Num() - 1)
		{
			Output += TEXT(", ");
		}
	}

	Output += TEXT("]");

	UE_LOG(LogTemp, Warning, TEXT("Footprints: %s"), *Output);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, Output);
	}
}
