#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ScannerCharacter.generated.h"

class UFootprintControllerComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UScannerControllerComponent;
class UScannerIconsControllerComponent;
struct FInputActionValue;
struct FScannerState;
enum class EFootstepType : uint8;


UCLASS(BlueprintType)
class DSTERRAINSCAN_API AScannerCharacter : public ACharacter
{
	GENERATED_BODY()
	
public: /* Constructor(s) */
	
	AScannerCharacter();
	
private: /* Blueprint-exposed parameters*/
	
	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> JumpAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> ScanAction;

	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scanner", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UScannerControllerComponent> ScannerController;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scanner", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UScannerIconsControllerComponent> ScannerIconsController;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footprints", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFootprintControllerComponent> FootprintController;

public:
	
	/**
	 * Called inside the player's AnimationBP. Listens for AnimNotifies from bone sockets
	 * and delegates footstep spawning to the controller.
	 * @param FootstepType left or right foot.
	 */	
	UFUNCTION(BlueprintCallable)
	void OnFootStep(EFootstepType FootstepType);	

protected:
	virtual void BeginPlay() override;
	
	virtual void Tick(float DeltaTime) override;
    	
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	virtual void NotifyControllerChanged() override;

	
	void Move(const FInputActionValue& Value);

	void Look(const FInputActionValue& Value);

	void Scan(const FInputActionValue&);
};
