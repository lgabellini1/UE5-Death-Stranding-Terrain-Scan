#include "ScannerIconsControllerComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Landscape.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Components/SceneCaptureComponent2D.h"
#include "ScannerControllerComponent.h"
#include "Kismet/GameplayStatics.h"

namespace IconsTextureAtlas
{
	// Atlas size is 3x3. Consider UV coordinate system, so (0,0) is top-left.
	
	const FIntPoint FullSquare{0,0};
	const FIntPoint Cross{1,0};
	const FIntPoint EmptySquare{2,0};
	
	const FIntPoint Circle{0,1};
	const FIntPoint Triangle{1,1};
	const FIntPoint Chevron{2,1};
	
	const FIntPoint ReverseTriangle{0,2};
	const FIntPoint Waves{1,2};
}

const TMap<ETerrainType, FIntPoint>& GetTerrainIcons()
{
	// Each FVector value encodes a specific scan icon. The values
	// represent coordinates inside the icons' texture atlas asset. 
	static const TMap<ETerrainType, FIntPoint> TerrainIcons
	{
		{ ETerrainType::Regular, IconsTextureAtlas::FullSquare },
		{ ETerrainType::Steep, IconsTextureAtlas::FullSquare },
		{ ETerrainType::Dangerous, IconsTextureAtlas::Cross },
		{ ETerrainType::ShallowWater, IconsTextureAtlas::EmptySquare },
		{ ETerrainType::DeepWater, IconsTextureAtlas::EmptySquare },
		{ ETerrainType::DangerousWater, IconsTextureAtlas::EmptySquare },
		{ ETerrainType::Rocky, IconsTextureAtlas::Triangle },
		{ ETerrainType::Vegetation, IconsTextureAtlas::Chevron },
		{ ETerrainType::Path, IconsTextureAtlas::Circle }
	};
	return TerrainIcons;
}


UScannerIconsControllerComponent::UScannerIconsControllerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.SetTickFunctionEnable(true);

	DepthSceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>("DepthSceneCapture");

	NormalsSceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>("NormalsSceneCapture");

	IDsSceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>("IDsSceneCapture");

	CustomDepthSceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>("CustomDepthSceneCapture");

	CameraMesh = CreateDefaultSubobject<UStaticMeshComponent>("CameraMesh");
	CameraMesh->SetupAttachment(DepthSceneCapture);
}

void UScannerIconsControllerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!MPC) return;

	if (const AActor* Player = GetOwner())
	{
		ScannerController = Player->FindComponentByClass<UScannerControllerComponent>();

		// Impose this controller ticks after the general ScannerController.
		// This is required due to the SceneCapture material being dependent on _Terrain_Scan_Range,
		// which is updated there.
		AddTickPrerequisiteComponent(ScannerController);
	}
	
	
	// Spawn the Niagara Component and first setup.
	
	IconsNiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		IconsParticleSystem,
		FVector::ZeroVector,
		FRotator::ZeroRotator);

	
	// Set grid settings.
	IconsNiagaraComponent->SetVariableInt(TEXT("GridX"), GridX);
	IconsNiagaraComponent->SetVariableInt(TEXT("GridY"), GridY);
	IconsNiagaraComponent->SetVariableVec3(TEXT("Padding"), FVector{Padding, Padding, 0.0f});
	IconsNiagaraComponent->SetVariableFloat(TEXT("ZOffset"), ZOffset);

	// Set scan infos.
	if (auto* MPCI = GetWorld()->GetParameterCollectionInstance(MPC))
	{
		float Angle;
		MPCI->GetScalarParameterValue(TEXT("_Terrain_Scan_Arc_Angle"), Angle);
		IconsNiagaraComponent->SetVariableFloat(TEXT("HalfAngle"), Angle / 2);
	}

	IconsNiagaraComponent->SetVariableFloat(TEXT("ScanEndTime"), ScannerController->GetTotalScanDuration());

	// Terrain type representation inside Niagara
	IconsNiagaraComponent->SetVariableInt(TEXT("Regular Terrain"), static_cast<int32>(ETerrainType::Regular));
	IconsNiagaraComponent->SetVariableInt(TEXT("Steep Terrain"), static_cast<int32>(ETerrainType::Steep));
	IconsNiagaraComponent->SetVariableInt(TEXT("Dangerous Terrain"), static_cast<int32>(ETerrainType::Dangerous));
	
	IconsNiagaraComponent->SetVariableInt(TEXT("Shallow Water"), static_cast<int32>(ETerrainType::ShallowWater));
	IconsNiagaraComponent->SetVariableInt(TEXT("Deep Water"), static_cast<int32>(ETerrainType::DeepWater));
	IconsNiagaraComponent->SetVariableInt(TEXT("Dangerous Water"), static_cast<int32>(ETerrainType::DangerousWater));

	IconsNiagaraComponent->SetVariableInt(TEXT("Rocky Terrain"), static_cast<int32>(ETerrainType::Rocky));
	IconsNiagaraComponent->SetVariableInt(TEXT("Vegetation Terrain"), static_cast<int32>(ETerrainType::Vegetation));
	IconsNiagaraComponent->SetVariableInt(TEXT("Path Terrain"), static_cast<int32>(ETerrainType::Path));

	// Thresholds
	IconsNiagaraComponent->SetVariableFloat(TEXT("RegularTerrainThreshold"), RegularTerrainThreshold);
	IconsNiagaraComponent->SetVariableFloat(TEXT("SteepTerrainThreshold"), SteepTerrainThreshold);

	IconsNiagaraComponent->SetVariableFloat(TEXT("ShallowWaterThreshold"), ShallowWaterThreshold);
	IconsNiagaraComponent->SetVariableFloat(TEXT("DeepWaterThreshold"), DeepWaterThreshold);

	// Icon encodings
	IconsNiagaraComponent->SetVariableVec2(TEXT("Regular Terrain Icon"), *GetTerrainIcons().Find(ETerrainType::Regular));
	IconsNiagaraComponent->SetVariableVec2(TEXT("Steep Terrain Icon"), *GetTerrainIcons().Find(ETerrainType::Steep));
	IconsNiagaraComponent->SetVariableVec2(TEXT("Dangerous Terrain Icon"), *GetTerrainIcons().Find(ETerrainType::Dangerous));
	
	IconsNiagaraComponent->SetVariableVec2(TEXT("Shallow Water Icon"), *GetTerrainIcons().Find(ETerrainType::ShallowWater));
	IconsNiagaraComponent->SetVariableVec2(TEXT("Deep Water Icon"), *GetTerrainIcons().Find(ETerrainType::DeepWater));
	IconsNiagaraComponent->SetVariableVec2(TEXT("Dangerous Water Icon"), *GetTerrainIcons().Find(ETerrainType::DangerousWater));

	IconsNiagaraComponent->SetVariableVec2(TEXT("Rocky Icon"), *GetTerrainIcons().Find(ETerrainType::Rocky));
	IconsNiagaraComponent->SetVariableVec2(TEXT("Vegetation Icon"), *GetTerrainIcons().Find(ETerrainType::Vegetation));
	IconsNiagaraComponent->SetVariableVec2(TEXT("Path Icon"), *GetTerrainIcons().Find(ETerrainType::Path));

	// Sprite sizes
	IconsNiagaraComponent->SetVariableVec2(TEXT("Default Size"), FVector2D{DefaultSize});
	IconsNiagaraComponent->SetVariableVec2(TEXT("Danger Icon Size"), FVector2D{DangerIconSize});
	IconsNiagaraComponent->SetVariableVec2(TEXT("Water Icons Size"), FVector2D{WaterIconsSize});
	IconsNiagaraComponent->SetVariableVec2(TEXT("Flare Size"), FVector2D{FlareSize});
	IconsNiagaraComponent->SetVariableVec2(TEXT("Rocky Icons Size"), FVector2D{RockyIconsSize});
	IconsNiagaraComponent->SetVariableVec2(TEXT("Vegetation Icons Size"), FVector2D{VegetationIconsSize});
	IconsNiagaraComponent->SetVariableVec2(TEXT("Path Icons Size"), FVector2D{PathIconsSize});

	// Sprite colors
	IconsNiagaraComponent->SetVariableLinearColor(TEXT("Icon Blue"), FLinearColor::FromSRGBColor(IconBlue));
	IconsNiagaraComponent->SetVariableLinearColor(TEXT("Icon Yellow"), FLinearColor::FromSRGBColor(IconYellow));
	IconsNiagaraComponent->SetVariableLinearColor(TEXT("Icon Red"), FLinearColor::FromSRGBColor(IconRed));
	IconsNiagaraComponent->SetVariableLinearColor(TEXT("Icon Green"), FLinearColor::FromSRGBColor(IconGreen));
	
	// Opacity animation
	IconsNiagaraComponent->SetVariableInt(TEXT("TotalAnimationCycles"), TotalAnimationCycles);
	IconsNiagaraComponent->SetVariableFloat(TEXT("OpacityAnimationSpeed"), OpacityAnimationSpeed);
	IconsNiagaraComponent->SetVariableFloat(TEXT("FadeAnimationWidth"), FadeIntensityFactor);
	IconsNiagaraComponent->SetVariableFloat(TEXT("DangerIconFadeoutTime"), DangerIconFadeoutTime);
	IconsNiagaraComponent->SetVariableFloat(TEXT("DangerIconAppearOffset"), DangerIconAppearOffset);

	// Flare animation
	IconsNiagaraComponent->SetVariableFloat(TEXT("FlareDuration"), FlareAnimationDuration);
	
	
	// Setup SceneCapture(s) and target(s)

	SetupSceneCaptureComponent(DepthSceneCapture, SCS_SceneDepth);
	SetupSceneCaptureComponent(NormalsSceneCapture, SCS_Normal);
	SetupSceneCaptureComponent(IDsSceneCapture, SCS_FinalColorLDR);
	SetupSceneCaptureComponent(CustomDepthSceneCapture, SCS_FinalColorLDR);

	CameraMesh->SetVisibility(bEnableCameraVisualization);
}

void UScannerIconsControllerComponent::TickComponent
	(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Timing logic
	if (bHasStarted) ElapsedTime += DeltaTime;
	
	if (ElapsedTime > TotalEffectDuration())
	{
		ElapsedTime = -1.f;
		bHasStarted = false;
	}
	
	FScannerState CurrentScannerState = ScannerController->GetCurrentFrameScannerState();
	IconsNiagaraComponent->SetVariableFloat(TEXT("CurrentRange"), CurrentScannerState.Range);
}

void UScannerIconsControllerComponent::StartIconsLifecycle()
{
	if (!IconsNiagaraComponent || !DepthSceneCapture || !NormalsSceneCapture || !IDsSceneCapture) return;

	FScannerState CurrentScannerState = ScannerController->GetCurrentFrameScannerState();
	
	// Niagara System and Scene Capture positioning

	float Offset = (GridX * Padding) / 2;
	FVector Direction = CurrentScannerState.Rotation.Vector();
	Direction.Z = 0.0f; // Do not take into account eventual Z-shift encoded in direction
	Direction.Normalize();
	
	FVector DeltaLocation = Direction * Offset;

	IconsNiagaraComponent->SetWorldLocation(CurrentScannerState.Origin);
	IconsNiagaraComponent->SetWorldRotation(FRotator{0.0f, CurrentScannerState.Rotation.Yaw, 0.0f});
	IconsNiagaraComponent->AddWorldOffset(DeltaLocation);

	PlaceSceneCaptureComponent(DepthSceneCapture, CurrentScannerState, DeltaLocation);
	PlaceSceneCaptureComponent(NormalsSceneCapture, CurrentScannerState, DeltaLocation);
	PlaceSceneCaptureComponent(IDsSceneCapture, CurrentScannerState, DeltaLocation);
	PlaceSceneCaptureComponent(CustomDepthSceneCapture, CurrentScannerState, DeltaLocation);

	if (bEnableCameraVisualization)
	{
		CameraMesh->SetWorldLocationAndRotation(DepthSceneCapture->GetComponentLocation(),
			DepthSceneCapture->GetComponentRotation());

		DrawCameraViewFrustum(GetWorld(), DepthSceneCapture);
	}

	float CameraZ = DepthSceneCapture->GetComponentLocation().Z;
	
	// Capture depth and normals
	DepthSceneCapture->CaptureScene();
	NormalsSceneCapture->CaptureScene();
	IDsSceneCapture->CaptureScene();
	CustomDepthSceneCapture->CaptureScene();

	// Send data to Niagara.
	IconsNiagaraComponent->SetVariablePosition(TEXT("ScanOrigin"), CurrentScannerState.Origin);
	IconsNiagaraComponent->SetVariablePosition(TEXT("GridOrigin"), IconsNiagaraComponent->GetComponentLocation());
	IconsNiagaraComponent->SetVariableVec3(TEXT("DirectionVector"), CurrentScannerState.Rotation.Vector());
	IconsNiagaraComponent->SetVariableFloat(TEXT("CameraZ"), CameraZ);
	
	// Start particle generation.
	IconsNiagaraComponent->ReinitializeSystem();
	ElapsedTime = 0.f;
	bHasStarted = true;
}

void UScannerIconsControllerComponent::SetupSceneCaptureComponent(USceneCaptureComponent2D* const SceneCaptureComponent,
	ESceneCaptureSource CaptureSource) const
{
	SceneCaptureComponent->CaptureSource = CaptureSource;
	SceneCaptureComponent->bCaptureEveryFrame = false;
	SceneCaptureComponent->bCaptureOnMovement = false;
	
	float AspectRatio = IconsAreaY() / IconsAreaX();
	SceneCaptureComponent->TextureTarget->SizeX = FMath::CeilToInt32(RenderTargetsHeight * AspectRatio);
	SceneCaptureComponent->TextureTarget->SizeY = RenderTargetsHeight;

	SceneCaptureComponent->ProjectionType = ECameraProjectionMode::Type::Orthographic;
	SceneCaptureComponent->OrthoWidth = IconsAreaY() + Padding;

	// Do not capture grass as it creates inconsistencies both in icons height and slope calculation.
	// This can be extended of course with other flags depending on scene composition, landscape layers, etc...
	SceneCaptureComponent->SetShowFlagSettings({{"InstancedGrass",false}});
}

void UScannerIconsControllerComponent::PlaceSceneCaptureComponent(
	USceneCaptureComponent2D* const SceneCaptureComponent,
	const FScannerState& CurrentScannerState, const FVector& Movement) const
{
	SceneCaptureComponent->SetWorldLocation(CurrentScannerState.Origin + FVector::UpVector * SceneCaptureHeightOffset);
	SceneCaptureComponent->SetWorldRotation(FRotator{-90.0f, CurrentScannerState.Rotation.Yaw, 0.0f});
	SceneCaptureComponent->AddWorldOffset(Movement);
}

float UScannerIconsControllerComponent::TotalEffectDuration() const
{
	return (RevealAnimationDuration() + FadeAnimationDuration()) * TotalAnimationCycles
		+  FadeAnimationDuration() + DangerIconFadeoutTime;
}

bool UScannerIconsControllerComponent::IsEffectActive() const
{
	return ElapsedTime >= 0 && ElapsedTime <= TotalEffectDuration();
}

void DrawCameraViewFrustum(const UWorld* World, USceneCaptureComponent2D* SceneCaptureComponent)
{
	FMinimalViewInfo CameraView;
	SceneCaptureComponent->GetCameraView(0.f, CameraView);
		
	FMatrix View, Projection, ViewProjection;
	UGameplayStatics::GetViewProjectionMatrix(CameraView,
		View, Projection, ViewProjection);

	UKismetSystemLibrary::FlushPersistentDebugLines(World);
		
	DrawDebugFrustum(
		World,
		ViewProjection.Inverse(),
		FColor::Orange, true);
}
