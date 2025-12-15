#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FootprintControllerComponent.generated.h"

class UScannerControllerComponent;
class UScannerIconsControllerComponent;

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

	/** Expected lifetime: footprint dies when Age >= Lifetime. */
	float Lifetime;

	bool IsHighlighted;

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
	
	/** Footprint parent material. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Decal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> DecalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Decal", meta = (AllowPrivateAccess = "true"))
	FVector DecalSize;


 	/** Time in seconds for the lifetime of a regular footprint, not including fade time. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time", meta = (AllowPrivateAccess = "true"))
	float RegularFootprintLifetime = 10.f;
	
	/** Time in seconds it takes for the footprint to fade permanently and die, after lifetime expiration. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time", meta = (AllowPrivateAccess = "true"))
	float FadeTime = 5.f;

	/** Time in seconds it takes for the highlight effect to fade. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time", meta = (AllowPrivateAccess = "true"))
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

	float ComputeLifetime(bool bHighlighted) const;

	UMaterialInstanceDynamic* CreateFootstepDMI(bool bRight, bool bHighlighted);

	/**
	 * Collection of all the footprints currently present in-game.
	 */
	TArray<FFootprintData> Footprints;

	UPROPERTY()
	UScannerControllerComponent* Scanner;

	UPROPERTY()
	UScannerIconsControllerComponent* Icons;

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
	
	UMaterialInstanceDynamic* GetFootprintMaterial(const FFootprintData& Footprint) const;
};
