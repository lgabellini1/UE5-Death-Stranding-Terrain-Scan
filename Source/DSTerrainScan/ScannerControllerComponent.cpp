#include "ScannerControllerComponent.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"

UScannerControllerComponent::UScannerControllerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.SetTickFunctionEnable(true);
}

void UScannerControllerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!MPC) return;

	// Initializes the scanner by setting starting and default parameters.
	if (UMaterialParameterCollectionInstance* MPCInstance = GetWorld()->GetParameterCollectionInstance(MPC))
	{
		MPCInstance->SetScalarParameterValue(TEXT("_Terrain_Scan_Range"), 0.0f);
		MPCInstance->SetScalarParameterValue(TEXT("_Effect_Opacity"), 0.0f);
		MPCInstance->SetScalarParameterValue(TEXT("_Dark_Circle_Range"), 0.0f);

		MPCInstance->SetScalarParameterValue(TEXT("_Distance_Between_Scan_Lines"), DistanceBetweenLines);
		MPCInstance->SetScalarParameterValue(TEXT("_Outline_Thickness"), OutlineThickness);
		MPCInstance->SetScalarParameterValue(TEXT("_Terrain_Scan_Arc_Angle"), ArcAngle);
		CurrentScannerState.Angle = ArcAngle;
		MPCInstance->SetScalarParameterValue(TEXT("_Terrain_Scan_Arc_Blend_Factor"), ArcBlendFactor);
		MPCInstance->SetScalarParameterValue(TEXT("_Edge_Gradient_Start"), EdgeGradientStart);
		MPCInstance->SetScalarParameterValue(TEXT("_Edge_Gradient_Falloff"), EdgeGradientFalloff);
		MPCInstance->SetScalarParameterValue(TEXT("_Dark_Circle_Size"), DarkCircleSize);

		MPCInstance->SetVectorParameterValue(TEXT("_Terrain_Scan_Color"),
			FLinearColor::FromSRGBColor(ScanLinesColor));
		MPCInstance->SetVectorParameterValue(TEXT("_First_Line_Color"),
			FLinearColor::FromSRGBColor(FirstLineColor));
		MPCInstance->SetVectorParameterValue(TEXT("_Edge_Gradient_Color_Start"),
			FLinearColor::FromSRGBColor(EdgeGradientStartColor));
		MPCInstance->SetVectorParameterValue(TEXT("_Edge_Gradient_Color_End"),
			FLinearColor::FromSRGBColor(EdgeGradientEndColor));
	}
}

void UScannerControllerComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!MPC) return;

	// Avoids subsequent, unmeaningful updates to the Inactive state
	if (CurrentScannerState.AnimationState == EScannerAnimationState::Inactive)
	{
		return;
	}

	CurrentScannerState.ElapsedTime += DeltaTime;

	// If conditions are met, switch to the successive state
	switch (CurrentScannerState.AnimationState)
	{
		// Spawning -> Prewarm
		case EScannerAnimationState::Spawning:
			if (CurrentScannerState.ElapsedTime >= SpawnAnimationDuration)
			{
				CurrentScannerState.ElapsedTime -= SpawnAnimationDuration;
				CurrentScannerState.AnimationState = EScannerAnimationState::Prewarm;
			}
			break;

		// Prewarm -> Expanding
		case EScannerAnimationState::Prewarm:
			if (CurrentScannerState.ElapsedTime >= PrewarmAnimationDuration)
			{
				CurrentScannerState.ElapsedTime -= PrewarmAnimationDuration;
				CurrentScannerState.AnimationState = EScannerAnimationState::Expanding;
			}
			break;

		// Expanding -> Inactive
		case EScannerAnimationState::Expanding:
			if (CurrentScannerState.ElapsedTime >= ExpansionAnimationDuration)
			{
				CurrentScannerState.AnimationState = EScannerAnimationState::Inactive;
			}
			break;

		default: return;
	}

	UpdateScanSpeedAndRange(DeltaTime);
	
	UpdateScanOpacity(DeltaTime);
}

void UScannerControllerComponent::StartScannerLifecycle()
{
	if (!IsValid(MPC)) return;
	
	// Scanner should be previously inactive
	if (CurrentScannerState.AnimationState != EScannerAnimationState::Inactive)
	{
		return;
	}

	// Set spawn state
	CurrentScannerState.AnimationState = EScannerAnimationState::Spawning;
	CurrentScannerState.ElapsedTime = 0.0f;
	CurrentScannerState.StartTime = GetWorld()->GetTimeSeconds();
	CurrentScannerState.Range = SpawnInitialRange;
	CurrentScannerState.Speed = SpawnInitialSpeed;
	
	CurrentScannerState.Origin = GetOwner()->GetActorLocation();

	APlayerCameraManager* CameraManager = GetWorld()->GetFirstPlayerController()->PlayerCameraManager;
	CurrentScannerState.Rotation = CameraManager->GetCameraRotation();
	
	// Note that this code assumes the owner to be the player character.
	// To generalize it for any character, one would need to find some
	// way to get its internal camera rotation.

	if (UMaterialParameterCollectionInstance* MPCInstance = GetWorld()->GetParameterCollectionInstance(MPC))
	{
		MPCInstance->SetVectorParameterValue(TEXT("_Terrain_Scan_Origin"), CurrentScannerState.Origin);
		MPCInstance->SetVectorParameterValue(TEXT("_Terrain_Scan_Direction"),
			CurrentScannerState.Rotation.Vector());
	}
}

constexpr float UScannerControllerComponent::GetScannerFinalRange() const
{
	// For each phase, range is either constant or a linear growth function
	return SpawnInitialRange
	+ SpawnAnimationDuration * ((SpawnInitialSpeed + PrewarmSpeed) / 2)
	+ PrewarmAnimationDuration * PrewarmSpeed
	+ ExpansionMaxSpeedDuration * ((PrewarmSpeed + ExpansionMaxSpeed) / 2)
	+ (ExpansionAnimationDuration - ExpansionMaxSpeedDuration) * ((ExpansionMaxSpeed + ExpansionFinalSpeed) / 2);
} 

void UScannerControllerComponent::UpdateScanSpeedAndRange(float DeltaTime)
{
	if (DeltaTime > CurrentScannerState.ElapsedTime) 
	{
		// Part of DeltaTime passed at the previous speed value.
		// Add that value before changing speed.
		CurrentScannerState.Range += (DeltaTime - CurrentScannerState.ElapsedTime) * CurrentScannerState.Speed;
	}
	
	switch (CurrentScannerState.AnimationState)
	{
		case EScannerAnimationState::Spawning:
			CurrentScannerState.Speed = FMath::Lerp(SpawnInitialSpeed, PrewarmSpeed,
				FMath::Clamp(CurrentScannerState.ElapsedTime / SpawnAnimationDuration, 0.0f, 1.0f));
			break;
		
		case EScannerAnimationState::Prewarm:
			break; // Leave speed as it is
		
		case EScannerAnimationState::Expanding:
			// Accelerate and then slow down
			if (CurrentScannerState.ElapsedTime <= ExpansionMaxSpeedDuration)
			{
				CurrentScannerState.Speed = FMath::Lerp(PrewarmSpeed, ExpansionMaxSpeed,
					FMath::Clamp(CurrentScannerState.ElapsedTime / ExpansionMaxSpeedDuration, 0.0f, 1.0f));
			}
			else
			{
				float DecelerationProgress = (CurrentScannerState.ElapsedTime - ExpansionMaxSpeedDuration) /
					(ExpansionAnimationDuration - ExpansionMaxSpeedDuration);

				CurrentScannerState.Speed = FMath::Lerp(ExpansionMaxSpeed, ExpansionFinalSpeed, FMath::Clamp(
					DecelerationProgress, 0.0f, 1.0f));
			}
			break;

		case EScannerAnimationState::Inactive:
		default:
			CurrentScannerState.Speed = 0.0f;
			break;
	}

	// As per the first comment above...
	if (DeltaTime > CurrentScannerState.ElapsedTime)
	{
		CurrentScannerState.Range += CurrentScannerState.ElapsedTime * CurrentScannerState.Speed;
	}
	else
	{
		CurrentScannerState.Range += DeltaTime * CurrentScannerState.Speed;
	}

	if (UMaterialParameterCollectionInstance* MPCInstance = GetWorld()->GetParameterCollectionInstance(MPC))
	{
		MPCInstance->SetScalarParameterValue(TEXT("_Terrain_Scan_Range"), CurrentScannerState.Range);
	}
}

void UScannerControllerComponent::UpdateScanOpacity(float DeltaTime)
{
	switch (CurrentScannerState.AnimationState)
	{
		case EScannerAnimationState::Spawning:
			CurrentScannerState.Opacity = FMath::Lerp(0.0f, SpawnFinalOpacity,
				FMath::Clamp(CurrentScannerState.ElapsedTime / SpawnAnimationDuration, 0.0f, 1.0f));
			CurrentScannerState.DarkCircleOpacity = 1.0f;
			break;

		case EScannerAnimationState::Prewarm:
			break; // Leave opacity as it is

		case EScannerAnimationState::Expanding:
			{
				float FadeoutStartTime = ExpansionAnimationDuration - FadeoutDuration;
				if (CurrentScannerState.ElapsedTime >= FadeoutStartTime)
				{
					CurrentScannerState.Opacity = FMath::Lerp(SpawnFinalOpacity, 0.0f,
						FMath::Clamp((CurrentScannerState.ElapsedTime - FadeoutStartTime) / FadeoutDuration, 0.0f, 1.0f));
				}

				if (CurrentScannerState.ElapsedTime >= DarkCircleFadeoutDuration)
				{
					CurrentScannerState.DarkCircleOpacity = 1.0f - FMath::Clamp(
						CurrentScannerState.ElapsedTime / DarkCircleFadeoutDuration, 0.0f, 1.0f);
				}
			}
			break;

		case EScannerAnimationState::Inactive:
			CurrentScannerState.Opacity = 0.0f;
			CurrentScannerState.DarkCircleOpacity = 0.0f;
			break;
	}

	if (UMaterialParameterCollectionInstance* MPCInstance = GetWorld()->GetParameterCollectionInstance(MPC))
	{
		MPCInstance->SetScalarParameterValue(TEXT("_Effect_Opacity"), CurrentScannerState.Opacity);
		MPCInstance->SetScalarParameterValue(TEXT("_Dark_Circle_Opacity"), CurrentScannerState.DarkCircleOpacity);
	}
}

constexpr float UScannerControllerComponent::GetTotalScanDuration() const
{
	return SpawnAnimationDuration + PrewarmAnimationDuration + ExpansionAnimationDuration;
}

bool UScannerControllerComponent::IsPointInsideScanArea(const FVector& Point) const
{
	FVector2D PointDirectionVectorXY = FVector2D{Point - CurrentScannerState.Origin};

	FVector2D ScannerDirectionVectorXY = FVector2D{CurrentScannerState.Rotation.Vector()};
	float CosineAngleBetweenDirections = ScannerDirectionVectorXY.GetSafeNormal().Dot(
		PointDirectionVectorXY.GetSafeNormal());
	float HalfAngle = CurrentScannerState.Angle * 0.5f;

	return CosineAngleBetweenDirections >= FMath::Cos(FMath::DegreesToRadians(HalfAngle));
}
