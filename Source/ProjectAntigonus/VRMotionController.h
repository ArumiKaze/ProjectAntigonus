#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VRMotionController.generated.h"

UCLASS()
class PROJECTANTIGONUS_API AVRMotionController : public AActor
{
	GENERATED_BODY()

private:

	EControllerHand m_hand;

	bool b_isroomscale;

	UPROPERTY()
	UHapticFeedbackEffect_Base* haptic_motioncontroller;

	void SetupRoomScaleOutline();

	void RumbleController(float intensity);

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* Scene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class UMotionControllerComponent* MotionController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* HandMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class USteamVRChaperoneComponent* SteamVRChaperone;

	//---Teleport Cylinder---//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* TeleportCylinder;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Ring;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Arrow;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* RoomScaleMesh;

	virtual void BeginPlay() override;

public:	

	AVRMotionController();
	AVRMotionController(EControllerHand hand);

	virtual void Tick(float DeltaTime) override;

};
