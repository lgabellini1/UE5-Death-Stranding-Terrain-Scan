#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ScannerControllerComponent.generated.h"

class UScannerControllerComponent;
class UMaterialParameterCollection;
class UScannerIconsControllerComponent;


UENUM()
enum class EScannerAnimationState : uint8
{
	Inactive,
	Spawning,
	Prewarm,
	Expanding
};


/**
 *	Represents the state of the scanner at a given frame.
 */
USTRUCT()
struct FScannerState
{
	GENERATED_BODY();
	
	UPROPERTY()
	FVector Origin;
	
	UPROPERTY()
	FRotator Rotation;

	UPROPERTY()
	float Angle;
	
	UPROPERTY()
	EScannerAnimationState AnimationState = EScannerAnimationState::Inactive;

	/** Note: this is the elapsed time relative to the current
	 *  animation state, not since the start of the whole lifecycle. */
	UPROPERTY()
	float ElapsedTime;

	UPROPERTY()
	double StartTime;
	
	UPROPERTY()
	float Range;

	UPROPERTY()
	float Speed;

	UPROPERTY()
	float Opacity;

	UPROPERTY()
	float DarkCircleOpacity;
};


UCLASS(ClassGroup=(Custom), Blueprintable, meta=(BlueprintSpawnableComponent))
class DSTERRAINSCAN_API UScannerControllerComponent : public UActorComponent
{
	GENERATED_BODY()

public: /* Constructor(s) */
	
	UScannerControllerComponent();

protected:
	virtual void BeginPlay() override;

private: /* Blueprint-exposed parameters */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (AllowPrivateAccess = "true"))
	float DistanceBetweenLines = 350.0f;

	/**
	 * Thickness (in pixels) of a single scan line.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (AllowPrivateAccess = "true"))
	float OutlineThickness = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|Arc", meta = (AllowPrivateAccess = "true"))
	float ArcAngle = 120.0f;

	/**
	 * Strength of the blending on the arc sides.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|Arc", meta = (AllowPrivateAccess = "true"))
	float ArcBlendFactor = 2.5f;

	/**
	 * Starting point of the edge gradient as a percentage relative to the scan origin (a value of 0 means
	 * that the gradient touches the origin and the scan cone is fully covered, and so on).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|Edge Gradient", meta = (AllowPrivateAccess = "true"))
	float EdgeGradientStart = 0.85f;

	/**
	 * Strength of the blending of the edge gradient effect.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|Edge Gradient", meta = (AllowPrivateAccess = "true"))
	float EdgeGradientFalloff = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|Dark Circle", meta = (AllowPrivateAccess = "true"))
	float DarkCircleSize = 800.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|Colors", meta = (AllowPrivateAccess = "true"))
	FColor ScanLinesColor{219,249,255};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|Colors", meta = (AllowPrivateAccess = "true"))
	FColor FirstLineColor = FColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|Colors", meta = (AllowPrivateAccess = "true"))
	FColor EdgeGradientStartColor{23,0,255};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|Colors", meta = (AllowPrivateAccess = "true"))
	FColor EdgeGradientEndColor{181, 216, 255};
	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Durations", meta = (AllowPrivateAccess = "true"))
	float SpawnAnimationDuration = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Durations", meta = (AllowPrivateAccess = "true"))
	float PrewarmAnimationDuration = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Durations", meta = (AllowPrivateAccess = "true"))
	float ExpansionAnimationDuration = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Durations", meta = (AllowPrivateAccess = "true"))
	float FadeoutDuration = 1.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Durations", meta = (AllowPrivateAccess = "true"))
	float DarkCircleFadeoutDuration = 1.7f;
	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Options", meta = (AllowPrivateAccess = "true"))
	float SpawnInitialRange = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Options", meta = (AllowPrivateAccess = "true"))
	float SpawnInitialSpeed = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Options", meta = (AllowPrivateAccess = "true"))
	float SpawnFinalOpacity = 0.55f;
	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pre-warm Options", meta = (AllowPrivateAccess = "true"))
	float PrewarmSpeed = 200.0f;
	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Expansion Options", meta = (AllowPrivateAccess = "true"))
	float ExpansionMaxSpeed = 20000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Expansion Options", meta = (AllowPrivateAccess = "true"))
	float ExpansionMaxSpeedDuration = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Expansion Options", meta = (AllowPrivateAccess = "true"))
	float ExpansionFinalSpeed = 500.0f;

	
public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	
	/**
	 *	Starts the scan process from this component's parent
	 *	world position and facing direction.
	 */
	void StartScannerLifecycle();

	constexpr float GetScannerFinalRange() const;

	constexpr float GetTotalScanDuration() const;
	
	/**
	 * Returns true if the given point lies inside the scan effect area (only angle check).
	 * 
	 * @param Point to test against the scan area.
	 * @return true if the point lies inside, else false.
	 */
	bool IsPointInsideScanArea(const FVector& Point) const;

	constexpr const FScannerState& GetCurrentFrameScannerState() const { return CurrentScannerState; }
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parameters", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialParameterCollection> MPC;

private: /* Class internals */

	UPROPERTY()
	FScannerState CurrentScannerState;

	void UpdateScanSpeedAndRange(float DeltaTime);

	void UpdateScanOpacity(float DeltaTime);
};
