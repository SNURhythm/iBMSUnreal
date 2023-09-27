#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "shiftjis.generated.h"

UCLASS()
class IBMSUNREAL_API Ashiftjis : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this empty's properties
	Ashiftjis();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
