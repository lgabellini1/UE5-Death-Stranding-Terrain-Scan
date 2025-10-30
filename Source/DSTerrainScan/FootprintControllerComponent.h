#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FootprintControllerComponent.generated.h"

UENUM(BlueprintType)
enum class EFootstepType : uint8
{
	Left,
	Right
};

USTRUCT()
struct FFootprintData
{
	GENERATED_BODY();

	FVector Location;

	FRotator Rotation;

	EFootstepType Type;

	float Age;

	bool IsHighlight;

	UPROPERTY()
	UDecalComponent* Decal;
};

UCLASS(ClassGroup=(Custom), Blueprintable, meta=(BlueprintSpawnableComponent))
class DSTERRAINSCAN_API UFootprintControllerComponent : public UActorComponent
{
	GENERATED_BODY()

public: // Constructor(s)
	
	UFootprintControllerComponent();

private:
	
	/**
	 * Parent material for the instances.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Decal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> DecalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Decal", meta = (AllowPrivateAccess = "true"))
	FVector DecalSize;

	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timings", meta = (AllowPrivateAccess = "true"))
	float RegularFootprintLifetime = 10.f;
	
	/**
	 * Time in seconds it takes for the footprint to fade permanently and die, after lifetime expiration.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timings", meta = (AllowPrivateAccess = "true"))
	float FadeTime = 5.f;

	/**
	 * Time in seconds it takes for the highlight effect to fade and reveal a regular footprint.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timings", meta = (AllowPrivateAccess = "true"))
	float HighlightFadeTime = 10.f;	
	

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MPC", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialParameterCollection> MPC;

protected:
	virtual void BeginPlay() override;

public:
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * Handles footprint spawning in response to player's walking.
	 * @param FootstepType left or right.
	 */
	void HandleFootstep(EFootstepType FootstepType);

	void StartFootprintsLifecycle();

private:
	
	TOptional<FFootprintData> CheckFootstepCollision(EFootstepType FootstepType) const;

	/**
	 * Collection of all the footprints currently present in-game.
	 */
	TArray<FFootprintData> Footprints;
	
	float GetFootprintLifetime(const FFootprintData& Footprint) const;

private: // Decal DMIs

	// Each DMI drives a different batch of footprints. All the footprints in the batch
	// share the same state, material-wise.
	
	UPROPERTY()
	UMaterialInstanceDynamic* LeftFootstep;

	UPROPERTY()
	UMaterialInstanceDynamic* LeftFootstepHighlight;

	UPROPERTY()
	UMaterialInstanceDynamic* RightFootstep;

	UPROPERTY()
	UMaterialInstanceDynamic* RightFootstepHighlight;
	
	constexpr UMaterialInstanceDynamic* GetFootprintMaterial(const FFootprintData& Footprint) const;
};
