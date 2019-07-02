#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VRMotionController.generated.h"

UENUM(BlueprintType)
enum class E_GRIPSTATE : uint8
{
	GRIP_OPEN,
	GRIP_CANGRAB,
	GRIP_GRAB
};

UCLASS()
class PROJECTANTIGONUS_API AVRMotionController : public AActor
{
	GENERATED_BODY()

private:

	EControllerHand m_hand;

	bool b_isroomscale;

	bool b_shouldgrip;

	UPROPERTY()
	UHapticFeedbackEffect_Base *haptic_motioncontroller;

	UPROPERTY()
	AActor *attachedactor;

	//---Room scale set up---//
	void SetupRoomScaleOutline();

	//---Controller rumble---//
	void RumbleController(float intensity);

	//---Grabbing---//
	UPROPERTY()
	AActor* GetActorNearHand();

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent *scene;

	//---Hand---//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class UMotionControllerComponent *motioncontroller;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent *handmesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class USphereComponent *grabsphere;

	//---Teleport Cylinder---//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent *teleportcylinder;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent *ring;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent *arrow;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent *roomscalemesh;

	//---SteamVRChaperone---//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class USteamVRChaperoneComponent *steamvrchaperone;

	//---Grip Enum---//
	UPROPERTY(BlueprintReadOnly)
	E_GRIPSTATE gripstate;

	virtual void BeginPlay() override;

public:	

	AVRMotionController();
	AVRMotionController(EControllerHand hand);

	virtual void Tick(float DeltaTime) override;

};
