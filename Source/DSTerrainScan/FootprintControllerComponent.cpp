#include "FootprintControllerComponent.h"

#include "ScannerCharacter.h"
#include "Engine/World.h"
#include "ScannerControllerComponent.h"
#include "ScannerIconsControllerComponent.h"
#include "Components/DecalComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialParameterCollectionInstance.h"

constexpr int32 GMaxFootprints = 99;

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

	Scanner = GetOwner()->GetComponentByClass<UScannerControllerComponent>();
	Icons = GetOwner()->GetComponentByClass<UScannerIconsControllerComponent>();
	
	// Setup the DMIs
	LeftFootstep = CreateFootstepDMI(false, false);
	LeftFootstepHighlight = CreateFootstepDMI(false, true);
	RightFootstep = CreateFootstepDMI(true, false);
	RightFootstepHighlight = CreateFootstepDMI(true, true);

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

	if (!IsValid(Scanner) || !IsValid(Icons)) return;
	
	double CurrentTime = GetWorld()->GetTimeSeconds();
	double ElapsedTime = CurrentTime - Scanner->GetCurrentFrameScannerState().StartTime;
	double RelativeHighlightTime = ElapsedTime - Icons->TotalEffectDuration();
	
	UMaterialParameterCollectionInstance* MPCI = GetWorld()->GetParameterCollectionInstance(MPC);
	MPCI->SetScalarParameterValue(TEXT("_Footprint_Relative_Highlight_Time"),
		FMath::Clamp(static_cast<float>(RelativeHighlightTime), 0.f, HighlightFadeTime));

	// Clean array from dead footprints
	Footprints.RemoveAll([this](const FFootprintData& Footprint)
	{
		return Footprint.Age >= Footprint.Lifetime;
	});
}

void UFootprintControllerComponent::HandleFootstep(EFootstepType FootstepType)
{
	TOptional<FFootprintData> Hit = CheckFootstepCollision(FootstepType);
	if (!Hit) return;
	
	FFootprintData Footprint = Hit.GetValue();
		
	if (Icons->IsEffectActive() && Scanner->IsPointInsideScanArea(Footprint.Location))
	{
		Footprint.IsHighlighted = true;
	}

	UMaterialInstanceDynamic* FootprintDMI = GetFootprintMaterial(Footprint);

	Footprint.Lifetime = ComputeLifetime(Footprint.IsHighlighted);

	UDecalComponent* FootprintDecal = UGameplayStatics::SpawnDecalAtLocation(GetWorld(),
		FootprintDMI, DecalSize, Footprint.Location, Footprint.Rotation, Footprint.Lifetime);
		
	FootprintDecal->SetFadeOut(Footprint.Lifetime - FadeTime, FadeTime, false);
		
	Footprint.Decal = FootprintDecal;

	// Save footprint in the controller's memory
	if (Footprints.Num() == GMaxFootprints)
	{
		UDecalComponent* Decal = Footprints[0].Decal;
			
		Footprints.RemoveAt(0, EAllowShrinking::No);
			
		Decal->DestroyComponent();
		if (AActor* Owner = Decal->GetOwner())
		{
			Owner->Destroy();
		}
	}
	Footprints.Add(Footprint);
}

void UFootprintControllerComponent::StartFootprintsLifecycle()
{
	for (FFootprintData& Footprint : Footprints)
	{
		if (!Footprint.Decal) break;
		
		bool bWasHighlighted = Footprint.IsHighlighted;
		Footprint.IsHighlighted = Scanner->IsPointInsideScanArea(Footprint.Location);
		
		bool bHighlightChange = Footprint.IsHighlighted != bWasHighlighted;

		if (bHighlightChange || Footprint.IsHighlighted)
		{
			Footprint.Age = 0.f;
			Footprint.Lifetime = ComputeLifetime(Footprint.IsHighlighted);

			if (bHighlightChange)
			{
				UMaterialInstanceDynamic* NewMaterial = GetFootprintMaterial(Footprint);
				Footprint.Decal->SetDecalMaterial(NewMaterial);
			}

			Footprint.Decal->SetLifeSpan(Footprint.Lifetime);
			Footprint.Decal->SetFadeOut(Footprint.Lifetime - FadeTime, FadeTime, false);
		}
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

UMaterialInstanceDynamic* UFootprintControllerComponent::GetFootprintMaterial(const FFootprintData& Footprint) const
{
	switch (Footprint.Type)
	{
		case EFootstepType::Left:
			return Footprint.IsHighlighted ? LeftFootstepHighlight : LeftFootstep;
		case EFootstepType::Right:
			return Footprint.IsHighlighted ? RightFootstepHighlight : RightFootstep;
		default:
			return nullptr;
	}
}

float UFootprintControllerComponent::ComputeLifetime(bool bHighlighted) const
{
	float Lifetime = RegularFootprintLifetime + FadeTime;
	
	if (bHighlighted)
	{
		double CurrentTime = GetWorld()->GetTimeSeconds();
		double ElapsedTime = CurrentTime - Scanner->GetCurrentFrameScannerState().StartTime;
			
		Lifetime += FMath::Max(0.f, Icons->TotalEffectDuration() - ElapsedTime) + HighlightFadeTime;
	}
	
	return Lifetime;
}

UMaterialInstanceDynamic* UFootprintControllerComponent::CreateFootstepDMI(bool bRight, bool bHighlighted) 
{
	auto* DMI = UMaterialInstanceDynamic::Create(DecalMaterial, this);
	DMI->SetScalarParameterValue(TEXT("IsRight"), bRight ? 1.f : 0.f);
	DMI->SetScalarParameterValue(TEXT("IsHighlighted"), bHighlighted ? 1.f : 0.f);
	return DMI;
}

void DEBUG_PrintTArray(const TArray<FFootprintData>& Footprints)
{
	FString Output = TEXT("[");

	for (int32 i = 0; i < Footprints.Num(); ++i)
	{
		const FFootprintData& FP = Footprints[i];

		Output += FString::Printf(TEXT("{%d%s}"), 
			i, 
			FP.IsHighlighted ? TEXT(",h") : TEXT(""));

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
