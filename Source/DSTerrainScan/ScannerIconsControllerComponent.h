#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ScannerIconsControllerComponent.generated.h"

class UScannerControllerComponent;
class UNiagaraSystem;
class USceneCaptureComponent2D;
class UNiagaraComponent;
class UNiagaraDataInterfaceLandscape;
class UTextureRenderTarget2D;
class UMaterialParameterGroup;
class UMaterial;
struct FScannerState;

UENUM()
enum class ETerrainType : int32
{
	Regular,
	Steep,
	Dangerous,
	
	// Water terrains
	ShallowWater,
	DeepWater,
	DangerousWater
};


UCLASS(ClassGroup=(Custom), Blueprintable, meta=(BlueprintSpawnableComponent))
class DSTERRAINSCAN_API UScannerIconsControllerComponent : public UActorComponent
{
	GENERATED_BODY()

public: /* Constructor(s) */
	
	UScannerIconsControllerComponent();

private: /* Blueprint-exposed parameters */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Particle System", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraSystem> IconsParticleSystem;

	
	/* Grid settings */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Particle System|Grid Settings", meta = (AllowPrivateAccess = "true"))
	int32 GridX = 75;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Particle System|Grid Settings", meta = (AllowPrivateAccess = "true"))
	int32 GridY = 140;

	/**
	 * Distance between the icons from each other in the grid.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Particle System|Grid Settings", meta = (AllowPrivateAccess = "true"))
	float Padding = 60.0f;

	/**
	 * Vertical offset of the icon from the terrain.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Particle System|Grid Settings", meta = (AllowPrivateAccess = "true"))
	float ZOffset = 40.0f;

	
	/* Thresholds */

	/**
	 * Maximum slope of regular terrain, derived from normals. Keep in mind
	 * that a value of 1.0 means a perfectly flat surface, and a value of 0.0
	 * a perpendicular surface, i.e. fully vertical, for example a wall.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Particle System|Thresholds", meta = (AllowPrivateAccess = "true"))
	float RegularTerrainThreshold = 0.8f;

	/**
	 * Maximum slope of steep terrain. See RegularTerrainThreshold for more info.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Particle System|Thresholds", meta = (AllowPrivateAccess = "true"))
	float SteepTerrainThreshold = 0.7f;

	/**
	 * Maximum water depth (in Unreal Units) for shallow water.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Particle System|Thresholds", meta = (AllowPrivateAccess = "true"))
	float ShallowWaterThreshold = 100.0f;

	/**
	 * Maximum water depth (in Unreal Units) for deep water.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Particle System|Thresholds", meta = (AllowPrivateAccess = "true"))
	float DeepWaterThreshold = 500.0f;

	
	/* Sprite sizes */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Particle System|Sprite Sizes", meta = (AllowPrivateAccess = "true"))
	float DefaultSize = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Particle System|Sprite Sizes", meta = (AllowPrivateAccess = "true"))
	float DangerIconSize = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Particle System|Sprite Sizes", meta = (AllowPrivateAccess = "true"))
	float WaterIconsSize = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Particle System|Sprite Sizes", meta = (AllowPrivateAccess = "true"))
	float FlareSize = 750.0f;

	
	/* Sprite colors */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Particle System|Colors", meta = (AllowPrivateAccess = "true"))
	FColor IconBlue{64,206,229};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Particle System|Colors", meta = (AllowPrivateAccess = "true"))
	FColor IconYellow{237, 223, 66};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Particle System|Colors", meta = (AllowPrivateAccess = "true"))
	FColor IconRed{220, 45, 67};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Particle System|Colors", meta = (AllowPrivateAccess = "true"))
	FColor IconGreen{68, 151, 53};

	
	/* Opacity animation */

	/**
	 * Each cycle consists of a single reveal-fade animation.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Particle System|Opacity Animation", meta = (AllowPrivateAccess = "true"))
	int32 TotalAnimationCycles = 2;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Particle System|Opacity Animation", meta = (AllowPrivateAccess = "true"))
	float OpacityAnimationSpeed = 2000.0f;

	/**
	 * Strength of the fade step. With lower values you get a sharper animation.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Particle System|Opacity Animation", meta = (AllowPrivateAccess = "true"))
	float FadeIntensityFactor = 4000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Particle System|Opacity Animation", meta = (AllowPrivateAccess = "true"))
	float DangerIconFadeoutTime = 5.0f;

	/**
	 *  How much space before the scan edge (in Unreal Units) the red icons show up in the effect.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Particle System|Opacity Animation", meta = (AllowPrivateAccess = "true"))
	float DangerIconAppearOffset = 2000.0f;

	
	/* Flare animation */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Particle System|Flare Animation", meta = (AllowPrivateAccess = "true"))
	float FlareAnimationDuration = 0.35f;

	
	/* Water */
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Particle System|Water", meta = (AllowPrivateAccess = "true"))
	float WaterLevelHeight = -3910.0f;

	
	/**
	 * Captures terrain depth into a texture (used to derive particle height).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene Capture", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneCaptureComponent2D> DepthSceneCapture;

	/**
	 * Captures terrain normals into a texture (used to derive particle inclination with respect to the terrain).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene Capture", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneCaptureComponent2D> NormalsSceneCapture;
	
	/** Additional SceneCaptureComponent2D height. Any value works as long we do not
	 *	intersect geometry (for ortho perspective). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene Capture", meta = (AllowPrivateAccess = "true"))
	float SceneCaptureHeight = 40.0f;
	
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Other", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialParameterCollection> MPC;

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	
	void StartIconsLifecycle();

	bool IsEffectActive() const;

	float TotalEffectDuration() const;

private: /* Class internals */

	constexpr float IconsAreaX() const { return (GridX - 1) * Padding; }

	constexpr float IconsAreaY() const { return (GridY - 1) * Padding; }

	float MaxEffectDistance() const { return FMath::Sqrt(FMath::Pow(IconsAreaX(), 2.f) + FMath::Pow(IconsAreaY() / 2, 2.f)); }
	

	float RevealAnimationDuration() const { return MaxEffectDistance() / OpacityAnimationSpeed; }

	float FadeAnimationDuration() const { return (MaxEffectDistance() + FadeIntensityFactor) / OpacityAnimationSpeed; }
	
	
	float ElapsedTime = -1.f;

	bool bHasStarted = false;
	

	UPROPERTY()
	TObjectPtr<UScannerControllerComponent> ScannerController;

	/**
	 *	World-positioned Niagara component for icon generation.
	 */
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> IconsNiagaraComponent;
	
	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> DepthRT;
	
	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> NormalsRT;
};
